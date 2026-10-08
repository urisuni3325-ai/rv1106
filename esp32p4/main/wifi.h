#pragma once

#include <stdbool.h>
#include "esp_err.h"

/* STA 모드로 공유기에 붙고 IP 를 받을 때까지 기다린다.
 * ESP32-P4 는 WiFi 가 없어서 보드의 ESP32-C6 를 esp_hosted(SDIO) 로 쓴다.
 * 코드 쪽에서는 일반 esp_wifi API 그대로다. */
esp_err_t wifi_start(void);

bool wifi_is_connected(void);

/* "192.168.0.23" 형태. 연결 전이면 빈 문자열 */
const char *wifi_ip_str(void);
