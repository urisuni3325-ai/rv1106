#include "v4l2_cap.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>

#define BUFFER_COUNT 4

typedef struct {
    void *start;
    size_t length;
} buffer_t;

struct v4l2_cap {
    int fd;
    int mplane;
    int width;
    int height;
    int stride;
    buffer_t buffers[BUFFER_COUNT];
    int buffer_count;
    int queued_index;   /* 호출자가 들고 있는 버퍼. -1 이면 없음 */
    int streaming;
};

static int xioctl(int fd, unsigned long request, void *arg)
{
    int r;
    do {
        r = ioctl(fd, request, arg);
    } while (r < 0 && errno == EINTR);
    return r;
}

/* 첫 평면이 8비트 휘도로 연속되어 있는 포맷만 쓴다. */
static int luma_plane_format(uint32_t fourcc)
{
    return fourcc == V4L2_PIX_FMT_NV12 || fourcc == V4L2_PIX_FMT_NV21 ||
           fourcc == V4L2_PIX_FMT_NV16 || fourcc == V4L2_PIX_FMT_GREY;
}

/* 출력할 수 없는 문자가 섞인 fourcc 도 알아볼 수 있게 16진수를 함께 적는다.
 * rkisp 의 일부 노드는 우리가 모르는 벤더 전용 포맷을 쓴다. */
static void fourcc_str(uint32_t fourcc, char out[24])
{
    char c[4];
    int printable = 1;

    for (int i = 0; i < 4; i++) {
        c[i] = (char)((fourcc >> (i * 8)) & 0xFF);
        if (c[i] < 0x20 || c[i] > 0x7E)
            printable = 0;
    }

    if (printable)
        snprintf(out, 24, "%c%c%c%c (0x%08x)", c[0], c[1], c[2], c[3], fourcc);
    else
        snprintf(out, 24, "0x%08x", fourcc);
}

static int setup_format(v4l2_cap_t *cap, int want_w, int want_h, char *err, size_t errlen)
{
    struct v4l2_format fmt;
    memset(&fmt, 0, sizeof(fmt));
    fmt.type = cap->mplane ? V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE
                           : V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (xioctl(cap->fd, VIDIOC_G_FMT, &fmt) < 0) {
        snprintf(err, errlen, "VIDIOC_G_FMT 실패: %s", strerror(errno));
        return -1;
    }

    uint32_t current = cap->mplane ? fmt.fmt.pix_mp.pixelformat : fmt.fmt.pix.pixelformat;

    /* 드라이버가 이미 잡아 놓은 포맷이 쓸 만하면 그대로 둔다.
     * rkisp 는 서브디바이스 파이프라인과 캡처 노드의 포맷이 맞아야 하는데,
     * 여기서 함부로 S_FMT 를 하면 STREAMON 이 EINVAL 로 거부한다. */
    int need_change = !luma_plane_format(current) || (want_w > 0 && want_h > 0);

    if (need_change) {
        struct v4l2_format wanted = fmt;
        if (cap->mplane) {
            if (want_w > 0 && want_h > 0) {
                wanted.fmt.pix_mp.width = want_w;
                wanted.fmt.pix_mp.height = want_h;
            }
            wanted.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_NV12;
        } else {
            if (want_w > 0 && want_h > 0) {
                wanted.fmt.pix.width = want_w;
                wanted.fmt.pix.height = want_h;
            }
            wanted.fmt.pix.pixelformat = V4L2_PIX_FMT_NV12;
        }

        if (xioctl(cap->fd, VIDIOC_S_FMT, &wanted) == 0)
            fmt = wanted;
        else
            xioctl(cap->fd, VIDIOC_G_FMT, &fmt);   /* 원래 설정으로 되돌린다 */
    }

    if (cap->mplane) {
        cap->width = fmt.fmt.pix_mp.width;
        cap->height = fmt.fmt.pix_mp.height;
        cap->stride = fmt.fmt.pix_mp.plane_fmt[0].bytesperline;
        current = fmt.fmt.pix_mp.pixelformat;
    } else {
        cap->width = fmt.fmt.pix.width;
        cap->height = fmt.fmt.pix.height;
        cap->stride = fmt.fmt.pix.bytesperline;
        current = fmt.fmt.pix.pixelformat;
    }

    if (!luma_plane_format(current)) {
        char code[24];
        fourcc_str(current, code);
        snprintf(err, errlen,
                 "이 노드의 픽셀 포맷 %s 는 지원하지 않습니다 "
                 "(NV12/NV21/NV16/GREY 필요). af_tool list-video 로 다른 노드를 보세요",
                 code);
        return -1;
    }

    if (cap->stride <= 0)
        cap->stride = cap->width;
    return 0;
}

