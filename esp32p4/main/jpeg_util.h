#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 표준 허프만 테이블 DHT 세그먼트 길이(마커 포함) */
#define JPEG_STD_DHT_SIZE 420

typedef struct {
    uint16_t width;
    uint16_t height;
    /* 휘도(Y) 샘플링 비율. 4:2:0 이면 2x2, 4:2:2 면 2x1, 4:4:4 면 1x1 */
    uint8_t  y_h_samp;
    uint8_t  y_v_samp;
    uint8_t  components;
    bool     has_dht;
} jpeg_header_t;

/* SOI 부터 SOS 앞까지 훑어서 크기·샘플링·DHT 유무를 읽는다.
 * 성공하면 0, JPEG 가 아니거나 잘렸으면 -1 */
int jpeg_parse_header(const uint8_t *data, size_t len, jpeg_header_t *out);

/* UVC 카메라의 MJPEG 프레임은 대부분 DHT(허프만 테이블)를 생략한다.
 * 그대로 저장하면 일부 뷰어가 못 여니까 SOS 앞에 표준 DHT 를 끼워 넣는다.
 *
 * out 은 len + JPEG_STD_DHT_SIZE 이상이어야 한다.
 * 반환: 출력 길이. DHT 가 이미 있으면 그대로 복사한 길이. 실패 시 0 */
size_t jpeg_insert_std_dht(const uint8_t *in, size_t len, uint8_t *out, size_t out_cap);

#ifdef __cplusplus
}
#endif
