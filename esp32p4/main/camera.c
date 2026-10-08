#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include "usb/usb_host.h"
#include "usb/uvc_host.h"
#include "jpeg_util.h"
#include "camera.h"

static const char *TAG = "camera";

#define USB_HOST_PRIO   15
#define MAX_FORMATS     32
#define BIT_DEV_FOUND   BIT0

#define FPS_FROM_INTERVAL(i) ((i) ? 10000000.0f / (float)(i) : 0.0f)

typedef struct {
    bool hires;
    camera_snapshot_t *out;
    esp_err_t result;
} snap_req_t;

static camera_frame_cb_t s_frame_cb;
static camera_resume_cb_t s_resume_cb;
static int s_want_w, s_want_h, s_want_fps;

static EventGroupHandle_t s_events;
static QueueHandle_t s_frame_q;
static volatile bool s_disconnected;
static uint8_t s_dev_addr;
static uint8_t s_stream_idx;
static uvc_host_frame_info_t s_formats[MAX_FORMATS];
static size_t s_format_num;

static SemaphoreHandle_t s_snap_lock;   /* 스냅샷은 한 번에 하나 */
static SemaphoreHandle_t s_snap_done;
static snap_req_t *volatile s_snap_req;

static camera_status_t s_status;
static uint32_t s_fps_frames;
static int64_t s_fps_t0;

static bool on_frame(const uvc_host_frame_t *frame, void *user_ctx)
{
    if (xQueueSend(s_frame_q, &frame, 0) != pdTRUE) {
        s_status.dropped++;
        return true;   /* 큐가 꽉 찼으면 바로 돌려준다 */
    }
    return false;      /* 나중에 uvc_host_frame_return() */
}

static void on_stream_event(const uvc_host_stream_event_data_t *event, void *user_ctx)
{
    switch (event->type) {
    case UVC_HOST_TRANSFER_ERROR:
        ESP_LOGW(TAG, "USB 전송 오류 %d", event->transfer_error.error);
        break;
    case UVC_HOST_DEVICE_DISCONNECTED: {
        ESP_LOGW(TAG, "카메라가 분리됨");
        s_disconnected = true;
        const uvc_host_frame_t *wake = NULL;
        xQueueSend(s_frame_q, &wake, 0);
        break;
    }
    case UVC_HOST_FRAME_BUFFER_OVERFLOW:
        /* 프레임이 버퍼보다 큼 — 주로 고해상도 스냅샷에서 */
        s_status.dropped++;
        ESP_LOGW(TAG, "프레임이 버퍼(%dKB)보다 큼", CONFIG_APP_UVC_FRAME_BUF_KB);
        break;
    case UVC_HOST_FRAME_BUFFER_UNDERFLOW:
        s_status.dropped++;
        break;
    default:
        break;
    }
}

static void on_driver_event(const uvc_host_driver_event_data_t *event, void *user_ctx)
{
    if (event->type != UVC_HOST_DRIVER_EVENT_DEVICE_CONNECTED) {
        return;
    }
    if (s_status.connected) {
        ESP_LOGW(TAG, "카메라가 이미 연결돼 있어 추가 장치(addr %d)는 무시", event->device_connected.dev_addr);
        return;
    }
    s_dev_addr = event->device_connected.dev_addr;
    s_stream_idx = event->device_connected.uvc_stream_index;
    s_format_num = MAX_FORMATS;
    uvc_host_get_frame_list(s_dev_addr, s_stream_idx, (uvc_host_frame_info_t (*)[])s_formats, &s_format_num);
    xEventGroupSetBits(s_events, BIT_DEV_FOUND);
}

static void usb_lib_task(void *arg)
{
    while (1) {
        uint32_t flags;
        usb_host_lib_handle_events(portMAX_DELAY, &flags);
        if (flags & USB_HOST_LIB_EVENT_FLAGS_NO_CLIENTS) {
            usb_host_device_free_all();
        }
    }
}

static const char *fmt_name(enum uvc_host_stream_format f)
{
    switch (f) {
    case UVC_VS_FORMAT_MJPEG:
        return "MJPEG";
    case UVC_VS_FORMAT_YUY2:
        return "YUY2";
    case UVC_VS_FORMAT_H264:
        return "H264";
    case UVC_VS_FORMAT_H265:
        return "H265";
    default:
        return "?";
    }
}

