#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "camera.h"

/* 카메라 → WebRTC 영상 파이프라인
 *
 *   카메라 태스크 ─(최신 1장)→ 디코드 태스크 ─(YUV ×2)→ 인코드 태스크 ─(H.264 ×3)→ 전송 태스크 → esp_peer
 *
 * 단계마다 태스크를 나눠서
 *  - 하드웨어 JPEG 디코더와 H.264 인코더가 동시에 돈다 (프레임 N 인코드 중에 N+1 디코드)
 *  - WiFi 가 밀려 전송이 멈춰도 USB 수신과 인코딩은 멈추지 않는다
 *  - 뒤 단계가 밀리면 "인코딩 전에" 프레임을 건너뛴다. 인코딩 뒤에 버리면 다음 P 프레임들이
 *    참조를 잃어 화면이 깨지기 때문 */

esp_err_t pipeline_start(uint8_t fps, uint32_t max_bitrate_bps, uint32_t min_bitrate_bps, uint8_t gop);

/* camera_start 에 넘기는 프레임 콜백 */
bool pipeline_on_camera_frame(const camera_frame_t *frame);

/* 다음 프레임을 IDR 로 (새 시청자, PLI, 스냅샷 뒤) */
void pipeline_request_idr(void);

/* 브라우저가 알려 준 네트워크 상태로 비트레이트 조절
 * est_bps: 수신 측 추정 대역폭(REMB 또는 TWCC 수신률, 모르면 0), loss: 손실률 0~1 (모르면 음수) */
void pipeline_on_network_feedback(uint32_t est_bps, float loss);

/* 새 연결이 시작될 때 비트레이트를 최대치부터 다시 */
void pipeline_reset_bitrate(void);

typedef struct {
    int      width;
    int      height;
    bool     ppa_path;
    float    dec_fps;        /* 디코드된 프레임/초 */
    float    enc_fps;        /* 인코딩된 프레임/초 */
    float    sent_fps;       /* 전송된 프레임/초 */
    float    sent_kbps;      /* 실제 보낸 비트레이트 */
    uint32_t target_kbps;    /* 현재 인코더 목표 비트레이트 */
    uint32_t dec_ms;         /* 단계별 처리 시간 (이동 평균) */
    uint32_t enc_ms;
    uint32_t send_ms;
    uint32_t drop_busy;      /* 디코더가 바빠서 건너뛴 카메라 프레임 (누적) */
    uint32_t drop_net;       /* 전송이 밀려 인코딩 전에 건너뛴 프레임 (누적) */
    uint32_t errors;
} pipeline_stats_t;

void pipeline_get_stats(pipeline_stats_t *out);
