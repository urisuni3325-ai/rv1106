#include "af_search.h"

#include <stddef.h>

#define MAX_SAMPLES 256

af_config_t af_config_default(void)
{
    af_config_t cfg;
    /* 콘(거리 고정 경통)을 쓰면 실제 필요한 범위는 훨씬 좁다.
     * 처음에는 전 구간을 훑고, 사용 위치를 알아낸 뒤 min/max 를 좁히면 탐색이 빨라진다. */
    cfg.min_pos = 0;
    cfg.max_pos = 1023;
    cfg.coarse_step = 64;       /* 17 지점 */
    cfg.fine_span = 64;
    cfg.fine_step = 8;          /* 최대 17 지점 */
    cfg.min_contrast_ratio = 1.15;
    return cfg;
}

const char *af_status_str(af_status_t status)
{
    switch (status) {
    case AF_OK:           return "성공";
    case AF_FAIL_MEASURE: return "측정 실패";
    case AF_FAIL_FLAT:    return "봉우리 없음 (무늬가 없거나 렌즈가 안 움직임)";
    case AF_WARN_AT_EDGE: return "봉우리가 구간 끝 (초점 범위를 벗어났을 수 있음)";
    }
    return "알 수 없음";
}

typedef struct {
    int pos;
    double score;
} sample_t;

/* [from, to] 를 step 간격으로 훑어 samples 에 채운다. 마지막 지점은 항상 to 를 포함한다. */
static int sweep(af_measure_fn measure, void *ctx,
                 int from, int to, int step,
                 sample_t *samples, int *count, double *worst)
{
    *count = 0;

    for (int pos = from; ; pos += step) {
        if (pos > to)
            pos = to;

        double score = measure(pos, ctx);
        if (score < 0.0)
            return -1;

        if (*count < MAX_SAMPLES) {
            samples[*count].pos = pos;
            samples[*count].score = score;
            (*count)++;
        }
        if (*worst < 0.0 || score < *worst)
            *worst = score;

        if (pos >= to)
            break;
    }
    return 0;
}

static int best_index(const sample_t *samples, int count)
{
    int best = 0;
    for (int i = 1; i < count; i++) {
        if (samples[i].score > samples[best].score)
            best = i;
    }
    return best;
}

/* 봉우리와 좌우 이웃으로 포물선을 맞춰 스텝보다 미세한 정점을 추정한다.
 * 세 점이 봉우리 모양(아래로 볼록)이 아니면 가운데 위치를 그대로 쓴다. */
static int parabolic_peak(const sample_t *samples, int count, int index, int lo, int hi)
{
    if (index <= 0 || index >= count - 1)
        return samples[index].pos;

    const sample_t *l = &samples[index - 1];
    const sample_t *m = &samples[index];
    const sample_t *r = &samples[index + 1];

    double denom = l->score - 2.0 * m->score + r->score;
    if (denom >= 0.0)
        return m->pos;

    double offset = 0.5 * (l->score - r->score) / denom;
    if (offset < -1.0 || offset > 1.0)
        return m->pos;

    int step = r->pos - m->pos;
    if (step <= 0)
        step = m->pos - l->pos;

    int estimate = m->pos + (int)(offset * step);
    if (estimate < lo) estimate = lo;
    if (estimate > hi) estimate = hi;
    return estimate;
}

af_result_t af_search(const af_config_t *cfg, af_measure_fn measure, void *ctx)
{
    af_result_t result = { AF_FAIL_MEASURE, 0, 0.0, 0.0, 0 };

    if (!cfg || !measure || cfg->max_pos <= cfg->min_pos)
        return result;

    int coarse_step = cfg->coarse_step > 0 ? cfg->coarse_step : 64;
    int fine_step = cfg->fine_step > 0 ? cfg->fine_step : 8;

    sample_t samples[MAX_SAMPLES];
    int count = 0;
    double worst = -1.0;

    if (sweep(measure, ctx, cfg->min_pos, cfg->max_pos, coarse_step,
              samples, &count, &worst) < 0 || count == 0)
        return result;
    result.measurements += count;

    int index = best_index(samples, count);
    int coarse_best_pos = samples[index].pos;
    double coarse_best_score = samples[index].score;

    /* 미세 탐색 구간을 거친 봉우리 주변으로 좁힌다. */
    int lo = coarse_best_pos - cfg->fine_span;
    int hi = coarse_best_pos + cfg->fine_span;
    if (lo < cfg->min_pos) lo = cfg->min_pos;
    if (hi > cfg->max_pos) hi = cfg->max_pos;

    if (hi > lo) {
        sample_t fine[MAX_SAMPLES];
        int fine_count = 0;
        if (sweep(measure, ctx, lo, hi, fine_step, fine, &fine_count, &worst) < 0)
            return result;
        result.measurements += fine_count;

        if (fine_count > 0) {
            int fine_index = best_index(fine, fine_count);
            if (fine[fine_index].score >= coarse_best_score) {
                for (int i = 0; i < fine_count; i++)
                    samples[i] = fine[i];
                count = fine_count;
                index = fine_index;
            }
        }
    }

    result.best_pos = samples[index].pos;
    result.best_score = samples[index].score;
    result.worst_score = worst < 0.0 ? 0.0 : worst;

    int peak_pos = samples[index].pos;
    result.best_pos = parabolic_peak(samples, count, index, cfg->min_pos, cfg->max_pos);

    double ratio = result.worst_score > 0.0
                 ? result.best_score / result.worst_score
                 : (result.best_score > 0.0 ? 1e9 : 1.0);

    if (ratio < cfg->min_contrast_ratio)
        result.status = AF_FAIL_FLAT;
    else if (peak_pos <= cfg->min_pos || peak_pos >= cfg->max_pos)
        result.status = AF_WARN_AT_EDGE;
    else
        result.status = AF_OK;

    return result;
}