/* 실시간용: 원하는 해상도가 있으면 그것, 없으면 하드웨어 인코더가 받을 수 있는
 * (가로 1920 이하) MJPEG 중 면적이 가장 가까운 것 */
static const uvc_host_frame_info_t *pick_stream_format(void)
{
    const uvc_host_frame_info_t *best = NULL;
    long want = (long)s_want_w * s_want_h;
    long best_diff = 0;
    for (size_t i = 0; i < s_format_num; i++) {
        const uvc_host_frame_info_t *f = &s_formats[i];
        if (f->format != UVC_VS_FORMAT_MJPEG || f->h_res > 1920) {
            continue;
        }
        if ((int)f->h_res == s_want_w && (int)f->v_res == s_want_h) {
            return f;
        }
        long diff = labs((long)f->h_res * (long)f->v_res - want);
        if (best == NULL || diff < best_diff) {
            best = f;
            best_diff = diff;
        }
    }
    return best;
}

static const uvc_host_frame_info_t *pick_max_format(void)
{
    const uvc_host_frame_info_t *best = NULL;
    for (size_t i = 0; i < s_format_num; i++) {
        const uvc_host_frame_info_t *f = &s_formats[i];
        if (f->format != UVC_VS_FORMAT_MJPEG) {
            continue;
        }
        if (best == NULL || f->h_res * f->v_res > best->h_res * best->v_res) {
            best = f;
        }
    }
    return best;
}

/* 원하는 fps 를 그 해상도가 지원하는지 보고, 아니면 장치 기본값(0) */
static float pick_fps(const uvc_host_frame_info_t *f, int want)
{
    if (f->interval_type == 0) {
        float lo = FPS_FROM_INTERVAL(f->interval_max);
        float hi = FPS_FROM_INTERVAL(f->interval_min);
        return (want >= lo - 0.5f && want <= hi + 0.5f) ? (float)want : 0.0f;
    }
    for (int i = 0; i < f->interval_type && i < CONFIG_UVC_INTERVAL_ARRAY_SIZE; i++) {
        if (fabsf(FPS_FROM_INTERVAL(f->interval[i]) - want) < 0.5f) {
            return (float)want;
        }
    }
    return 0.0f;
}

static esp_err_t copy_snapshot(const uvc_host_frame_t *frame, camera_snapshot_t *out)
{
    jpeg_header_t hdr;
    if (jpeg_parse_header(frame->data, frame->data_len, &hdr) != 0) {
        return ESP_ERR_INVALID_RESPONSE;
    }
    size_t cap = frame->data_len + JPEG_STD_DHT_SIZE;
    uint8_t *buf = heap_caps_malloc(cap, MALLOC_CAP_SPIRAM);
    if (buf == NULL) {
        return ESP_ERR_NO_MEM;
    }
    size_t len = jpeg_insert_std_dht(frame->data, frame->data_len, buf, cap);
    if (len == 0) {
        free(buf);
        return ESP_ERR_INVALID_RESPONSE;
    }
    out->data = buf;
    out->len = len;
    out->width = hdr.width;
    out->height = hdr.height;
    return ESP_OK;
}

/* 요청을 카메라 태스크가 가져간다. 가져간 뒤에는 HTTP 쪽이 취소할 수 없고 끝까지 기다린다 */
static snap_req_t *claim_snapshot(void)
{
    return __atomic_exchange_n((snap_req_t **)&s_snap_req, NULL, __ATOMIC_SEQ_CST);
}

static void finish_snapshot(snap_req_t *req, esp_err_t result)
{
    req->result = result;
    xSemaphoreGive(s_snap_done);
}

static void drain_and_return(uvc_host_stream_hdl_t stream)
{
    uvc_host_frame_t *f;
    while (xQueueReceive(s_frame_q, &f, 0) == pdTRUE) {
        if (f) {
            uvc_host_frame_return(stream, f);
        }
    }
}

