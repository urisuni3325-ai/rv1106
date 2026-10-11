#include <string.h>
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "esp_check.h"
#include "esp_heap_caps.h"
#include "driver/jpeg_decode.h"
#include "driver/ppa.h"
#include "esp_h264_enc_single_hw.h"
#include "esp_h264_alloc.h"
#include "video_enc.h"

static const char *TAG = "venc";

#define ALIGN16_DOWN(x) ((x) & ~15)
#define ALIGN_UP_TO(x, a) (((x) + (a) - 1) / (a) * (a))

/* 디코드 태스크 소유 */
static jpeg_decoder_handle_t s_jpeg;
static ppa_client_handle_t s_ppa;
static uint8_t *s_rgb;       /* PPA 경로용 RGB565 디코드 결과 */
static size_t s_rgb_size;
static int s_path_logged = -1;

/* 인코드 태스크 소유 */
static esp_h264_enc_handle_t s_enc;
static esp_h264_enc_param_hw_handle_t s_enc_param;
static int s_enc_w, s_enc_h;

esp_err_t video_dec_init(void)
{
    jpeg_decode_engine_cfg_t jcfg = {
        .intr_priority = 0,
        .timeout_ms = 200,
    };
    ESP_RETURN_ON_ERROR(jpeg_new_decoder_engine(&jcfg, &s_jpeg), TAG, "JPEG 디코더 생성 실패");

    ppa_client_config_t pcfg = {
        .oper_type = PPA_OPERATION_SRM,
        .max_pending_trans_num = 1,
    };
    ESP_RETURN_ON_ERROR(ppa_register_client(&pcfg, &s_ppa), TAG, "PPA 등록 실패");
    return ESP_OK;
}

static void *alloc_dec_buf(size_t want, size_t *got)
{
    jpeg_decode_memory_alloc_cfg_t mc = {
        .buffer_direction = JPEG_DEC_ALLOC_OUTPUT_BUFFER,
    };
    return jpeg_alloc_decoder_mem(want, &mc, got);
}

uint8_t *video_alloc_yuv(int w, int h, size_t *cap)
{
    return alloc_dec_buf((size_t)w * h * 3 / 2, cap);
}

void video_free_yuv(uint8_t *buf)
{
    heap_caps_free(buf);
}

esp_err_t video_dec_probe(const uint8_t *jpeg, size_t len, int *enc_w, int *enc_h)
{
    jpeg_decode_picture_info_t info;
    if (jpeg_decoder_get_info(jpeg, len, &info) != ESP_OK || info.width < 80 || info.height < 80) {
        return ESP_ERR_INVALID_ARG;
    }
    if (info.width > 1920) {
        ESP_LOGE(TAG, "가로 %d 는 H.264 하드웨어 인코더 한계(1920)를 넘습니다", (int)info.width);
        return ESP_ERR_NOT_SUPPORTED;
    }
    *enc_w = ALIGN16_DOWN((int)info.width);
    *enc_h = ALIGN16_DOWN((int)info.height);
    return ESP_OK;
}

/* 4:2:0 이 아니거나 16 배수가 아닐 때: RGB565 로 디코드 → PPA 로 YUV420 + 크롭 */
static esp_err_t decode_via_ppa(const uint8_t *jpeg, size_t len, const jpeg_decode_picture_info_t *info,
                                uint8_t *yuv, size_t yuv_cap, int enc_w, int enc_h)
{
    /* 디코더 출력은 MCU 크기에 맞춰 늘어난다. 4:2:0=16x16, 4:2:2=16x8, 4:4:4/흑백=8x8 */
    int mcu_x = 8, mcu_y = 8;
    if (info->sample_method == JPEG_DOWN_SAMPLING_YUV420) {
        mcu_x = 16;
        mcu_y = 16;
    } else if (info->sample_method == JPEG_DOWN_SAMPLING_YUV422) {
        mcu_x = 16;
    }
    int pw = ALIGN_UP_TO((int)info->width, mcu_x);
    int ph = ALIGN_UP_TO((int)info->height, mcu_y);
    size_t need = (size_t)pw * ph * 2;
    if (s_rgb_size < need) {
        heap_caps_free(s_rgb);
        s_rgb = alloc_dec_buf(need, &s_rgb_size);
        if (s_rgb == NULL) {
            s_rgb_size = 0;
            return ESP_ERR_NO_MEM;
        }
    }

    jpeg_decode_cfg_t dcfg = {
        .output_format = JPEG_DECODE_OUT_FORMAT_RGB565,
        .rgb_order = JPEG_DEC_RGB_ELEMENT_ORDER_BGR,   /* 리틀엔디언 RGB565 = PPA 기본 입력 */
        .conv_std = JPEG_YUV_RGB_CONV_STD_BT601,
    };
    uint32_t out_size = 0;
    esp_err_t err = jpeg_decoder_process(s_jpeg, &dcfg, jpeg, len, s_rgb, s_rgb_size, &out_size);
    if (err != ESP_OK) {
        return err;
    }

    ppa_srm_oper_config_t srm = {
        .in = {
            .buffer = s_rgb,
            .pic_w = pw,
            .pic_h = ph,
            .block_w = enc_w,
            .block_h = enc_h,
            /* 16 배수로 자르면서 생기는 여백은 가운데 정렬 (짝수여야 함) */
            .block_offset_x = ((info->width - enc_w) / 2) & ~1u,
            .block_offset_y = ((info->height - enc_h) / 2) & ~1u,
            .srm_cm = PPA_SRM_COLOR_MODE_RGB565,
        },
        .out = {
            .buffer = yuv,
            .buffer_size = yuv_cap,
            .pic_w = enc_w,
            .pic_h = enc_h,
            .srm_cm = PPA_SRM_COLOR_MODE_YUV420,
            .yuv_range = PPA_COLOR_RANGE_LIMIT,
            .yuv_std = PPA_COLOR_CONV_STD_RGB_YUV_BT601,
        },
        .rotation_angle = PPA_SRM_ROTATION_ANGLE_0,
        .scale_x = 1.0f,
        .scale_y = 1.0f,
        .mode = PPA_TRANS_MODE_BLOCKING,
    };
    return ppa_do_scale_rotate_mirror(s_ppa, &srm);
}

