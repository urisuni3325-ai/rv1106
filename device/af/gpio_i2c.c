#include "gpio_i2c.h"

#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/gpio.h>

#define DEFAULT_DELAY_US 5
#define STRETCH_TIMEOUT_US 100000

typedef struct {
    int dir_fd;     /* sysfs: direction 파일 */
    int val_fd;     /* sysfs: value 파일 */
    int line_fd;    /* chardev: 라인 핸들 */
    int number;     /* 원래 지정값 (오류 메시지용) */
    int exported;   /* sysfs: 우리가 export 했으면 종료 시 unexport */
} gpio_pin_t;

struct gpio_i2c {
    gpio_backend_t backend;
    gpio_pin_t sda;
    gpio_pin_t scl;
    int delay_us;
    char err[192];
};

static void udelay(int us)
{
    struct timespec ts = { us / 1000000, (long)(us % 1000000) * 1000L };
    nanosleep(&ts, NULL);
}

static void set_err(gpio_i2c_t *bus, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(bus->err, sizeof(bus->err), fmt, ap);
    va_end(ap);
}

/* ---------------------------------------------------------------- sysfs */

static int sysfs_write(const char *path, const char *value)
{
    int fd = open(path, O_WRONLY);
    if (fd < 0)
        return -1;
    ssize_t n = write(fd, value, strlen(value));
    close(fd);
    return n < 0 ? -1 : 0;
}

static int sysfs_pin_open(gpio_i2c_t *bus, gpio_pin_t *pin, int number)
{
    char path[96], num[16];

    pin->number = number;
    pin->line_fd = -1;
    snprintf(num, sizeof(num), "%d", number);

    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d", number);
    if (access(path, F_OK) != 0) {
        if (sysfs_write("/sys/class/gpio/export", num) < 0) {
            set_err(bus, "GPIO %d export 실패: %s "
                         "(핀이 다른 드라이버에 점유돼 있거나 번호가 틀렸습니다)",
                    number, strerror(errno));
            return -1;
        }
        pin->exported = 1;
        /* udev 가 파일을 만들 때까지 잠깐 기다린다. */
        for (int i = 0; i < 100 && access(path, F_OK) != 0; i++)
            udelay(1000);
    }

    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/direction", number);
    pin->dir_fd = open(path, O_WRONLY);
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value", number);
    pin->val_fd = open(path, O_RDONLY);

    if (pin->dir_fd < 0 || pin->val_fd < 0) {
        set_err(bus, "GPIO %d 파일 열기 실패: %s", number, strerror(errno));
        return -1;
    }
    return 0;
}

static void sysfs_pin_close(gpio_pin_t *pin)
{
    char num[16];
    if (pin->dir_fd >= 0) close(pin->dir_fd);
    if (pin->val_fd >= 0) close(pin->val_fd);
    if (pin->exported) {
        snprintf(num, sizeof(num), "%d", pin->number);
        sysfs_write("/sys/class/gpio/unexport", num);
    }
    pin->dir_fd = pin->val_fd = -1;
    pin->exported = 0;
}

/* "low" 를 direction 에 쓰면 출력+0 이 한 번에 설정된다. */
static void sysfs_drive_low(gpio_pin_t *pin)
{
    if (write(pin->dir_fd, "low", 3) < 0) { /* 폴백 없음 - 아래 read 에서 드러난다 */ }
}

/* 입력으로 되돌리면 외부 풀업이 HIGH 로 끌어올린다 (오픈 드레인). */
static void sysfs_release(gpio_pin_t *pin)
{
    if (write(pin->dir_fd, "in", 2) < 0) { }
}

static int sysfs_read(gpio_pin_t *pin)
{
    char c = '0';
    if (lseek(pin->val_fd, 0, SEEK_SET) < 0)
        return -1;
    if (read(pin->val_fd, &c, 1) != 1)
        return -1;
    return c == '1' ? 1 : 0;
}

/* -------------------------------------------------------------- chardev */

static int chardev_pin_open(gpio_i2c_t *bus, gpio_pin_t *pin, int encoded)
{
    struct gpiohandle_request req;
    char path[48];
    int chip = encoded / 1000;
    int line = encoded % 1000;

    pin->number = encoded;
    pin->dir_fd = pin->val_fd = -1;
    pin->exported = 0;

    snprintf(path, sizeof(path), "/dev/gpiochip%d", chip);
    int chip_fd = open(path, O_RDWR);
    if (chip_fd < 0) {
        set_err(bus, "%s 열기 실패: %s", path, strerror(errno));
        return -1;
    }

    memset(&req, 0, sizeof(req));
    req.lineoffsets[0] = line;
    req.lines = 1;
    req.flags = GPIOHANDLE_REQUEST_OUTPUT | GPIOHANDLE_REQUEST_OPEN_DRAIN;
    req.default_values[0] = 1;
    snprintf(req.consumer_label, sizeof(req.consumer_label), "af-i2c");

    if (ioctl(chip_fd, GPIO_GET_LINEHANDLE_IOCTL, &req) < 0) {
        set_err(bus, "gpiochip%d 라인 %d 요청 실패: %s "
                     "(오픈 드레인 미지원일 수 있습니다. sysfs 백엔드를 쓰세요)",
                chip, line, strerror(errno));
        close(chip_fd);
        return -1;
    }
    close(chip_fd);
    pin->line_fd = req.fd;
    return 0;
}