/* 최대 해상도로 바꿔서 한 장 찍고 실시간 해상도로 복귀 */
static esp_err_t take_hires(uvc_host_stream_hdl_t stream, const uvc_host_frame_info_t *hi,
                            const uvc_host_stream_format_t *stream_fmt, camera_snapshot_t *out)
{
    ESP_LOGI(TAG, "고해상도 스냅샷: %ux%u 로 전환", hi->h_res, hi->v_res);
    drain_and_return(stream);
    uvc_host_stream_format_t hf = {
        .h_res = hi->h_res,
        .v_res = hi->v_res,
        .fps = 0,
        .format = UVC_VS_FORMAT_MJPEG,
    };
    esp_err_t err = uvc_host_stream_format_select(stream, &hf);
    if (err == ESP_OK) {
        err = ESP_ERR_TIMEOUT;
        int skip = CONFIG_APP_SNAPSHOT_SKIP_FRAMES;
        int64_t deadline = esp_timer_get_time() + 5 * 1000 * 1000;
        while (esp_timer_get_time() < deadline && !s_disconnected) {
            uvc_host_frame_t *f;
            if (xQueueReceive(s_frame_q, &f, pdMS_TO_TICKS(500)) != pdTRUE || f == NULL) {
                continue;
            }
            /* 전환 직전에 들어온 실시간 해상도 프레임은 건너뛴다 */
            bool match = f->vs_format.h_res == hi->h_res && f->vs_format.v_res == hi->v_res;
            if (match && skip-- <= 0) {
                err = copy_snapshot(f, out);
                uvc_host_frame_return(stream, f);
                if (err == ESP_OK) {
                    break;
                }
                continue;
            }
            uvc_host_frame_return(stream, f);
        }
    } else {
        ESP_LOGE(TAG, "고해상도 전환 실패: %s", esp_err_to_name(err));
    }

    if (!s_disconnected) {
        drain_and_return(stream);
        uvc_host_stream_format_t back = *stream_fmt;
        if (uvc_host_stream_format_select(stream, &back) != ESP_OK) {
            ESP_LOGE(TAG, "실시간 해상도 복귀 실패");
        }
        if (s_resume_cb) {
            s_resume_cb();
        }
    }
    return err;
}

static void update_fps(void)
{
    s_status.frames++;
    s_fps_frames++;
    int64_t now = esp_timer_get_time();
    if (now - s_fps_t0 >= 1000000) {
        s_status.fps = s_fps_frames * 1000000.0f / (float)(now - s_fps_t0);
        s_fps_frames = 0;
        s_fps_t0 = now;
    }
}

