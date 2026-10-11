#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include "esp_h264_alloc.h"
#include "video_enc.h"
#include "webrtc.h"
#include "pipeline.h"

static const char *TAG = "pipeline";

#define YUV_SLOTS 2   /* 디코드 N+1 과 인코드 N 이 겹치려면 2개 */
#define OUT_SLOTS 3   /* 전송 대기 H.264 AU. 이보다 밀리면 인코딩 전에 건너뛴다 */

typedef struct {
    int      slot;
    uint32_t pts_ms;
} yuv_job_t;

typedef struct {
    int      slot;
    uint32_t len;
    uint32_t pts_ms;
    bool     key;
} out_job_t;

static QueueHandle_t s_in_q;        /* camera_frame_t, 최신 1장 */
static QueueHandle_t s_yuv_free_q;  /* int */
static QueueHandle_t s_yuv_ready_q; /* yuv_job_t */
static QueueHandle_t s_out_free_q;  /* int */
static QueueHandle_t s_out_ready_q; /* out_job_t */

static uint8_t *s_yuv[YUV_SLOTS];
static size_t s_yuv_cap;
static uint8_t *s_out[OUT_SLOTS];
static uint32_t s_out_cap;
static volatile int s_w, s_h;       /* 버퍼를 할당한 해상도 (디코드 태스크가 바꾼다) */

static uint8_t s_fps, s_gop;
static uint32_t s_max_bps, s_min_bps;
static volatile uint32_t s_target_bps;
static volatile bool s_idr_req;
static int64_t s_last_bitrate_change;

/* 통계 */
static volatile uint32_t s_dec_count, s_enc_count, s_sent_count, s_sent_bytes;
static volatile uint32_t s_drop_busy, s_drop_net, s_errors;
static volatile uint32_t s_dec_ms, s_enc_ms, s_send_ms;
static volatile bool s_ppa_path;

static uint32_t ema(uint32_t avg, int64_t sample_us)
{
    uint32_t ms = (uint32_t)(sample_us / 1000);
    return avg ? (avg * 7 + ms) / 8 : ms;
}

bool pipeline_on_camera_frame(const camera_frame_t *frame)
{
    /* 보는 사람이 없으면 디코드·인코드를 하지 않는다. 붙는 순간 IDR 부터 나간다 */
    if (!webrtc_is_streaming()) {
        return false;
    }
    if (xQueueSend(s_in_q, frame, 0) == pdTRUE) {
        return true;
    }
    /* 디코더가 아직 이전 프레임을 안 가져갔으면 그걸 버리고 최신 프레임으로 바꾼다 (지연 누적 방지) */
    camera_frame_t old;
    if (xQueueReceive(s_in_q, &old, 0) == pdTRUE) {
        camera_frame_release(&old);
        s_drop_busy++;
    }
    if (xQueueSend(s_in_q, frame, 0) == pdTRUE) {
        return true;
    }
    s_drop_busy++;
    return false;
}

static void free_buffers(void)
{
    for (int i = 0; i < YUV_SLOTS; i++) {
        video_free_yuv(s_yuv[i]);
        s_yuv[i] = NULL;
    }
    for (int i = 0; i < OUT_SLOTS; i++) {
        if (s_out[i]) {
            esp_h264_free(s_out[i]);
        }
        s_out[i] = NULL;
    }
}

/* 해상도가 바뀌면(카메라 재연결 등) 모든 버퍼가 돌아올 때까지 기다렸다가 다시 할당한다.
 * 디코드 태스크에서만 부른다 */