static void chardev_set(gpio_pin_t *pin, int value)
{
    struct gpiohandle_data data;
    memset(&data, 0, sizeof(data));
    data.values[0] = (uint8_t)value;
    ioctl(pin->line_fd, GPIOHANDLE_SET_LINE_VALUES_IOCTL, &data);
}

static int chardev_read(gpio_pin_t *pin)
{
    struct gpiohandle_data data;
    memset(&data, 0, sizeof(data));
    if (ioctl(pin->line_fd, GPIOHANDLE_GET_LINE_VALUES_IOCTL, &data) < 0)
        return -1;
    return data.values[0] ? 1 : 0;
}

/* --------------------------------------------------------- 공통 라인 조작 */

static void pin_low(gpio_i2c_t *bus, gpio_pin_t *pin)
{
    if (bus->backend == GPIO_BACKEND_SYSFS)
        sysfs_drive_low(pin);
    else
        chardev_set(pin, 0);
}

static void pin_release(gpio_i2c_t *bus, gpio_pin_t *pin)
{
    if (bus->backend == GPIO_BACKEND_SYSFS)
        sysfs_release(pin);
    else
        chardev_set(pin, 1);
}

static int pin_read(gpio_i2c_t *bus, gpio_pin_t *pin)
{
    return bus->backend == GPIO_BACKEND_SYSFS ? sysfs_read(pin) : chardev_read(pin);
}

/* 슬레이브가 SCL 을 붙잡고 있을 수 있으므로(클럭 스트레칭) 실제로 HIGH 가 될 때까지 기다린다. */
static int scl_release_wait(gpio_i2c_t *bus)
{
    pin_release(bus, &bus->scl);
    for (int waited = 0; waited < STRETCH_TIMEOUT_US; waited += bus->delay_us) {
        int v = pin_read(bus, &bus->scl);
        if (v < 0)
            return -1;
        if (v == 1)
            return 0;
        udelay(bus->delay_us);
    }
    set_err(bus, "SCL 이 HIGH 로 올라오지 않습니다 "
                 "(풀업 저항 4.7k 가 3.3V 에 연결돼 있는지 확인하세요)");
    return -1;
}

/* ------------------------------------------------------------ I2C 프리미티브 */

static int i2c_start(gpio_i2c_t *bus)
{
    pin_release(bus, &bus->sda);
    if (scl_release_wait(bus) < 0)
        return -1;
    udelay(bus->delay_us);
    pin_low(bus, &bus->sda);
    udelay(bus->delay_us);
    pin_low(bus, &bus->scl);
    udelay(bus->delay_us);
    return 0;
}

static void i2c_stop(gpio_i2c_t *bus)
{
    pin_low(bus, &bus->sda);
    udelay(bus->delay_us);
    if (scl_release_wait(bus) < 0)
        return;
    udelay(bus->delay_us);
    pin_release(bus, &bus->sda);
    udelay(bus->delay_us);
}

/* 한 바이트 전송 후 ACK 를 읽는다. 1 = ACK, 0 = NACK, 음수 = 오류. */
static int i2c_write_byte(gpio_i2c_t *bus, uint8_t byte)
{
    for (int i = 7; i >= 0; i--) {
        if ((byte >> i) & 1)
            pin_release(bus, &bus->sda);
        else
            pin_low(bus, &bus->sda);
        udelay(bus->delay_us);
        if (scl_release_wait(bus) < 0)
            return -1;
        udelay(bus->delay_us);
        pin_low(bus, &bus->scl);
        udelay(bus->delay_us);
    }

    /* 9번째 클럭에서 슬레이브가 SDA 를 끌어내리면 ACK. */
    pin_release(bus, &bus->sda);
    udelay(bus->delay_us);
    if (scl_release_wait(bus) < 0)
        return -1;
    udelay(bus->delay_us);
    int ack = pin_read(bus, &bus->sda);
    pin_low(bus, &bus->scl);
    udelay(bus->delay_us);

    if (ack < 0)
        return -1;
    return ack == 0 ? 1 : 0;
}

