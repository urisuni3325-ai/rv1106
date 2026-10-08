#include <string.h>
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "esp_check.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include "driver/jpeg_decode.h"
#include "driver/ppa.h"
#include "esp_h264_enc_single_hw.h"
#include "esp_h264_alloc.h"
#include "video_enc.h"

static const char *TAG = "venc";

#define ALIGN16_DOWN(x) ((x) & ~15)
#define ALIGN_UP_TO(x, a) (((x) + (a) - 1) / (a) * (a))

static jpeg_decoder_handle_t s_jpeg;
static ppa_client_handle_t s_ppa;
static esp_h264_enc_handle_t s_enc;
static esp_h264_enc_param_hw_handle_t s_enc_param;

static uint8_t *s_yuv;       /* 인코더 입력 (O_UYY_E_VYY) */
static size_t s_yuv_size;
static uint8_t *s_rgb;       /* PPA 경로용 RGB565 디코드 결과 */
static size_t s_rgb_size;
static uint8_t *s_bits;      /* 인코더 출력 */
static uint32_t s_bits_size;

static int s_enc_w, s_enc_h;
static uint8_t s_fps, s_gop;
static uint32_t s_bitrate;
static volatile bool s_idr_req;
static video_enc_stats_t s_stats;
static int s_path_logged = -1;

