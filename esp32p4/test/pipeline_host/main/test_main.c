/* pipeline.c 동시성 테스트 (ESP-IDF linux 타깃, 보드 없이)
 *   cd esp32p4/test/pipeline_host && idf.py --preview set-target linux && idf.py build && ./build/pipe_test.elf
 * 하드웨어 함수(디코드/인코드/전송)를 sleep 하는 가짜로 바꿔서 단계 겹침·건너뛰기·버퍼 반환을 확인한다 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "pipeline.h"
#include "video_enc.h"
#include "webrtc.h"

#include <time.h>
int64_t esp_timer_get_time(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int64_t)ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}

/* ---- 가짜 하드웨어 ---- */
static int dec_ms = 10, enc_ms = 20, send_ms = 3, send_stall_ms = 0;
static volatile int held, max_held, released_bad;
static volatile int enc_open_w, enc_open_h, enc_opens, idr_count, bitrate_sets;
static volatile uint32_t last_bitrate;
static volatile int sent_frames;
static volatile uint32_t last_pts;
static volatile int pts_backwards;
static volatile bool streaming = true;
static int frame_w = 1280, frame_h = 720;

esp_err_t video_dec_init(void) { return ESP_OK; }
esp_err_t video_dec_probe(const uint8_t *jpeg, size_t len, int *w, int *h)
{
    /* jpeg 첫 4바이트에 크기를 넣어 둔다 */
    const int *d = (const int *)jpeg;
    *w = d[0]; *h = d[1];
    return (len >= 8) ? ESP_OK : ESP_FAIL;
}
uint8_t *video_alloc_yuv(int w, int h, size_t *cap) { *cap = (size_t)w * h * 3 / 2; return malloc(*cap); }
void video_free_yuv(uint8_t *b) { free(b); }
esp_err_t video_dec_to_yuv(const uint8_t *jpeg, size_t len, uint8_t *yuv, size_t cap, int w, int h, bool *ppa)
{
    const int *d = (const int *)jpeg;
    assert(d[0] == w && d[1] == h);
    assert(cap >= (size_t)w * h * 3 / 2);
    yuv[0] = (uint8_t)d[2];   /* 프레임 번호 */
    vTaskDelay(pdMS_TO_TICKS(dec_ms));
    *ppa = true;
    return ESP_OK;
}
esp_err_t video_h264_open(int w, int h, uint8_t fps, uint32_t br, uint8_t gop)
{ enc_open_w = w; enc_open_h = h; enc_opens++; return ESP_OK; }
void video_h264_close(void) {}
bool video_h264_is_open(int w, int h) { return enc_opens && enc_open_w == w && enc_open_h == h; }
esp_err_t video_h264_encode(const uint8_t *yuv, uint32_t yuv_len, uint32_t pts, uint8_t *out, uint32_t cap,
                            uint32_t *len, bool *key)
{
    assert(yuv_len == (uint32_t)enc_open_w * enc_open_h * 3 / 2);
    vTaskDelay(pdMS_TO_TICKS(enc_ms));
    out[0] = yuv[0];
    *len = 1000; *key = false;
    return ESP_OK;
}
void video_h264_force_idr(void) { idr_count++; }
esp_err_t video_h264_set_bitrate(uint32_t b) { bitrate_sets++; last_bitrate = b; return ESP_OK; }

bool webrtc_is_streaming(void) { return streaming; }
void webrtc_send_video(const uint8_t *d, size_t len, uint32_t pts)
{
    vTaskDelay(pdMS_TO_TICKS(send_ms + send_stall_ms));
    if (pts < last_pts) pts_backwards++;
    last_pts = pts;
    sent_frames++;
}

/* ---- 가짜 카메라 ---- */
typedef struct { int hdr[3]; int in_use; } fake_buf_t;
static fake_buf_t bufs[3];

void camera_frame_release(const camera_frame_t *f)
{
    fake_buf_t *b = (fake_buf_t *)f->priv;
    if (!b->in_use) released_bad++;
    b->in_use = 0;
    __atomic_sub_fetch(&held, 1, __ATOMIC_SEQ_CST);
}

static int cam_frames, cam_drops;
static void camera_sim(int fps, int seconds)
{
    int64_t end = esp_timer_get_time() + (int64_t)seconds * 1000000;
    while (esp_timer_get_time() < end) {
        fake_buf_t *b = NULL;
        for (int i = 0; i < 3; i++) if (!bufs[i].in_use) { b = &bufs[i]; break; }
        if (!b) { cam_drops++; vTaskDelay(pdMS_TO_TICKS(1000 / fps)); continue; }   /* USB 버퍼 부족 */
        b->in_use = 1;
        b->hdr[0] = frame_w; b->hdr[1] = frame_h; b->hdr[2] = cam_frames;
        camera_frame_t cf = { .data = (uint8_t *)b->hdr, .len = sizeof(b->hdr), .width = frame_w,
                              .height = frame_h, .pts_ms = (uint32_t)(esp_timer_get_time() / 1000), .priv = b };
        __atomic_add_fetch(&held, 1, __ATOMIC_SEQ_CST);
        if (held > max_held) max_held = held;
        if (!pipeline_on_camera_frame(&cf)) {
            b->in_use = 0;
            __atomic_sub_fetch(&held, 1, __ATOMIC_SEQ_CST);
        }
        cam_frames++;
        vTaskDelay(pdMS_TO_TICKS(1000 / fps));
    }
}

