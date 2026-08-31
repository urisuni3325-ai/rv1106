/* 초점 선명도 점수 (Tenengrad).
 *
 * 인접 픽셀의 기울기 제곱합. 초점이 맞을수록 경계가 뚜렷해져 값이 커진다.
 * NV12 의 휘도(Y) 평면만 보므로 색 정보는 필요 없다.
 *
 * 두피 스코프에서는 화면 전체가 아니라 중앙 ROI 만 봐야 한다.
 * 전체를 보면 시야 가장자리의 머리카락에 초점이 물리는 일이 생긴다.
 */
#ifndef FOCUS_SCORE_H
#define FOCUS_SCORE_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    double tenengrad;   /* 픽셀당 평균 기울기 제곱 */
    double mean_luma;   /* ROI 평균 밝기 (0..255) */
    double normalized;  /* tenengrad / mean_luma^2 — 노출 변화에 덜 민감 */
    int    pixels;
} focus_score_t;

/* ROI 는 프레임 좌표. 프레임 밖이면 잘라서 계산한다.
 * 유효 픽셀이 없으면 pixels 가 0 이고 나머지는 0 이다.
 */
focus_score_t focus_score_tenengrad(const uint8_t *luma, int stride,
                                    int width, int height,
                                    int roi_x, int roi_y, int roi_w, int roi_h);

/* 프레임 가운데를 fraction 비율(0<f<=1)로 잡는 ROI 헬퍼. */
void focus_score_center_roi(int width, int height, double fraction,
                            int *roi_x, int *roi_y, int *roi_w, int *roi_h);

#endif