static int setup_buffers(v4l2_cap_t *cap, char *err, size_t errlen)
{
    struct v4l2_requestbuffers req;
    memset(&req, 0, sizeof(req));
    req.count = BUFFER_COUNT;
    req.type = cap->mplane ? V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE
                           : V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (xioctl(cap->fd, VIDIOC_REQBUFS, &req) < 0) {
        snprintf(err, errlen, "VIDIOC_REQBUFS 실패: %s "
                              "(다른 프로그램이 카메라를 쓰고 있으면 rkipc 를 멈추세요)",
                 strerror(errno));
        return -1;
    }
    if (req.count < 2) {
        snprintf(err, errlen, "버퍼가 부족합니다 (%u개)", req.count);
        return -1;
    }
    cap->buffer_count = (int)req.count;

    for (int i = 0; i < cap->buffer_count; i++) {
        struct v4l2_buffer buf;
        struct v4l2_plane planes[VIDEO_MAX_PLANES];
        memset(&buf, 0, sizeof(buf));
        memset(planes, 0, sizeof(planes));

        buf.type = req.type;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        if (cap->mplane) {
            buf.m.planes = planes;
            buf.length = VIDEO_MAX_PLANES;
        }

        if (xioctl(cap->fd, VIDIOC_QUERYBUF, &buf) < 0) {
            snprintf(err, errlen, "VIDIOC_QUERYBUF 실패: %s", strerror(errno));
            return -1;
        }

        size_t length = cap->mplane ? planes[0].length : buf.length;
        off_t offset = cap->mplane ? planes[0].m.mem_offset : buf.m.offset;

        cap->buffers[i].length = length;
        cap->buffers[i].start = mmap(NULL, length, PROT_READ | PROT_WRITE,
                                     MAP_SHARED, cap->fd, offset);
        if (cap->buffers[i].start == MAP_FAILED) {
            cap->buffers[i].start = NULL;
            snprintf(err, errlen, "mmap 실패: %s", strerror(errno));
            return -1;
        }

        if (xioctl(cap->fd, VIDIOC_QBUF, &buf) < 0) {
            snprintf(err, errlen, "VIDIOC_QBUF 실패: %s", strerror(errno));
            return -1;
        }
    }

    int type = (int)req.type;
    if (xioctl(cap->fd, VIDIOC_STREAMON, &type) < 0) {
        snprintf(err, errlen, "VIDIOC_STREAMON 실패: %s", strerror(errno));
        return -1;
    }
    cap->streaming = 1;
    return 0;
}

v4l2_cap_t *v4l2_cap_open(const char *device, int want_w, int want_h,
                          char *err, size_t errlen)
{
    struct v4l2_capability capability;

    v4l2_cap_t *cap = calloc(1, sizeof(*cap));
    if (!cap) {
        snprintf(err, errlen, "메모리 부족");
        return NULL;
    }
    cap->fd = -1;
    cap->queued_index = -1;

    cap->fd = open(device, O_RDWR | O_NONBLOCK);
    if (cap->fd < 0) {
        snprintf(err, errlen, "%s 열기 실패: %s", device, strerror(errno));
        v4l2_cap_close(cap);
        return NULL;
    }
    /* 프레임 대기는 블로킹으로 하는 편이 간단하다. */
    int flags = fcntl(cap->fd, F_GETFL, 0);
    fcntl(cap->fd, F_SETFL, flags & ~O_NONBLOCK);

    memset(&capability, 0, sizeof(capability));
    if (xioctl(cap->fd, VIDIOC_QUERYCAP, &capability) < 0) {
        snprintf(err, errlen, "%s 는 V4L2 장치가 아닙니다", device);
        v4l2_cap_close(cap);
        return NULL;
    }

    uint32_t caps = capability.capabilities & V4L2_CAP_DEVICE_CAPS
                  ? capability.device_caps : capability.capabilities;

    if (caps & V4L2_CAP_VIDEO_CAPTURE_MPLANE)
        cap->mplane = 1;
    else if (!(caps & V4L2_CAP_VIDEO_CAPTURE)) {
        snprintf(err, errlen, "%s 는 캡처 노드가 아닙니다 "
                              "(af_tool list-video 로 캡처 가능한 노드를 찾으세요)", device);
        v4l2_cap_close(cap);
        return NULL;
    }

    if (setup_format(cap, want_w, want_h, err, errlen) < 0 ||
        setup_buffers(cap, err, errlen) < 0) {
        v4l2_cap_close(cap);
        return NULL;
    }

    return cap;
}

