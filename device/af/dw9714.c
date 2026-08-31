#include "dw9714.h"

#include <time.h>

static void msleep(int ms)
{
    struct timespec ts = { ms / 1000, (long)(ms % 1000) * 1000000L };
    nanosleep(&ts, NULL);
}

static int clamp_pos(int position)
{
    if (position < DW9714_POS_MIN) return DW9714_POS_MIN;
    if (position > DW9714_POS_MAX) return DW9714_POS_MAX;
    return position;
}

int dw9714_set(gpio_i2c_t *bus, int position)
{
    position = clamp_pos(position);
    uint8_t frame[2] = {
        (uint8_t)((position >> 4) & 0x3F),
        (uint8_t)((position & 0x0F) << 4),
    };
    return gpio_i2c_write(bus, DW9714_ADDR, frame, 2);
}

int dw9714_set_from_below(gpio_i2c_t *bus, int position, int backlash, int settle_ms)
{
    position = clamp_pos(position);
    if (backlash < 0)
        backlash = 0;

    int approach = position - backlash;
    if (approach < DW9714_POS_MIN)
        approach = DW9714_POS_MIN;

    if (approach != position) {
        int rc = dw9714_set(bus, approach);
        if (rc < 0)
            return rc;
        msleep(settle_ms > 0 ? settle_ms : 1);
    }

    int rc = dw9714_set(bus, position);
    if (rc < 0)
        return rc;
    msleep(settle_ms > 0 ? settle_ms : 1);
    return 0;
}
