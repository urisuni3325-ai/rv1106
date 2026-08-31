#include "focus_score.h"

#include <stddef.h>

focus_score_t focus_score_tenengrad(const uint8_t *luma, int stride,
                                    int width, int height,
                                    int roi_x, int roi_y, int roi_w, int roi_h)
{
    focus_score_t out = { 0.0, 0.0, 0.0, 0 };

    if (!luma || stride <= 0 || width <= 0 || height <= 0)
        return out;

    /* 기울기를 쓰므로 프레임 경계에서 한 픽셀씩 물러난다. */
    int x0 = roi_x < 1 ? 1 : roi_x;
    int y0 = roi_y < 1 ? 1 : roi_y;
    int x1 = roi_x + roi_w;
    int y1 = roi_y + roi_h;
    if (x1 > width - 1)  x1 = width - 1;
    if (y1 > height - 1) y1 = height - 1;

    if (x1 <= x0 || y1 <= y0)
        return out;

    double gradient_sum = 0.0;
    double luma_sum = 0.0;
    long count = 0;

    for (int y = y0; y < y1; y++) {
        const uint8_t *row = luma + (size_t)y * stride;
        const uint8_t *above = row - stride;
        const uint8_t *below = row + stride;
        for (int x = x0; x < x1; x++) {
            int gx = (int)row[x + 1] - (int)row[x - 1];
            int gy = (int)below[x] - (int)above[x];
            gradient_sum += (double)(gx * gx + gy * gy);
            luma_sum += row[x];
            count++;
        }
    }

    if (count == 0)
        return out;

    out.pixels = (int)count;
    out.tenengrad = gradient_sum / (double)count;
    out.mean_luma = luma_sum / (double)count;
    /* 밝기가 변하면 기울기도 비례해 커지므로 평균 밝기 제곱으로 나눠 보정한다.
     * 자동 노출이 도는 중에도 비교가 가능해진다. */
    if (out.mean_luma > 1.0)
        out.normalized = out.tenengrad / (out.mean_luma * out.mean_luma);
    return out;
}

void focus_score_center_roi(int width, int height, double fraction,
                            int *roi_x, int *roi_y, int *roi_w, int *roi_h)
{
    if (fraction <= 0.0 || fraction > 1.0)
        fraction = 0.25;

    int w = (int)(width * fraction);
    int h = (int)(height * fraction);
    if (w < 8) w = width < 8 ? width : 8;
    if (h < 8) h = height < 8 ? height : 8;

    *roi_w = w;
    *roi_h = h;
    *roi_x = (width - w) / 2;
    *roi_y = (height - h) / 2;
}