static esp_err_t reconfigure(int w, int h)
{
    for (int i = 0; i < 400; i++) {
        if (uxQueueMessagesWaiting(s_yuv_free_q) == YUV_SLOTS &&
            uxQueueMessagesWaiting(s_out_free_q) == OUT_SLOTS) {
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
    if (uxQueueMessagesWaiting(s_yuv_free_q) != YUV_SLOTS ||
        uxQueueMessagesWaiting(s_out_free_q) != OUT_SLOTS) {
        ESP_LOGE(TAG, "버퍼가 돌아오지 않아 해상도를 바꿀 수 없음");
        return ESP_ERR_TIMEOUT;
    }
    free_buffers();
    s_w = s_h = 0;
    for (int i = 0; i < YUV_SLOTS; i++) {
        s_yuv[i] = video_alloc_yuv(w, h, &s_yuv_cap);
    }
    /* 키프레임 한 장이 원본의 1/3 을 넘을 일은 없다. 넘으면 인코더가 OVERFLOW 후 IDR 로 복구 */
    uint32_t want = (uint32_t)w * h / 2;
    for (int i = 0; i < OUT_SLOTS; i++) {
        s_out[i] = esp_h264_aligned_calloc(16, 1, want, &s_out_cap, ESP_H264_MEM_SPIRAM);
    }
    for (int i = 0; i < YUV_SLOTS; i++) {
        if (s_yuv[i] == NULL) {
            goto nomem;
        }
    }
    for (int i = 0; i < OUT_SLOTS; i++) {
        if (s_out[i] == NULL) {
            goto nomem;
        }
    }
    s_w = w;
    s_h = h;
    ESP_LOGI(TAG, "버퍼 준비 %dx%d (YUV %d개, 출력 %d개 × %luKB)", w, h, YUV_SLOTS, OUT_SLOTS,
             (unsigned long)(s_out_cap / 1024));
    return ESP_OK;
nomem:
    ESP_LOGE(TAG, "%dx%d 버퍼 할당 실패 (PSRAM 부족)", w, h);
    free_buffers();
    return ESP_ERR_NO_MEM;
}

static void dec_task(void *arg)
{
    while (1) {
        /* YUV 슬롯을 먼저 잡고 나서 프레임을 받는다. 그래야 인코더가 밀려도
         * 카메라 쪽 큐에는 늘 최신 프레임이 남아 있다 */
        int slot;
        xQueueReceive(s_yuv_free_q, &slot, portMAX_DELAY);
        camera_frame_t f;
        xQueueReceive(s_in_q, &f, portMAX_DELAY);

        int w, h;
        if (video_dec_probe(f.data, f.len, &w, &h) != ESP_OK) {
            camera_frame_release(&f);
            xQueueSend(s_yuv_free_q, &slot, 0);
            s_errors++;
            continue;
        }
        if (w != s_w || h != s_h) {
            xQueueSend(s_yuv_free_q, &slot, 0);
            esp_err_t err = reconfigure(w, h);
            xQueueReceive(s_yuv_free_q, &slot, portMAX_DELAY);
            if (err != ESP_OK) {
                camera_frame_release(&f);
                xQueueSend(s_yuv_free_q, &slot, 0);
                vTaskDelay(pdMS_TO_TICKS(500));
                continue;
            }
        }

        int64_t t0 = esp_timer_get_time();
        bool ppa = false;
        esp_err_t err = video_dec_to_yuv(f.data, f.len, s_yuv[slot], s_yuv_cap, w, h, &ppa);
        /* 디코드가 끝나면 MJPEG 는 필요 없으니 USB 버퍼를 바로 돌려준다 */
        camera_frame_release(&f);
        s_dec_ms = ema(s_dec_ms, esp_timer_get_time() - t0);
        s_ppa_path = ppa;
        if (err != ESP_OK) {
            /* 깨진 MJPEG (USB 전송 오류) 는 그냥 건너뛴다 */
            xQueueSend(s_yuv_free_q, &slot, 0);
            s_errors++;
            continue;
        }
        s_dec_count++;
        yuv_job_t job = { .slot = slot, .pts_ms = f.pts_ms };
        xQueueSend(s_yuv_ready_q, &job, portMAX_DELAY);
    }
}

static void enc_task(void *arg)
{
    uint32_t applied_bps = 0;
    while (1) {
        yuv_job_t job;
        xQueueReceive(s_yuv_ready_q, &job, portMAX_DELAY);
        int w = s_w, h = s_h;

        if (!video_h264_is_open(w, h)) {
            applied_bps = s_target_bps;
            if (video_h264_open(w, h, s_fps, applied_bps, s_gop) != ESP_OK) {
                xQueueSend(s_yuv_free_q, &job.slot, 0);
                s_errors++;
                continue;
            }
        }
        /* 전송이 밀려 빈 출력 버퍼가 없으면 인코딩하기 전에 건너뛴다.
         * 인코더가 이 프레임을 본 적이 없으니 다음 P 프레임 참조가 깨지지 않는다 */
        int out;
        if (xQueueReceive(s_out_free_q, &out, 0) != pdTRUE) {
            xQueueSend(s_yuv_free_q, &job.slot, 0);
            s_drop_net++;
            continue;
        }
        uint32_t target = s_target_bps;
        if (target != applied_bps && video_h264_set_bitrate(target) == ESP_OK) {
            applied_bps = target;
        }
        if (s_idr_req) {
            s_idr_req = false;
            video_h264_force_idr();
        }

        int64_t t0 = esp_timer_get_time();
        uint32_t len = 0;
        bool key = false;
        esp_err_t err = video_h264_encode(s_yuv[job.slot], (uint32_t)w * h * 3 / 2, job.pts_ms,
                                          s_out[out], s_out_cap, &len, &key);
        s_enc_ms = ema(s_enc_ms, esp_timer_get_time() - t0);
        xQueueSend(s_yuv_free_q, &job.slot, 0);
        if (err != ESP_OK || len == 0) {
            xQueueSend(s_out_free_q, &out, 0);
            s_errors++;
            continue;
        }
        s_enc_count++;
        out_job_t oj = { .slot = out, .len = len, .pts_ms = job.pts_ms, .key = key };
        xQueueSend(s_out_ready_q, &oj, portMAX_DELAY);
    }
}

static void send_task(void *arg)
{
    while (1) {
        out_job_t j;
        xQueueReceive(s_out_ready_q, &j, portMAX_DELAY);
        int64_t t0 = esp_timer_get_time();
        /* WiFi 송신 큐가 차면 esp_peer 가 이 안에서 잠깐씩 기다린다. 그래도 카메라/인코더는 안 멈춘다 */
        webrtc_send_video(s_out[j.slot], j.len, j.pts_ms);
        s_send_ms = ema(s_send_ms, esp_timer_get_time() - t0);
        s_sent_count++;
        s_sent_bytes += j.len;
        xQueueSend(s_out_free_q, &j.slot, 0);
    }
}

esp_err_t pipeline_start(uint8_t fps, uint32_t max_bitrate_bps, uint32_t min_bitrate_bps, uint8_t gop)
{
    s_fps = fps;
    s_gop = gop;
    s_max_bps = max_bitrate_bps;
    s_min_bps = min_bitrate_bps < max_bitrate_bps ? min_bitrate_bps : max_bitrate_bps;
    s_target_bps = max_bitrate_bps;

    ESP_ERROR_CHECK(video_dec_init());
    s_in_q = xQueueCreate(1, sizeof(camera_frame_t));
    s_yuv_free_q = xQueueCreate(YUV_SLOTS, sizeof(int));
    s_yuv_ready_q = xQueueCreate(YUV_SLOTS, sizeof(yuv_job_t));
    s_out_free_q = xQueueCreate(OUT_SLOTS, sizeof(int));
    s_out_ready_q = xQueueCreate(OUT_SLOTS, sizeof(out_job_t));
    for (int i = 0; i < YUV_SLOTS; i++) {
        xQueueSend(s_yuv_free_q, &i, 0);
    }
    for (int i = 0; i < OUT_SLOTS; i++) {
        xQueueSend(s_out_free_q, &i, 0);
    }
    /* 디코드·인코드는 하드웨어를 기다리는 시간이 대부분이라 CPU 를 거의 안 쓴다.
     * 전송(SRTP 암호화 + 소켓)은 CPU 를 쓰므로 다른 코어에 둔다 */
    xTaskCreatePinnedToCore(dec_task, "vdec", 4096, NULL, 12, NULL, 0);
    xTaskCreatePinnedToCore(enc_task, "venc", 4096, NULL, 11, NULL, 0);
    xTaskCreatePinnedToCore(send_task, "vsend", 6144, NULL, 10, NULL, portNUM_PROCESSORS - 1);
    return ESP_OK;
}

void pipeline_request_idr(void)
{
    s_idr_req = true;
}

void pipeline_reset_bitrate(void)
{
    s_target_bps = s_max_bps;
    s_last_bitrate_change = esp_timer_get_time();
}

static void set_target(uint32_t bps)
{
    if (bps < s_min_bps) {
        bps = s_min_bps;
    }
    if (bps > s_max_bps) {
        bps = s_max_bps;
    }
    uint32_t cur = s_target_bps;
    /* 5% 미만 변화는 무시 (인코더 레이트 컨트롤이 출렁이지 않게) */
    if (bps > cur * 105 / 100 || bps < cur * 95 / 100) {
        ESP_LOGI(TAG, "비트레이트 %lu → %lu kbps", (unsigned long)(cur / 1000), (unsigned long)(bps / 1000));
        s_target_bps = bps;
        s_last_bitrate_change = esp_timer_get_time();
    }
}

void pipeline_on_network_feedback(uint32_t est_bps, float loss)
{
    uint32_t cur = s_target_bps;
    int64_t now = esp_timer_get_time();
    if (loss >= 0.10f) {
        set_target(cur * 70 / 100);
    } else if (loss >= 0.03f) {
        set_target(cur * 90 / 100);
    } else if (est_bps > 0 && est_bps < cur) {
        /* 받는 쪽 추정 대역폭보다 많이 보내면 곧 쌓이고 끊긴다 */
        set_target(est_bps * 85 / 100);
    } else if (now - s_last_bitrate_change > 2 * 1000 * 1000) {
        /* 2초 동안 문제 없으면 조금씩 다시 올린다 */
        uint32_t up = cur * 110 / 100;
        if (est_bps > 0 && up > est_bps) {
            up = est_bps;
        }
        set_target(up);
    }
}

void pipeline_get_stats(pipeline_stats_t *out)
{
    static int64_t last_t;
    static uint32_t last_dec, last_enc, last_sent, last_bytes;
    static float dec_fps, enc_fps, sent_fps, kbps;

    int64_t now = esp_timer_get_time();
    float dt = (now - last_t) / 1e6f;
    if (last_t != 0 && dt >= 0.5f) {
        dec_fps = (s_dec_count - last_dec) / dt;
        enc_fps = (s_enc_count - last_enc) / dt;
        sent_fps = (s_sent_count - last_sent) / dt;
        kbps = (s_sent_bytes - last_bytes) * 8 / dt / 1000;
    }
    if (last_t == 0 || dt >= 0.5f) {
        last_t = now;
        last_dec = s_dec_count;
        last_enc = s_enc_count;
        last_sent = s_sent_count;
        last_bytes = s_sent_bytes;
    }
    *out = (pipeline_stats_t) {
        .width = s_w,
        .height = s_h,
        .ppa_path = s_ppa_path,
        .dec_fps = dec_fps,
        .enc_fps = enc_fps,
        .sent_fps = sent_fps,
        .sent_kbps = kbps,
        .target_kbps = s_target_bps / 1000,
        .dec_ms = s_dec_ms,
        .enc_ms = s_enc_ms,
        .send_ms = s_send_ms,
        .drop_busy = s_drop_busy,
        .drop_net = s_drop_net,
        .errors = s_errors,
    };
}
