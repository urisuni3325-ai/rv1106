/* 콘트라스트 검출 AF 탐색.
 *
 * 하드웨어와 완전히 분리돼 있다. 렌즈를 어떻게 움직이고 선명도를 어떻게 재는지는
 * 콜백이 담당하므로, 부품 없이도 합성 곡선으로 단위 테스트가 가능하다.
 *
 * 두 단계로 찾는다.
 *   1) 거친 스윕  — 전 구간을 coarse_step 간격으로 훑어 봉우리의 대략 위치를 찾는다
 *   2) 미세 탐색  — 그 주변만 fine_step 간격으로 다시 훑는다
 * 마지막에 봉우리 좌우 세 점으로 포물선 보간을 해서 스텝보다 미세한 위치를 낸다.
 */
#ifndef AF_SEARCH_H
#define AF_SEARCH_H

typedef enum {
    AF_OK = 0,
    AF_FAIL_MEASURE,    /* 측정 콜백이 실패 */
    AF_FAIL_FLAT,       /* 봉우리가 없음 — 피사체에 무늬가 없거나 렌즈가 안 움직임 */
    AF_WARN_AT_EDGE,    /* 봉우리가 구간 끝 — 초점 범위 밖일 가능성 */
} af_status_t;

typedef struct {
    int min_pos;
    int max_pos;
    int coarse_step;
    int fine_span;              /* 거친 최대점 기준 +- 이 범위를 미세 탐색 */
    int fine_step;
    double min_contrast_ratio;  /* 최고점/최저점 비가 이보다 작으면 평평하다고 본다 */
} af_config_t;

typedef struct {
    af_status_t status;
    int best_pos;               /* 실제로 이동해야 할 위치 */
    double best_score;
    double worst_score;
    int measurements;           /* 측정 횟수 — 탐색 비용 확인용 */
} af_result_t;

/* position 에서의 선명도를 돌려준다. 음수면 오류로 간주하고 탐색을 중단한다. */
typedef double (*af_measure_fn)(int position, void *ctx);

af_config_t af_config_default(void);
af_result_t af_search(const af_config_t *cfg, af_measure_fn measure, void *ctx);
const char *af_status_str(af_status_t status);

#endif
