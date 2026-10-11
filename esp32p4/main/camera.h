#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "esp_err.h"

/* USB UVC 카메라(MJPEG) 수신
 *
 * Arducam B0290(IMX298) 은 USB2.0 UVC 카메라라 드라이버 없이 붙는다.
 * ESP32-P4 의 USB OTG 2.0 HS 포트(보드의 USB-A 또는 OTG 단자)에 꽂는다. */

typedef struct {
    const uint8_t *data;   /* MJPEG 한 장 */
    size_t         len;
    int            width;
    int            height;
    uint32_t       pts_ms; /* 받은 시각 */
    void          *priv;
} camera_frame_t;

/* 실시간 해상도 프레임이 올 때마다 카메라 태스크에서 불린다.
 * true 를 돌려주면 프레임 버퍼를 가져간 것이고, 다 쓰면 camera_frame_release() 로 돌려줘야 한다.
 * (USB 버퍼는 몇 개뿐이라 오래 잡고 있으면 그만큼 카메라 프레임이 버려진다)
 * false 면 콜백이 끝나자마자 돌려준다. 콜백 안에서 오래 걸리는 일을 하면 안 된다. */
typedef bool (*camera_frame_cb_t)(const camera_frame_t *frame);

void camera_frame_release(const camera_frame_t *frame);

/* 고해상도 스냅샷 뒤 실시간 해상도로 돌아왔을 때 (인코더에 IDR 요청용) */
typedef void (*camera_resume_cb_t)(void);

esp_err_t camera_start(int width, int height, int fps, camera_frame_cb_t frame_cb, camera_resume_cb_t resume_cb);

typedef struct {
    uint8_t *data;   /* JPEG (표준 DHT 포함). camera_snapshot_free 로 해제 */
    size_t   len;
    int      width;
    int      height;
    bool     fallback; /* 고해상도를 요청했지만 실패해서 실시간 해상도로 찍힌 경우 */
} camera_snapshot_t;

/* hires=false: 다음 실시간 프레임 한 장 (즉시, 끊김 없음)
 * hires=true : 카메라 최대 해상도(IMX298 은 4656x3496)로 잠깐 바꿔서 한 장 찍고 복귀.
 *              전환하는 1~2초 동안 실시간 영상이 멈춘다. */
esp_err_t camera_snapshot(bool hires, camera_snapshot_t *out, int timeout_ms);
void camera_snapshot_free(camera_snapshot_t *snap);

typedef struct {
    bool  connected;
    int   width;          /* 실시간 MJPEG 해상도 */
    int   height;
    float fps;            /* 실제 들어오는 프레임레이트 (최근 1초) */
    int   max_width;      /* 스냅샷 최대 해상도 */
    int   max_height;
    uint32_t frames;
    uint32_t dropped;     /* 버퍼 부족·크기 초과로 버려진 프레임 */
} camera_status_t;

void camera_get_status(camera_status_t *out);
