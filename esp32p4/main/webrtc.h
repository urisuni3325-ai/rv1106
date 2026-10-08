#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "esp_err.h"

/* 시청자 1명용 WebRTC 송신기 (esp_peer 기반, 영상만 송신)
 *
 * 시그널링은 이 보드의 HTTP 서버가 직접 한다 — 외부 서버 불필요.
 *   1. 브라우저: GET  /api/offer   → 보드가 만든 SDP offer (ICE 후보 포함)
 *   2. 브라우저: POST /api/answer  ← 브라우저가 만든 SDP answer
 * 새 시청자가 offer 를 요청하면 이전 시청자는 끊긴다. */

esp_err_t webrtc_init(int width, int height, int fps);

/* 새 연결용 offer 를 만든다. 성공하면 *sdp 에 malloc 된 문자열(호출자가 free),
 * *session 에 이 연결의 번호. answer/hangup 은 이 번호가 맞을 때만 받는다
 * (이미 밀려난 이전 시청자의 늦은 요청이 새 시청자를 끊지 않도록) */
esp_err_t webrtc_create_offer(char **sdp, uint32_t *session, int timeout_ms);

/* 브라우저 answer 를 넘긴다 (sdp 는 내부에서 복사) */
esp_err_t webrtc_set_answer(uint32_t session, const char *sdp);

/* 브라우저가 창을 닫거나 끊기를 눌렀을 때 */
void webrtc_hangup(uint32_t session);

/* 연결된 시청자가 있으면 H.264 AU 를 보낸다 */
void webrtc_send_video(const uint8_t *data, size_t len, uint32_t pts_ms);

bool webrtc_is_streaming(void);

/* 상태 문자열 ("idle", "offering", "connecting", "connected", "failed") */
const char *webrtc_state_str(void);

typedef void (*webrtc_keyframe_cb_t)(void);
/* 연결 직후·PLI 수신 시 불린다. 인코더에 IDR 을 요청하는 데 쓴다 */
void webrtc_set_keyframe_cb(webrtc_keyframe_cb_t cb);
