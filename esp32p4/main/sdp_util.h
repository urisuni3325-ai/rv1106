#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* SDP 안의 "a=candidate:..." 줄마다 cb 를 부른다.
 * cb 에는 브라우저 RTCIceCandidate.candidate 형식("candidate:..." — a= 와 줄바꿈 제외)으로 넘긴다.
 * 반환: 찾은 후보 개수 */
int sdp_foreach_candidate(const char *sdp, void (*cb)(const char *cand, void *ctx), void *ctx);

/* SDP 에 미디어 섹션(m=video) 이 있는지 */
int sdp_has_video(const char *sdp);

#ifdef __cplusplus
}
#endif