static void phase(const char *name, int fps, int secs, float min_sent_fps, float max_sent_fps)
{
    int s0 = sent_frames, c0 = cam_frames;
    pipeline_stats_t st0; pipeline_get_stats(&st0);
    camera_sim(fps, secs);
    vTaskDelay(pdMS_TO_TICKS(300));
    pipeline_stats_t st; pipeline_get_stats(&st);
    float sent_fps = (sent_frames - s0) / (float)secs;
    printf("[%s] cam=%d sent=%d (%.1f fps) busy=%lu net=%lu dec=%lums enc=%lums send=%lums held=%d max_held=%d target=%lukbps\n",
           name, cam_frames - c0, sent_frames - s0, sent_fps, (unsigned long)st.drop_busy, (unsigned long)st.drop_net,
           (unsigned long)st.dec_ms, (unsigned long)st.enc_ms, (unsigned long)st.send_ms, held, max_held,
           (unsigned long)st.target_kbps);
    assert(held == 0);              /* 모든 USB 프레임이 돌아왔다 */
    assert(released_bad == 0);      /* 두 번 돌려준 프레임 없음 */
    assert(sent_fps >= min_sent_fps && sent_fps <= max_sent_fps);
    assert(pts_backwards == 0);     /* 순서 뒤집힘 없음 */
}

void app_main(void)
{
    pipeline_start(30, 2000000, 500000, 60);

    /* 1) 디코드 10ms + 인코드 20ms: 직렬이면 33fps 근처가 한계지만 겹쳐서 30fps 를 거의 다 낸다 */
    phase("정상 30fps", 30, 3, 26, 35);

    /* 2) 디코드 25 + 인코드 25 = 50ms: 직렬이었다면 20fps, 파이프라인이면 ~30fps 에 가깝게 */
    /* 2) 디코드 28 + 인코드 28 = 56ms: 직렬이었다면 18fps 가 한계. 겹치면 ~30fps */
    dec_ms = 28; enc_ms = 28;
    camera_sim(30, 1);   /* 이동 평균이 새 값에 수렴하도록 */
    phase("무거운 변환", 30, 3, 25, 35);
    pipeline_stats_t h; pipeline_get_stats(&h);
    /* 직렬이면 1000/(dec+enc) fps 가 한계 — 그보다 확실히 많이 보냈으면 두 단계가 겹친 것 */
    assert(h.dec_ms + h.enc_ms >= 40);
    dec_ms = 10; enc_ms = 20;

    /* 3) WiFi 가 막혀 전송이 프레임당 100ms: 인코딩 전에 건너뛰고(drop_net), 카메라·USB 는 계속 돈다 */
    pipeline_stats_t a; pipeline_get_stats(&a);
    send_stall_ms = 97;
    phase("전송 막힘", 30, 3, 7, 11);
    pipeline_stats_t b; pipeline_get_stats(&b);
    assert(b.drop_net > a.drop_net);
    send_stall_ms = 0;
    phase("전송 회복", 30, 2, 26, 35);

    /* 4) 카메라 재연결로 해상도가 바뀌어도 버퍼를 다시 잡고 계속 간다 */
    frame_w = 1920; frame_h = 1072;
    int opens = enc_opens;
    phase("해상도 변경", 30, 3, 20, 35);
    assert(enc_opens == opens + 1 && enc_open_w == 1920 && enc_open_h == 1072);

    /* 5) 시청자 없으면 프레임을 안 잡는다 */
    streaming = false;
    phase("시청자 없음", 30, 1, 0, 0.5);
    streaming = true;

    /* 6) 비트레이트 적응: 손실 10% → 내림, 최소값 아래로는 안 감, 회복 후 다시 올림 */
    for (int i = 0; i < 20; i++) pipeline_on_network_feedback(0, 0.2f);
    pipeline_stats_t c; pipeline_get_stats(&c);
    printf("손실 20%% 반복 후 target=%lu\n", (unsigned long)c.target_kbps);
    assert(c.target_kbps == 500);
    pipeline_on_network_feedback(800000, 0.0f);   /* REMB 800k > 현재 → 2초 지나기 전엔 유지 */
    vTaskDelay(pdMS_TO_TICKS(2100));
    pipeline_on_network_feedback(800000, 0.0f);
    pipeline_get_stats(&c);
    printf("2초 무손실 후 target=%lu\n", (unsigned long)c.target_kbps);
    assert(c.target_kbps == 550);
    pipeline_on_network_feedback(300000, 0.0f);   /* 추정 대역폭이 더 낮으면 바로 맞춘다(최소값까지) */
    pipeline_get_stats(&c);
    assert(c.target_kbps == 500);
    camera_sim(30, 1);   /* 인코더에 실제로 적용됐는지 */
    vTaskDelay(pdMS_TO_TICKS(200));
    assert(last_bitrate == 500000);
    pipeline_reset_bitrate();
    pipeline_get_stats(&c);
    assert(c.target_kbps == 2000);

    /* 7) IDR 요청은 다음 인코드에서 한 번 */
    int idr0 = idr_count;
    pipeline_request_idr();
    camera_sim(30, 1);
    vTaskDelay(pdMS_TO_TICKS(200));
    assert(idr_count == idr0 + 1);

    printf("PIPELINE TEST OK\n");
    fflush(stdout);
    exit(0);
}