esp_err_t video_dec_to_yuv(const uint8_t *jpeg, size_t len, uint8_t *yuv, size_t yuv_cap,
                           int enc_w, int enc_h, bool *ppa_path)
{
    jpeg_decode_picture_info_t info;
    if (jpeg_decoder_get_info(jpeg, len, &info) != ESP_OK) {
        return ESP_ERR_INVALID_ARG;
    }
    bool direct = info.sample_method == JPEG_DOWN_SAMPLING_YUV420 &&
                  enc_w == (int)info.width && enc_h == (int)info.height;
    esp_err_t err;
    if (direct) {
        jpeg_decode_cfg_t dcfg = {
            .output_format = JPEG_DECODE_OUT_FORMAT_YUV420,
        };
        uint32_t out_size = 0;
        err = jpeg_decoder_process(s_jpeg, &dcfg, jpeg, len, yuv, yuv_cap, &out_size);
    } else {
        err = decode_via_ppa(jpeg, len, &info, yuv, yuv_cap, enc_w, enc_h);
    }
    if (s_path_logged != (int)direct) {
        s_path_logged = (int)direct;
        ESP_LOGI(TAG, "%s 경로 사용 (MJPEG %dx%d)", direct ? "JPEG→YUV420 직접" : "JPEG→RGB565→PPA",
                 (int)info.width, (int)info.height);
    }
    *ppa_path = !direct;
    return err;
}

esp_err_t video_h264_open(int w, int h, uint8_t fps, uint32_t bitrate_bps, uint8_t gop)
{
    video_h264_close();
    esp_h264_enc_cfg_hw_t cfg = {
        .pic_type = ESP_H264_RAW_FMT_O_UYY_E_VYY,
        .gop = gop,
        .fps = fps,
        .res = { .width = (uint16_t)w, .height = (uint16_t)h },
        .rc = {
            .bitrate = bitrate_bps,
            /* qp_min 을 너무 낮추면 키프레임이 커져서 WiFi 로 한꺼번에 못 보낸다 */
            .qp_min = 26,
            .qp_max = 42,
        },
    };
    if (esp_h264_enc_hw_new(&cfg, &s_enc) != ESP_H264_ERR_OK ||
        esp_h264_enc_hw_get_param_hd(s_enc, &s_enc_param) != ESP_H264_ERR_OK ||
        esp_h264_enc_open(s_enc) != ESP_H264_ERR_OK) {
        ESP_LOGE(TAG, "H.264 인코더 생성 실패 (%dx%d)", w, h);
        video_h264_close();
        return ESP_FAIL;
    }
    s_enc_w = w;
    s_enc_h = h;
    ESP_LOGI(TAG, "H.264 인코더 시작 %dx%d %dfps %lukbps GOP %d", w, h, fps,
             (unsigned long)(bitrate_bps / 1000), gop);
    return ESP_OK;
}

void video_h264_close(void)
{
    if (s_enc) {
        esp_h264_enc_close(s_enc);
        esp_h264_enc_del(s_enc);
    }
    s_enc = NULL;
    s_enc_param = NULL;
    s_enc_w = s_enc_h = 0;
}

bool video_h264_is_open(int w, int h)
{
    return s_enc && s_enc_w == w && s_enc_h == h;
}

esp_err_t video_h264_encode(const uint8_t *yuv, uint32_t yuv_len, uint32_t pts_ms,
                            uint8_t *out, uint32_t out_cap, uint32_t *out_len, bool *keyframe)
{
    esp_h264_enc_in_frame_t in = {
        .raw_data = { .buffer = (uint8_t *)yuv, .len = yuv_len },
        .pts = pts_ms,
    };
    esp_h264_enc_out_frame_t o = {
        .raw_data = { .buffer = out, .len = out_cap },
    };
    esp_h264_err_t r = esp_h264_enc_process(s_enc, &in, &o);
    if (r != ESP_H264_ERR_OK) {
        /* 출력 버퍼 초과(OVERFLOW) 등. 인코더가 다음 프레임을 IDR 로 만들어 스스로 복구한다 */
        return r == ESP_H264_ERR_OVERFLOW ? ESP_ERR_INVALID_SIZE : ESP_FAIL;
    }
    *out_len = o.length;
    *keyframe = o.frame_type == ESP_H264_FRAME_TYPE_IDR || o.frame_type == ESP_H264_FRAME_TYPE_I;
    return ESP_OK;
}

void video_h264_force_idr(void)
{
    if (s_enc_param) {
        esp_h264_enc_force_idr(&s_enc_param->base);
    }
}

esp_err_t video_h264_set_bitrate(uint32_t bitrate_bps)
{
    if (s_enc_param == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    return esp_h264_enc_set_bitrate(&s_enc_param->base, bitrate_bps) == ESP_H264_ERR_OK ? ESP_OK : ESP_FAIL;
}