/* 스트림을 열어 분리될 때까지 돌린다. 열기에 실패하면 false */
static bool run_stream(void)
{
    ESP_LOGI(TAG, "카메라 연결됨 (addr %d) — 지원 포맷 %d개:", s_dev_addr, (int)s_format_num);
    for (size_t i = 0; i < s_format_num; i++) {
        const uvc_host_frame_info_t *f = &s_formats[i];
        ESP_LOGI(TAG, "  %-5s %4ux%-4u 기본 %.1ffps", fmt_name(f->format), f->h_res, f->v_res,
                 FPS_FROM_INTERVAL(f->default_interval));
    }
    const uvc_host_frame_info_t *sf = pick_stream_format();
    const uvc_host_frame_info_t *hf = pick_max_format();
    if (sf == NULL) {
        ESP_LOGE(TAG, "MJPEG 포맷을 지원하지 않는 카메라입니다");
        return true;
    }
    if ((int)sf->h_res != s_want_w || (int)sf->v_res != s_want_h) {
        ESP_LOGW(TAG, "%dx%d 미지원 → %ux%u 사용", s_want_w, s_want_h, sf->h_res, sf->v_res);
    }

    uvc_host_stream_config_t cfg = {
        .event_cb = on_stream_event,
        .frame_cb = on_frame,
        .user_ctx = NULL,
        .usb = {
            .dev_addr = s_dev_addr,
            .vid = UVC_HOST_ANY_VID,
            .pid = UVC_HOST_ANY_PID,
            .uvc_stream_index = s_stream_idx,
        },
        .vs_format = {
            .h_res = sf->h_res,
            .v_res = sf->v_res,
            .fps = pick_fps(sf, s_want_fps),
            .format = UVC_VS_FORMAT_MJPEG,
        },
        .advanced = {
            .number_of_frame_buffers = CONFIG_APP_UVC_FRAME_BUF_COUNT,
            .frame_size = CONFIG_APP_UVC_FRAME_BUF_KB * 1024,
            .frame_heap_caps = MALLOC_CAP_SPIRAM,
            .number_of_urbs = 4,
            .urb_size = 0,
        },
    };
    uvc_host_stream_hdl_t stream = NULL;
    esp_err_t err = uvc_host_stream_open(&cfg, pdMS_TO_TICKS(5000), &stream);
    if (err != ESP_OK && cfg.vs_format.fps != 0) {
        cfg.vs_format.fps = 0;
        err = uvc_host_stream_open(&cfg, pdMS_TO_TICKS(5000), &stream);
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "스트림 열기 실패: %s", esp_err_to_name(err));
        return false;
    }
    uvc_host_stream_format_t cur = { 0 };
    uvc_host_stream_format_get(stream, &cur);
    s_status.connected = true;
    s_status.width = cur.h_res;
    s_status.height = cur.v_res;
    s_status.max_width = hf ? (int)hf->h_res : (int)cur.h_res;
    s_status.max_height = hf ? (int)hf->v_res : (int)cur.v_res;
    ESP_LOGI(TAG, "실시간 MJPEG %ux%u @ %.0ffps, 스냅샷 최대 %dx%d", cur.h_res, cur.v_res, cur.fps,
             s_status.max_width, s_status.max_height);

    s_fps_t0 = esp_timer_get_time();
    s_fps_frames = 0;
    if (uvc_host_stream_start(stream) != ESP_OK) {
        ESP_LOGE(TAG, "스트림 시작 실패");
        goto out;
    }

    /* 카메라 태스크가 가져간 스냅샷 요청 (고해상도 실패 후 실시간 해상도로 대신 찍을 때도 여기 남는다) */
    snap_req_t *claimed = NULL;
    while (!s_disconnected) {
        uvc_host_frame_t *f = NULL;
        if (xQueueReceive(s_frame_q, &f, pdMS_TO_TICKS(2000)) != pdTRUE) {
            ESP_LOGW(TAG, "2초 동안 프레임이 안 옴");
            s_status.fps = 0;
            /* 가져간 요청은 HTTP 쪽이 끝까지 기다리므로 프레임이 끊기면 실패로 돌려준다 */
            if (claimed && !claimed->hires) {
                finish_snapshot(claimed, ESP_ERR_TIMEOUT);
                claimed = NULL;
            }
        }
        if (s_disconnected) {
            break;
        }
        if (claimed == NULL && s_snap_req != NULL) {
            claimed = claim_snapshot();
        }
        if (f) {
            bool is_stream = f->vs_format.h_res == cur.h_res && f->vs_format.v_res == cur.v_res;
            if (is_stream && claimed && !claimed->hires) {
                finish_snapshot(claimed, copy_snapshot(f, claimed->out));
                claimed = NULL;
            }
            if (is_stream) {
                update_fps();
                if (s_frame_cb) {
                    s_frame_cb(f->data, f->data_len, cur.h_res, cur.v_res,
                               (uint32_t)(esp_timer_get_time() / 1000));
                }
            }
            uvc_host_frame_return(stream, f);
        }

        if (claimed && claimed->hires) {
            if (hf && (hf->h_res != cur.h_res || hf->v_res != cur.v_res)) {
                esp_err_t r = take_hires(stream, hf, &cur, claimed->out);
                s_fps_t0 = esp_timer_get_time();
                s_fps_frames = 0;
                if (r == ESP_OK || s_disconnected) {
                    finish_snapshot(claimed, r);
                    claimed = NULL;
                } else {
                    /* 고해상도가 안 되면(버퍼 부족 등) 다음 실시간 프레임으로 대신 찍는다 */
                    claimed->hires = false;
                    claimed->out->fallback = true;
                }
            } else {
                /* 실시간 해상도가 이미 최대 */
                claimed->hires = false;
            }
        }
    }

    if (claimed) {
        finish_snapshot(claimed, ESP_ERR_INVALID_STATE);
    }
