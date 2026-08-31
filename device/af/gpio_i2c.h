/* GPIO 비트뱅 I2C 마스터.
 *
 * RV1106의 하드웨어 I2C 컨트롤러(i2c-3/i2c-4)는 핀헤더로 나와 있지 않아서,
 * 남는 GPIO 두 개로 I2C를 소프트웨어로 구현한다.
 *
 * I2C는 최소 클럭 속도 규정이 없어서 유저스페이스의 타이밍 지터를 그대로 견딘다.
 * DW9714는 한 번에 3바이트만 쓰므로 속도는 문제가 되지 않는다.
 *
 * 두 가지 백엔드를 지원한다.
 *   sysfs   : /sys/class/gpio  — 셸에서도 같은 동작을 재현할 수 있어 배선 확인이 쉽다
 *   chardev : /dev/gpiochipN   — sysfs가 커널에서 빠진 경우
 */
#ifndef GPIO_I2C_H
#define GPIO_I2C_H

#include <stdint.h>

typedef enum {
    GPIO_BACKEND_SYSFS = 0,
    GPIO_BACKEND_CHARDEV = 1,
} gpio_backend_t;

typedef struct gpio_i2c gpio_i2c_t;

/* sda/scl 는 백엔드에 따라 해석이 다르다.
 *   sysfs   : 전역 GPIO 번호 (GPIO1_A0 -> 32)
 *   chardev : "칩:라인" 을 chip*1000+line 으로 인코딩 (GPIO1_A0 -> 1000)
 * delay_us 는 클럭 반주기. 0 이면 기본값(5us).
 */
gpio_i2c_t *gpio_i2c_open(gpio_backend_t backend, int sda, int scl, int delay_us);
void gpio_i2c_close(gpio_i2c_t *bus);

/* 7비트 주소로 len 바이트 쓰기. 0 성공, 음수 실패(-2 = NACK). */
int gpio_i2c_write(gpio_i2c_t *bus, uint8_t addr7, const uint8_t *data, int len);

/* 주소만 보내고 ACK 여부를 본다. 1 = 응답 있음, 0 = 없음, 음수 = 오류. */
int gpio_i2c_ping(gpio_i2c_t *bus, uint8_t addr7);

/* 열기가 성공했는지 확인한다. gpio_i2c_open 은 실패해도 핸들을 돌려주므로 반드시 검사할 것. */
int gpio_i2c_ok(const gpio_i2c_t *bus);

/* 마지막 오류 설명. */
const char *gpio_i2c_strerror(const gpio_i2c_t *bus);

/* "1:0" / "32" 형태의 문자열을 백엔드에 맞는 정수로 바꾼다. 실패 시 -1. */
int gpio_i2c_parse_pin(const char *spec, gpio_backend_t backend);

#endif
