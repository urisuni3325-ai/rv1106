/* DW9714 VCM 드라이버 (I2C 10비트 DAC).
 *
 * 프로토콜은 레지스터 주소가 없는 2바이트 쓰기다.
 *   byte0 = [00][pos 상위 6비트]
 *   byte1 = [pos 하위 4비트][S1 S0][T1 T0]
 * S/T 는 이동 모드와 스텝 주기이며 0 이면 직접 구동(Direct mode)이다.
 *
 * 위치는 0..1023 이며 값이 클수록 렌즈가 센서에서 멀어진다(=근거리 초점).
 * 실제 방향은 모듈마다 반대일 수 있으므로 sweep 으로 확인할 것.
 */
#ifndef DW9714_H
#define DW9714_H

#include "gpio_i2c.h"

#define DW9714_ADDR      0x0C
#define DW9714_POS_MIN   0
#define DW9714_POS_MAX   1023

/* 위치를 그대로 쓴다. 0 성공, 음수 실패. */
int dw9714_set(gpio_i2c_t *bus, int position);

/* 히스테리시스를 없애기 위해 항상 아래쪽에서 접근한다.
 * VCM 은 스프링 구조라 같은 DAC 값이라도 어느 방향에서 왔느냐에 따라
 * 실제 렌즈 위치가 수 um 달라진다. 탐색 중에는 반드시 이 함수를 쓸 것.
 * backlash 는 되돌아갈 여유(권장 40), settle_ms 는 정착 대기(권장 25).
 */
int dw9714_set_from_below(gpio_i2c_t *bus, int position, int backlash, int settle_ms);

#endif
