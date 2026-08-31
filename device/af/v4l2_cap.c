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

static void fourcc_str(uint32_t fourcc, char out[5])
{
    out[0] = (char)(fourcc & 0xFF);
    out[1] = (char)((fourcc >> 8) & 0xFF);
    out[2] = (char)((fourcc >> 16) & 0xFF);
    out[3] = (char)((fourcc >> 24) & 0xFF);
    out[4] = '\0';
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

    if (cap->mplane) {
        if (want_w > 0 && want_h > 0) {
            fmt.fmt.pix_mp.width = want_w;
            fmt.fmt.pix_mp.height = want_h;
        }
        fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_NV12;
        if (xioctl(cap->fd, VIDIOC_S_FMT, &fmt) < 0)
            xioctl(cap->fd, VIDIOC_G_FMT, &fmt);   /* 드라이버가 정한 값을 그대로 쓴다 */

        if (!luma_plane_format(fmt.fmt.pix_mp.pixelformat)) {
            char code[5];
            fourcc_str(fmt.fmt.pix_mp.pixelformat, code);
            snprintf(err, errlen,
                     "지원하지 않는 픽셀 포맷 %s 입니다 (NV12/NV21/NV16/GREY 필요)", code);
            return -1;
        }
        cap->width = fmt.fmt.pix_mp.width;
        cap->height = fmt.fmt.pix_mp.height;
        cap->stride = fmt.fmt.pix_mp.plane_fmt[0].bytesperline;
    } else {
        if (want_w > 0 && want_h > 0) {
            fmt.fmt.pix.width = want_w;
            fmt.fmt.pix.height = want_h;
        }
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_NV12;
        if (xioctl(cap->fd, VIDIOC_S_FMT, &fmt) < 0)
            xioctl(cap->fd, VIDIOC_G_FMT, &fmt);

        if (!luma_plane_format(fmt.fmt.pix.pixelformat)) {
            char code[5];
            fourcc_str(fmt.fmt.pix.pixelformat, code);
            snprintf(err, errlen,
                     "지원하지 않는 픽셀 포맷 %s 입니다 (NV12/NV21/NV16/GREY 필요)", code);
            return -1;
        }
        cap->width = fmt.fmt.pix.width;
        cap->height = fmt.fmt.pix.height;
        cap->stride = fmt.fmt.pix.bytesperline;
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
    printf("캡처 가능한 노드:\n");
    for (int i = 0; i < 64; i++) {
        char path[32];
        snprintf(path, sizeof(path), "/dev/video%d", i);

        int fd = open(path, O_RDWR);
        if (fd < 0)
            continue;

        struct v4l2_capability capability;
        memset(&capability, 0, sizeof(capability));
        if (xioctl(fd, VIDIOC_QUERYCAP, &capability) == 0) {
            uint32_t caps = capability.capabilities & V4L2_CAP_DEVICE_CAPS
                          ? capability.device_caps : capability.capabilities;
            if (caps & (V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_VIDEO_CAPTURE_MPLANE)) {
                printf("  %-16s %s%s\n", path, capability.card,
                       (caps & V4L2_CAP_VIDEO_CAPTURE_MPLANE) ? " [mplane]" : "");
            }
        }
        close(fd);
    }
}
