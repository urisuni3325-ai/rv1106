#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "esp_err.h"

/* 하드웨어 영상 변환 단계들. 실제 태스크 구성은 pipeline.c
 *
 *  디코드: MJPEG → (하드웨어 JPEG 디코더) → YUV420 (O_UYY_E_VYY, H.264 인코더 입력 포맷)
 *          4:2:0 이고 16 배수면 디코더가 바로 YUV420 으로 뽑는다. 4:2:2 등은 RGB565 로
 *          디코드한 뒤 PPA(픽셀 처리 가속기)로 YUV420 변환 + 16 배수 크롭.
 *  인코드: YUV420 → (하드웨어 H.264 인코더) → H.264 AU
 *
 * 디코드 함수는 디코드 태스크에서만, 인코드 함수는 인코드 태스크에서만 부른다. */

esp_err_t video_dec_init(void);

/* JPEG 헤더만 보고 인코딩 크기(16 배수로 내림)를 알려 준다 */
esp_err_t video_dec_probe(const uint8_t *jpeg, size_t len, int *enc_w, int *enc_h);

/* jpeg_alloc_decoder_mem 으로 YUV420 버퍼 할당 (디코더·PPA·H.264 DMA 정렬 조건을 만족) */
uint8_t *video_alloc_yuv(int w, int h, size_t *cap);
void video_free_yuv(uint8_t *buf);

/* *ppa_path 에 어느 경로를 썼는지 돌려준다 */
esp_err_t video_dec_to_yuv(const uint8_t *jpeg, size_t len, uint8_t *yuv, size_t yuv_cap,
                           int enc_w, int enc_h, bool *ppa_path);

esp_err_t video_h264_open(int w, int h, uint8_t fps, uint32_t bitrate_bps, uint8_t gop);
void video_h264_close(void);
bool video_h264_is_open(int w, int h);
esp_err_t video_h264_encode(const uint8_t *yuv, uint32_t yuv_len, uint32_t pts_ms,
                            uint8_t *out, uint32_t out_cap, uint32_t *out_len, bool *keyframe);
void video_h264_force_idr(void);
esp_err_t video_h264_set_bitrate(uint32_t bitrate_bps);
