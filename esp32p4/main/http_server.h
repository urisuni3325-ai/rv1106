#pragma once

#include "esp_err.h"

/* 80번 포트 HTTP 서버
 *   GET  /                    웹 뷰어 (실시간 영상 + 스냅샷)
 *   GET  /api/offer           WebRTC SDP offer (text/plain)
 *   POST /api/answer          브라우저 SDP answer (text/plain)
 *   POST /api/hangup          시청 종료
 *   GET  /api/snapshot        스냅샷 JPEG. ?hires=1 이면 카메라 최대 해상도
 *   GET  /api/status          상태 JSON */
esp_err_t http_server_start(void);
