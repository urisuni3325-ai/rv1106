/* 호스트 단위 테스트: make -C esp32p4/test
 * jpeg_util / sdp_util 은 ESP-IDF 의존성이 없어서 PC 에서 그대로 돈다. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "jpeg_util.h"
#include "sdp_util.h"

static uint8_t *read_file(const char *path, size_t *len)
{
    FILE *f = fopen(path, "rb");
    if (!f) {
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    *len = (size_t)ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *buf = malloc(*len);
    if (fread(buf, 1, *len, f) != *len) {
        free(buf);
        buf = NULL;
    }
    fclose(f);
    return buf;
}

/* 원본 JPEG 에서 DHT 세그먼트만 빼서 MJPEG 프레임처럼 만든다 */
static size_t strip_dht(const uint8_t *in, size_t len, uint8_t *out)
{
    size_t o = 0, pos = 2;
    out[o++] = 0xFF;
    out[o++] = 0xD8;
    while (pos + 4 <= len) {
        uint8_t m = in[pos + 1];
        if (m == 0xDA) {
            memcpy(out + o, in + pos, len - pos);
            return o + len - pos;
        }
        size_t seg = ((size_t)in[pos + 2] << 8) | in[pos + 3];
        if (m != 0xC4) {
            memcpy(out + o, in + pos, seg + 2);
            o += seg + 2;
        }
        pos += seg + 2;
    }
    return 0;
}

static void write_file(const char *path, const uint8_t *d, size_t len)
{
    FILE *f = fopen(path, "wb");
    assert(f);
    fwrite(d, 1, len, f);
    fclose(f);
}

static void test_jpeg(const char *src, const char *out_dir, int exp_w, int exp_h, int hs, int vs)
{
    size_t len = 0;
    uint8_t *orig = read_file(src, &len);
    assert(orig);

    jpeg_header_t hdr;
    assert(jpeg_parse_header(orig, len, &hdr) == 0);
    assert(hdr.width == exp_w && hdr.height == exp_h);
    assert(hdr.y_h_samp == hs && hdr.y_v_samp == vs);
    assert(hdr.has_dht);

    uint8_t *mjpeg = malloc(len);
    size_t mlen = strip_dht(orig, len, mjpeg);
    assert(mlen > 0 && mlen < len);
    assert(jpeg_parse_header(mjpeg, mlen, &hdr) == 0);
    assert(!hdr.has_dht);

    uint8_t *fixed = malloc(mlen + JPEG_STD_DHT_SIZE);
    /* 버퍼가 모자라면 실패해야 한다 */
    assert(jpeg_insert_std_dht(mjpeg, mlen, fixed, mlen) == 0);
    size_t flen = jpeg_insert_std_dht(mjpeg, mlen, fixed, mlen + JPEG_STD_DHT_SIZE);
    assert(flen == mlen + JPEG_STD_DHT_SIZE);
    assert(jpeg_parse_header(fixed, flen, &hdr) == 0 && hdr.has_dht);

    /* DHT 가 이미 있으면 그대로 */
    uint8_t *same = malloc(len + JPEG_STD_DHT_SIZE);
    assert(jpeg_insert_std_dht(orig, len, same, len + JPEG_STD_DHT_SIZE) == len);
    assert(memcmp(same, orig, len) == 0);

    char path[512];
    snprintf(path, sizeof(path), "%s/mjpeg_%dx%d.jpg", out_dir, exp_w, exp_h);
    write_file(path, mjpeg, mlen);
    snprintf(path, sizeof(path), "%s/fixed_%dx%d.jpg", out_dir, exp_w, exp_h);
    write_file(path, fixed, flen);

    free(orig);
    free(mjpeg);
    free(fixed);
    free(same);
}

static int cand_count;
static void on_cand(const char *c, void *ctx)
{
    (void)ctx;
    assert(strncmp(c, "candidate:", 10) == 0);
    assert(strchr(c, '\r') == NULL && strchr(c, '\n') == NULL);
    cand_count++;
}

static void test_sdp(void)
{
    const char *sdp =
        "v=0\r\n"
        "o=- 1 2 IN IP4 127.0.0.1\r\n"
        "m=video 9 UDP/TLS/RTP/SAVPF 96\r\n"
        "a=candidate:1 1 udp 2122260223 192.168.0.10 53472 typ host generation 0\r\n"
        "a=candidate:2 1 udp 2122260223 4f1c-uuid.local 53473 typ host\r\n"
        "a=end-of-candidates\r\n";
    cand_count = 0;
    assert(sdp_foreach_candidate(sdp, on_cand, NULL) == 2);
    assert(cand_count == 2);
    assert(sdp_has_video(sdp));
    /* LF 만 쓰는 SDP 도 */
    assert(sdp_foreach_candidate("a=candidate:9 1 udp 1 10.0.0.1 1 typ host\n", NULL, NULL) == 1);
    assert(sdp_foreach_candidate("v=0\r\nm=audio 9 RTP 0\r\n", NULL, NULL) == 0);
    assert(!sdp_has_video("v=0\r\nm=audio 9 RTP 0\r\n"));
    assert(sdp_foreach_candidate(NULL, NULL, NULL) == 0);
}

int main(int argc, char **argv)
{
    if (argc < 4) {
        fprintf(stderr, "usage: %s <420.jpg> <422.jpg> <out_dir>\n", argv[0]);
        return 2;
    }
    test_jpeg(argv[1], argv[3], 1280, 720, 2, 2);
    test_jpeg(argv[2], argv[3], 640, 480, 2, 1);
    test_sdp();
    printf("test_util: OK\n");
    return 0;
}