/* SDA 가 어떤 슬레이브에 붙잡혀 있으면 클럭 9개로 풀어준다. */
static void bus_recover(gpio_i2c_t *bus)
{
    pin_release(bus, &bus->sda);
    for (int i = 0; i < 9; i++) {
        if (pin_read(bus, &bus->sda) == 1)
            break;
        pin_release(bus, &bus->scl);
        udelay(bus->delay_us);
        pin_low(bus, &bus->scl);
        udelay(bus->delay_us);
    }
    pin_release(bus, &bus->scl);
}

/* ------------------------------------------------------------------ 공개 API */

gpio_i2c_t *gpio_i2c_open(gpio_backend_t backend, int sda, int scl, int delay_us)
{
    gpio_i2c_t *bus = calloc(1, sizeof(*bus));
    if (!bus)
        return NULL;

    bus->backend = backend;
    bus->delay_us = delay_us > 0 ? delay_us : DEFAULT_DELAY_US;
    bus->sda.dir_fd = bus->sda.val_fd = bus->sda.line_fd = -1;
    bus->scl.dir_fd = bus->scl.val_fd = bus->scl.line_fd = -1;

    int rc;
    if (backend == GPIO_BACKEND_SYSFS)
        rc = sysfs_pin_open(bus, &bus->sda, sda) || sysfs_pin_open(bus, &bus->scl, scl);
    else
        rc = chardev_pin_open(bus, &bus->sda, sda) || chardev_pin_open(bus, &bus->scl, scl);

    if (rc) {
        /* set_err 는 이미 채워져 있다. 호출자가 메시지를 읽고 닫는다. */
        return bus;
    }

    pin_release(bus, &bus->sda);
    pin_release(bus, &bus->scl);
    udelay(bus->delay_us * 4);
    bus_recover(bus);
    return bus;
}

void gpio_i2c_close(gpio_i2c_t *bus)
{
    if (!bus)
        return;
    if (bus->backend == GPIO_BACKEND_SYSFS) {
        sysfs_pin_close(&bus->sda);
        sysfs_pin_close(&bus->scl);
    } else {
        if (bus->sda.line_fd >= 0) close(bus->sda.line_fd);
        if (bus->scl.line_fd >= 0) close(bus->scl.line_fd);
    }
    free(bus);
}

int gpio_i2c_ok(const gpio_i2c_t *bus)
{
    return bus && bus->err[0] == 0;
}

int gpio_i2c_write(gpio_i2c_t *bus, uint8_t addr7, const uint8_t *data, int len)
{
    if (!bus || bus->err[0])
        return -1;
    if (i2c_start(bus) < 0)
        return -1;

    int ack = i2c_write_byte(bus, (uint8_t)(addr7 << 1));
    if (ack < 0) { i2c_stop(bus); return -1; }
    if (ack == 0) {
        set_err(bus, "주소 0x%02x 가 ACK 하지 않습니다 (배선/전원/주소 확인)", addr7);
        i2c_stop(bus);
        return -2;
    }

    for (int i = 0; i < len; i++) {
        ack = i2c_write_byte(bus, data[i]);
        if (ack < 0) { i2c_stop(bus); return -1; }
        if (ack == 0) {
            set_err(bus, "%d 번째 데이터 바이트에서 NACK", i);
            i2c_stop(bus);
            return -2;
        }
    }

    i2c_stop(bus);
    return 0;
}

int gpio_i2c_ping(gpio_i2c_t *bus, uint8_t addr7)
{
    if (!bus || bus->err[0])
        return -1;
    if (i2c_start(bus) < 0)
        return -1;
    int ack = i2c_write_byte(bus, (uint8_t)(addr7 << 1));
    i2c_stop(bus);
    if (ack < 0)
        return -1;
    return ack;
}

const char *gpio_i2c_strerror(const gpio_i2c_t *bus)
{
    if (!bus)
        return "버스가 없습니다";
    return bus->err[0] ? bus->err : "오류 없음";
}

int gpio_i2c_parse_pin(const char *spec, gpio_backend_t backend)
{
    if (!spec || !*spec)
        return -1;

    const char *colon = strchr(spec, ':');
    if (colon) {
        if (backend != GPIO_BACKEND_CHARDEV)
            return -1;
        int chip = atoi(spec);
        int line = atoi(colon + 1);
        if (chip < 0 || line < 0 || line >= 1000)
            return -1;
        return chip * 1000 + line;
    }

    if (backend != GPIO_BACKEND_SYSFS)
        return -1;
    char *end = NULL;
    long v = strtol(spec, &end, 10);
    if (end == spec || *end != '\0' || v < 0)
        return -1;
    return (int)v;
}
