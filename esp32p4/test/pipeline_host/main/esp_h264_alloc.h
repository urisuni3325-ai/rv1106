#pragma once
#include <stdlib.h>
#include <stdint.h>
#define ESP_H264_MEM_SPIRAM 0
static inline void *esp_h264_aligned_calloc(uint32_t a, uint32_t n, uint32_t size, uint32_t *actual, uint32_t caps)
{ (void)a; (void)caps; *actual = n * size; return calloc(n, size); }
#define esp_h264_free free