esp_err_t video_enc_init(uint8_t fps, uint32_t bitrate_bps, uint8_t gop)
{
    s_fps = fps;
    s_bitrate = bitrate_bps;
    s_gop = gop;

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

static void free_buffers(void)
{
    if (s_enc) {
        esp_h264_enc_close(s_enc);
        esp_h264_enc_del(s_enc);
        s_enc = NULL;
        s_enc_param = NULL;
    }
    heap_caps_free(s_yuv);
    heap_caps_free(s_rgb);
    if (s_bits) {
        esp_h264_free(s_bits);
    }
    s_yuv = s_rgb = s_bits = NULL;
    s_yuv_size = s_rgb_size = s_bits_size = 0;
    s_enc_w = s_enc_h = 0;
}

static void *alloc_dec_buf(size_t want, size_t *got)
{
    jpeg_decode_memory_alloc_cfg_t mc = {
        .buffer_direction = JPEG_DEC_ALLOC_OUTPUT_BUFFER,
    };
    return jpeg_alloc_decoder_mem(want, &mc, got);
}

/* 해상도가 바뀌면(카메라 재연결 등) 인코더와 버퍼를 새로 만든다 */
static esp_err_t ensure_encoder(int w, int h)
{
    if (s_enc && w == s_enc_w && h == s_enc_h) {
        return ESP_OK;
    }
    free_buffers();

    size_t want = (size_t)w * h * 3 / 2;
    s_yuv = alloc_dec_buf(want, &s_yuv_size);
    s_bits = esp_h264_aligned_calloc(16, 1, want, &s_bits_size, ESP_H264_MEM_SPIRAM);
    if (s_yuv == NULL || s_bits == NULL) {
        ESP_LOGE(TAG, "%dx%d 버퍼 할당 실패", w, h);
        free_buffers();
        return ESP_ERR_NO_MEM;
    }

    esp_h264_enc_cfg_hw_t cfg = {
        .pic_type = ESP_H264_RAW_FMT_O_UYY_E_VYY,
        .gop = s_gop,
        .fps = s_fps,
        .res = { .width = (uint16_t)w, .height = (uint16_t)h },
        .rc = {
            .bitrate = s_bitrate,
            .qp_min = 22,
            .qp_max = 38,
        },
    };
    if (esp_h264_enc_hw_new(&cfg, &s_enc) != ESP_H264_ERR_OK ||
        esp_h264_enc_hw_get_param_hd(s_enc, &s_enc_param) != ESP_H264_ERR_OK ||
        esp_h264_enc_open(s_enc) != ESP_H264_ERR_OK) {
        ESP_LOGE(TAG, "H.264 인코더 생성 실패 (%dx%d)", w, h);
        free_buffers();
        return ESP_FAIL;
    }
    s_enc_w = w;
    s_enc_h = h;
    s_stats.width = w;
    s_stats.height = h;
    ESP_LOGI(TAG, "H.264 인코더 시작 %dx%d %dfps %lukbps GOP %d", w, h, s_fps,
             (unsigned long)(s_bitrate / 1000), s_gop);
    return ESP_OK;
}

/* 4:2:0 이 아니거나 16 배수가 아닐 때: RGB565 로 디코드 → PPA 로 YUV420 + 크롭 */
static esp_err_t decode_via_ppa(const uint8_t *jpeg, size_t len, const jpeg_decode_picture_info_t *info)
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
            .block_w = s_enc_w,
            .block_h = s_enc_h,
            /* 16 배수로 자르면서 생기는 여백은 가운데 정렬 (짝수여야 함) */
            .block_offset_x = ((info->width - s_enc_w) / 2) & ~1u,
            .block_offset_y = ((info->height - s_enc_h) / 2) & ~1u,
            .srm_cm = PPA_SRM_COLOR_MODE_RGB565,
        },
        .out = {
            .buffer = s_yuv,
            .buffer_size = s_yuv_size,
            .pic_w = s_enc_w,
            .pic_h = s_enc_h,
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

esp_err_t video_enc_process(const uint8_t *jpeg, size_t len, uint32_t pts_ms, video_enc_out_cb_t cb, void *ctx)
{
    int64_t t0 = esp_timer_get_time();
    jpeg_decode_picture_info_t info;
    if (jpeg_decoder_get_info(jpeg, len, &info) != ESP_OK || info.width < 80 || info.height < 80) {
        s_stats.errors++;
        return ESP_ERR_INVALID_ARG;
    }
    int w = ALIGN16_DOWN((int)info.width);
    int h = ALIGN16_DOWN((int)info.height);
    if (w > 1920) {
        /* 하드웨어 인코더 한계 */
        ESP_LOGE(TAG, "가로 %d 는 H.264 하드웨어 인코더 한계(1920)를 넘습니다", (int)info.width);
        return ESP_ERR_NOT_SUPPORTED;
    }
    esp_err_t err = ensure_encoder(w, h);
    if (err != ESP_OK) {
        return err;
    }

    bool direct = info.sample_method == JPEG_DOWN_SAMPLING_YUV420 && w == (int)info.width && h == (int)info.height;
    if (direct) {
        jpeg_decode_cfg_t dcfg = {
            .output_format = JPEG_DECODE_OUT_FORMAT_YUV420,
        };
        uint32_t out_size = 0;
        err = jpeg_decoder_process(s_jpeg, &dcfg, jpeg, len, s_yuv, s_yuv_size, &out_size);
    } else {
        err = decode_via_ppa(jpeg, len, &info);
    }
    if (s_path_logged != (int)direct) {
        s_path_logged = (int)direct;
        ESP_LOGI(TAG, "%s 경로 사용 (MJPEG %dx%d)", direct ? "JPEG→YUV420 직접" : "JPEG→RGB565→PPA",
                 (int)info.width, (int)info.height);
        s_stats.ppa_path = !direct;
    }
    if (err != ESP_OK) {
        s_stats.errors++;
        return err;
    }

    if (s_idr_req) {
        s_idr_req = false;
        esp_h264_enc_force_idr(&s_enc_param->base);
    }
    esp_h264_enc_in_frame_t in = {
        .raw_data = { .buffer = s_yuv, .len = (uint32_t)w * h * 3 / 2 },
        .pts = pts_ms,
    };
    esp_h264_enc_out_frame_t out = {
        .raw_data = { .buffer = s_bits, .len = s_bits_size },
    };
    if (esp_h264_enc_process(s_enc, &in, &out) != ESP_H264_ERR_OK) {
        s_stats.errors++;
        return ESP_FAIL;
    }
    s_stats.frames++;
    s_stats.bytes += out.length;
    s_stats.last_ms = (uint32_t)((esp_timer_get_time() - t0) / 1000);

    bool key = out.frame_type == ESP_H264_FRAME_TYPE_IDR || out.frame_type == ESP_H264_FRAME_TYPE_I;
    if (cb && out.length) {
        cb(out.raw_data.buffer, out.length, key, pts_ms, ctx);
    }
    return ESP_OK;
}

void video_enc_request_idr(void)
{
    s_idr_req = true;
}

void video_enc_get_stats(video_enc_stats_t *out)
{
    *out = s_stats;
}
