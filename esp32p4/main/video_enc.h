#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "esp_err.h"

/* MJPEG 한 장 → (하드웨어 JPEG 디코더) → YUV420 → (하드웨어 H.264 인코더) → H.264 AU
 *
 * 카메라 MJPEG 가 4:2:0 이고 해상도가 16 의 배수면 디코더가 인코더 입력 포맷
 * (O_UYY_E_VYY) 으로 바로 뽑아 준다. 4:2:2 등 그 밖의 경우엔 RGB565 로 디코드한 뒤
 * PPA(픽셀 처리 가속기)로 YUV420 변환 + 16 배수 크롭을 한다. 둘 다 CPU 를 거의 안 쓴다. */

typedef void (*video_enc_out_cb_t)(const uint8_t *data, size_t len, bool keyframe, uint32_t pts_ms, void *ctx);

esp_err_t video_enc_init(uint8_t fps, uint32_t bitrate_bps, uint8_t gop);

/* 결과는 cb 로 바로 넘어간다(같은 태스크, 같은 호출 안에서). */
esp_err_t video_enc_process(const uint8_t *jpeg, size_t len, uint32_t pts_ms, video_enc_out_cb_t cb, void *ctx);

/* 다음 프레임을 IDR 로. 새 시청자가 붙었거나 브라우저가 PLI 를 보냈을 때 */
void video_enc_request_idr(void);

typedef struct {
    int      width;          /* 인코딩 해상도 (0 이면 아직 시작 전) */
    int      height;
    bool     ppa_path;       /* RGB565 + PPA 경로를 쓰는 중인지 */
    uint32_t frames;         /* 누적 인코딩 프레임 */
    uint32_t bytes;          /* 누적 출력 바이트 */
    uint32_t last_ms;        /* 마지막 프레임 처리 시간 (디코드+변환+인코드) */
    uint32_t errors;
} video_enc_stats_t;

void video_enc_get_stats(video_enc_stats_t *out);
