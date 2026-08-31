/* 최소한의 V4L2 캡처.
 *
 * 선명도 계산에는 휘도(Y) 평면만 있으면 되므로 NV12/NV21/NV16/GREY 만 다룬다.
 * Rockchip 의 캡처 노드는 멀티플레인(MPLANE)인 경우가 많아 양쪽을 모두 지원한다.
 */
#ifndef V4L2_CAP_H
#define V4L2_CAP_H

#include <stddef.h>
#include <stdint.h>

typedef struct v4l2_cap v4l2_cap_t;

/* want_w/want_h 가 0 이면 드라이버의 현재 설정을 그대로 쓴다. */
v4l2_cap_t *v4l2_cap_open(const char *device, int want_w, int want_h,
                          char *err, size_t errlen);

/* 한 프레임을 받아 휘도 평면 포인터를 돌려준다. 다음 호출 전까지만 유효하다.
 * 실패하면 NULL. */
const uint8_t *v4l2_cap_grab(v4l2_cap_t *cap, int *width, int *height, int *stride);

/* 자동 노출이 안정될 때까지 몇 장 버린다. */
int v4l2_cap_warmup(v4l2_cap_t *cap, int frames);

void v4l2_cap_close(v4l2_cap_t *cap);

/* /dev/video* 중 캡처가 가능한 노드를 표준출력에 나열한다. */
void v4l2_cap_list(void);

#endif
