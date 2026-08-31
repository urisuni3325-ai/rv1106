/* 호스트에서 도는 단위 테스트. 보드도 VCM 도 필요 없다.
 * 탐색 로직과 선명도 계산이 하드웨어와 분리돼 있어서 가능한 검증이다.
 */
#include "af_search.h"
#include "focus_score.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures = 0;
static int checks = 0;

static void check(int condition, const char *what)
{
    checks++;
    if (!condition) {
        failures++;
        printf("  FAIL  %s\n", what);
    } else {
        printf("  ok    %s\n", what);
    }
}

/* ------------------------------------------------ af_search */

typedef struct {
    double peak_position;
    double width;
    double baseline;
    int calls;
    int fail_after;     /* 0 이면 실패하지 않음 */
} curve_t;

/* 실제 초점 곡선을 흉내낸 가우시안. */
static double curve_measure(int position, void *ctx)
{
    curve_t *curve = (curve_t *)ctx;
    curve->calls++;
    if (curve->fail_after && curve->calls > curve->fail_after)
        return -1.0;

    double d = (position - curve->peak_position) / curve->width;
    return curve->baseline + exp(-d * d);
}

static void test_finds_peak(void)
{
    printf("탐색: 봉우리를 찾는다\n");
    curve_t curve = { 600.0, 90.0, 0.05, 0, 0 };
    af_config_t cfg = af_config_default();

    af_result_t r = af_search(&cfg, curve_measure, &curve);

    check(r.status == AF_OK, "상태가 AF_OK");
    check(abs(r.best_pos - 600) <= cfg.fine_step,
          "찾은 위치가 실제 봉우리에서 미세 스텝 이내");
    check(r.measurements <= 40, "측정 횟수가 40회 이하 (탐색 비용)");
    check(r.measurements == curve.calls, "측정 횟수 집계가 실제 호출과 일치");
}

static void test_off_grid_peak(void)
{
    printf("탐색: 격자에 없는 봉우리도 보간으로 잡는다\n");
    /* 605 는 거친(64) 격자에도 미세(8) 격자에도 없다. */
    curve_t curve = { 605.0, 90.0, 0.05, 0, 0 };
    af_config_t cfg = af_config_default();

    af_result_t r = af_search(&cfg, curve_measure, &curve);

    check(r.status == AF_OK, "상태가 AF_OK");
    check(abs(r.best_pos - 605) <= cfg.fine_step,
          "격자 밖 봉우리도 미세 스텝 이내로 추정");
}

static void test_flat_scene(void)
{
    printf("탐색: 무늬 없는 장면은 실패로 보고한다\n");
    /* 폭을 아주 넓게 잡으면 전 구간이 거의 평평해진다. */
    curve_t curve = { 500.0, 100000.0, 10.0, 0, 0 };
    af_config_t cfg = af_config_default();

    af_result_t r = af_search(&cfg, curve_measure, &curve);

    check(r.status == AF_FAIL_FLAT, "상태가 AF_FAIL_FLAT");
}

static void test_peak_at_edge(void)
{
    printf("탐색: 봉우리가 구간 끝이면 경고한다\n");
    curve_t curve = { 0.0, 90.0, 0.05, 0, 0 };
    af_config_t cfg = af_config_default();

    af_result_t r = af_search(&cfg, curve_measure, &curve);

    check(r.status == AF_WARN_AT_EDGE, "상태가 AF_WARN_AT_EDGE");
    check(r.best_pos < cfg.fine_step, "그래도 위치는 구간 시작 근처로 나온다");
}

static void test_measure_failure(void)
{
    printf("탐색: 측정이 실패하면 중단한다\n");
    curve_t curve = { 600.0, 90.0, 0.05, 0, 3 };
    af_config_t cfg = af_config_default();

    af_result_t r = af_search(&cfg, curve_measure, &curve);

    check(r.status == AF_FAIL_MEASURE, "상태가 AF_FAIL_MEASURE");
    check(curve.calls <= 4, "실패 직후 더 진행하지 않는다");
}