out:
    /* 분리된 뒤에는 프레임 버퍼가 close 에서 한꺼번에 해제되니 돌려주지 않는다 */
    xQueueReset(s_frame_q);
    uvc_host_stream_close(stream);
    s_status.connected = false;
    s_status.fps = 0;
    return true;
}

static void camera_task(void *arg)
{
    while (1) {
        ESP_LOGI(TAG, "USB 카메라를 기다리는 중...");
        xEventGroupWaitBits(s_events, BIT_DEV_FOUND, pdTRUE, pdTRUE, portMAX_DELAY);
        s_disconnected = false;
        /* 꽂은 직전에는 카메라가 아직 준비 안 돼 열기가 실패하기도 해서 몇 번 다시 시도한다 */
        for (int i = 0; i < 3 && !s_disconnected; i++) {
            if (run_stream()) {
                break;
            }
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
}

esp_err_t camera_start(int width, int height, int fps, camera_frame_cb_t frame_cb, camera_resume_cb_t resume_cb)
{
    s_want_w = width;
    s_want_h = height;
    s_want_fps = fps;
    s_frame_cb = frame_cb;
    s_resume_cb = resume_cb;
    s_events = xEventGroupCreate();
    s_frame_q = xQueueCreate(CONFIG_APP_UVC_FRAME_BUF_COUNT + 1, sizeof(uvc_host_frame_t *));
    s_snap_lock = xSemaphoreCreateMutex();
    s_snap_done = xSemaphoreCreateBinary();

    const usb_host_config_t host_cfg = {
        .skip_phy_setup = false,
        .intr_flags = ESP_INTR_FLAG_LOWMED,
    };
    ESP_ERROR_CHECK(usb_host_install(&host_cfg));
    xTaskCreatePinnedToCore(usb_lib_task, "usb_lib", 4096, NULL, USB_HOST_PRIO, NULL, tskNO_AFFINITY);

    const uvc_host_driver_config_t uvc_cfg = {
        .driver_task_stack_size = 6 * 1024,
        .driver_task_priority = USB_HOST_PRIO + 1,
        .xCoreID = tskNO_AFFINITY,
        .create_background_task = true,
        .event_cb = on_driver_event,
    };
    ESP_ERROR_CHECK(uvc_host_install(&uvc_cfg));
    /* 디코드·인코드를 이 태스크에서 동기 처리한다 (둘 다 하드웨어라 CPU 는 놀고 기다림) */
    xTaskCreatePinnedToCore(camera_task, "camera", 8192, NULL, USB_HOST_PRIO - 2, NULL, 0);
    return ESP_OK;
}

esp_err_t camera_snapshot(bool hires, camera_snapshot_t *out, int timeout_ms)
{
    memset(out, 0, sizeof(*out));
    if (!s_status.connected) {
        return ESP_ERR_INVALID_STATE;
    }
    if (xSemaphoreTake(s_snap_lock, pdMS_TO_TICKS(timeout_ms)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    snap_req_t req = { .hires = hires, .out = out, .result = ESP_FAIL };
    xSemaphoreTake(s_snap_done, 0);
    s_snap_req = &req;
    esp_err_t err;
    if (xSemaphoreTake(s_snap_done, pdMS_TO_TICKS(timeout_ms)) == pdTRUE) {
        err = req.result;
    } else {
        /* 카메라 태스크가 아직 안 집어 갔으면 취소. 이미 처리 중이면 끝날 때까지 기다린다 */
        snap_req_t *expected = &req;
        if (__atomic_compare_exchange_n((snap_req_t **)&s_snap_req, &expected, NULL, false,
                                        __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)) {
            err = ESP_ERR_TIMEOUT;
        } else {
            xSemaphoreTake(s_snap_done, portMAX_DELAY);
            err = req.result;
        }
    }
    if (err != ESP_OK) {
        camera_snapshot_free(out);
    }
    xSemaphoreGive(s_snap_lock);
    return err;
}

void camera_snapshot_free(camera_snapshot_t *snap)
{
    if (snap && snap->data) {
        free(snap->data);
        snap->data = NULL;
        snap->len = 0;
    }
}

void camera_get_status(camera_status_t *out)
{
    *out = s_status;
}