const uint8_t *v4l2_cap_grab(v4l2_cap_t *cap, int *width, int *height, int *stride)
{
    if (!cap || cap->fd < 0)
        return NULL;

    struct v4l2_buffer buf;
    struct v4l2_plane planes[VIDEO_MAX_PLANES];
    int type = cap->mplane ? V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE
                           : V4L2_BUF_TYPE_VIDEO_CAPTURE;

    /* 앞서 넘겨준 버퍼를 이제 돌려준다. */
    if (cap->queued_index >= 0) {
        memset(&buf, 0, sizeof(buf));
        memset(planes, 0, sizeof(planes));
        buf.type = type;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = cap->queued_index;
        if (cap->mplane) {
            buf.m.planes = planes;
            buf.length = VIDEO_MAX_PLANES;
        }
        xioctl(cap->fd, VIDIOC_QBUF, &buf);
        cap->queued_index = -1;
    }

    memset(&buf, 0, sizeof(buf));
    memset(planes, 0, sizeof(planes));
    buf.type = type;
    buf.memory = V4L2_MEMORY_MMAP;
    if (cap->mplane) {
        buf.m.planes = planes;
        buf.length = VIDEO_MAX_PLANES;
    }

    if (xioctl(cap->fd, VIDIOC_DQBUF, &buf) < 0)
        return NULL;

    cap->queued_index = (int)buf.index;
    if (width)  *width = cap->width;
    if (height) *height = cap->height;
    if (stride) *stride = cap->stride;
    return (const uint8_t *)cap->buffers[buf.index].start;
}

int v4l2_cap_warmup(v4l2_cap_t *cap, int frames)
{
    for (int i = 0; i < frames; i++) {
        if (!v4l2_cap_grab(cap, NULL, NULL, NULL))
            return -1;
    }
    return 0;
}

void v4l2_cap_close(v4l2_cap_t *cap)
{
    if (!cap)
        return;
    if (cap->streaming) {
        int type = cap->mplane ? V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE
                               : V4L2_BUF_TYPE_VIDEO_CAPTURE;
        xioctl(cap->fd, VIDIOC_STREAMOFF, &type);
    }
    for (int i = 0; i < cap->buffer_count; i++) {
        if (cap->buffers[i].start)
            munmap(cap->buffers[i].start, cap->buffers[i].length);
    }
    if (cap->fd >= 0)
        close(cap->fd);
    free(cap);
}

void v4l2_cap_list(void)
{
    printf("캡처 가능한 노드:\n\n");

    for (int i = 0; i < 64; i++) {
        char path[32];
        snprintf(path, sizeof(path), "/dev/video%d", i);

        int fd = open(path, O_RDWR);
        if (fd < 0)
            continue;

        struct v4l2_capability capability;
        memset(&capability, 0, sizeof(capability));
        if (xioctl(fd, VIDIOC_QUERYCAP, &capability) < 0) {
            close(fd);
            continue;
        }

        uint32_t caps = capability.capabilities & V4L2_CAP_DEVICE_CAPS
                      ? capability.device_caps : capability.capabilities;
        if (!(caps & (V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_VIDEO_CAPTURE_MPLANE))) {
            close(fd);
            continue;
        }

        int mplane = (caps & V4L2_CAP_VIDEO_CAPTURE_MPLANE) ? 1 : 0;
        int type = mplane ? V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE
                          : V4L2_BUF_TYPE_VIDEO_CAPTURE;

        printf("  %-14s %s%s\n", path, capability.card, mplane ? "  [mplane]" : "");

        /* 현재 설정 - 이게 그대로 쓸 수 있는지가 관건이다. */
        struct v4l2_format fmt;
        memset(&fmt, 0, sizeof(fmt));
        fmt.type = type;
        if (xioctl(fd, VIDIOC_G_FMT, &fmt) == 0) {
            uint32_t pixfmt = mplane ? fmt.fmt.pix_mp.pixelformat : fmt.fmt.pix.pixelformat;
            unsigned w = mplane ? fmt.fmt.pix_mp.width : fmt.fmt.pix.width;
            unsigned h = mplane ? fmt.fmt.pix_mp.height : fmt.fmt.pix.height;
            char code[24];
            fourcc_str(pixfmt, code);
            printf("      현재 설정   %ux%u  %s%s\n", w, h, code,
                   luma_plane_format(pixfmt) ? "   <- 바로 사용 가능" : "   <- 지원 안 함");
        }

        /* 이 노드가 낼 수 있는 포맷 중 우리가 쓸 수 있는 것. */
        printf("      사용 가능   ");
        int usable = 0;
        for (uint32_t index = 0; index < 32; index++) {
            struct v4l2_fmtdesc desc;
            memset(&desc, 0, sizeof(desc));
            desc.index = index;
            desc.type = type;
            if (xioctl(fd, VIDIOC_ENUM_FMT, &desc) < 0)
                break;
            if (luma_plane_format(desc.pixelformat)) {
                char code[24];
                fourcc_str(desc.pixelformat, code);
                printf("%s%s", usable ? ", " : "", code);
                usable++;
            }
        }
        printf("%s\n\n", usable ? "" : "(없음)");

        close(fd);
    }

    printf("휘도(Y) 평면을 바로 읽을 수 있는 노드를 --video 에 지정하세요.\n"
           "STREAMON 이 실패하면 rkipc 가 파이프라인을 잡고 있는 것입니다:\n"
           "  killall rkipc  후 다시 시도하고, 끝나면  RkLunch.sh &  로 되살리세요.\n");
}