static void test_narrow_range(void)
{
    printf("탐색: 구간을 좁히면 측정이 줄어든다 (콘 사용 시)\n");
    curve_t wide = { 600.0, 90.0, 0.05, 0, 0 };
    curve_t narrow = { 600.0, 90.0, 0.05, 0, 0 };

    af_config_t full = af_config_default();
    af_config_t tight = af_config_default();
    tight.min_pos = 520;
    tight.max_pos = 680;
    tight.coarse_step = 16;
    tight.fine_span = 16;

    af_result_t r_full = af_search(&full, curve_measure, &wide);
    af_result_t r_tight = af_search(&tight, curve_measure, &narrow);

    check(r_tight.status == AF_OK, "좁은 구간에서도 성공");
    check(abs(r_tight.best_pos - r_full.best_pos) <= full.fine_step,
          "좁힌 구간의 결과가 전 구간 결과와 일치");
    check(r_tight.measurements < r_full.measurements,
          "좁힌 구간이 측정 횟수가 더 적다");
}

/* ---------------------------------------------- focus_score */

static void fill_flat(uint8_t *image, int stride, int height, uint8_t value)
{
    memset(image, value, (size_t)stride * height);
}

static void fill_checker(uint8_t *image, int stride, int width, int height, int size)
{
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++)
            image[(size_t)y * stride + x] = ((x / size + y / size) % 2) ? 220 : 30;
    }
}

static void test_focus_score(void)
{
    printf("선명도: 무늬가 있으면 점수가 높다\n");
    const int w = 64, h = 64, stride = 80;   /* stride > width 로 패딩 확인 */
    uint8_t *flat = malloc((size_t)stride * h);
    uint8_t *checker = malloc((size_t)stride * h);

    fill_flat(flat, stride, h, 128);
    fill_flat(checker, stride, h, 0);
    fill_checker(checker, stride, w, h, 2);

    focus_score_t s_flat = focus_score_tenengrad(flat, stride, w, h, 0, 0, w, h);
    focus_score_t s_checker = focus_score_tenengrad(checker, stride, w, h, 0, 0, w, h);

    check(s_flat.tenengrad == 0.0, "균일한 화면의 Tenengrad 는 0");
    check(s_checker.tenengrad > s_flat.tenengrad, "체커 무늬가 균일 화면보다 높다");
    check(s_flat.pixels > 0 && s_checker.pixels == s_flat.pixels,
          "두 경우의 유효 픽셀 수가 같다");

    printf("선명도: 밝기가 변해도 정규화 값은 크게 안 흔들린다\n");
    uint8_t *dim = malloc((size_t)stride * h);
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++)
            dim[(size_t)y * stride + x] = (uint8_t)(checker[(size_t)y * stride + x] / 2);

    focus_score_t s_dim = focus_score_tenengrad(dim, stride, w, h, 0, 0, w, h);
    double ratio = s_dim.normalized / s_checker.normalized;
    check(ratio > 0.7 && ratio < 1.4, "밝기 절반에서도 정규화 값이 비슷");
    check(s_dim.tenengrad < s_checker.tenengrad * 0.6,
          "정규화 안 한 값은 밝기에 따라 크게 변한다 (정규화가 필요한 이유)");

    printf("선명도: ROI 가 프레임을 벗어나도 잘라서 계산한다\n");
    focus_score_t s_clipped = focus_score_tenengrad(checker, stride, w, h,
                                                    w - 4, h - 4, 100, 100);
    check(s_clipped.pixels > 0, "잘린 ROI 에서도 픽셀이 남는다");

    focus_score_t s_outside = focus_score_tenengrad(checker, stride, w, h,
                                                    w + 10, h + 10, 8, 8);
    check(s_outside.pixels == 0, "완전히 벗어난 ROI 는 픽셀 0");

    printf("선명도: 중앙 ROI 계산\n");
    int rx, ry, rw, rh;
    focus_score_center_roi(1920, 1080, 0.25, &rx, &ry, &rw, &rh);
    check(rw == 480 && rh == 270, "1920x1080 의 25% ROI 는 480x270");
    check(rx == 720 && ry == 405, "ROI 가 가운데 위치");

    free(flat);
    free(checker);
    free(dim);
}

int main(void)
{
    test_finds_peak();
    test_off_grid_peak();
    test_flat_scene();
    test_peak_at_edge();
    test_measure_failure();
    test_narrow_range();
    test_focus_score();

    printf("\n%d 개 검사 중 %d 개 실패\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
