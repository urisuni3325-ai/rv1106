#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_log.h"
#include "mdns.h"
#include "wifi.h"

static const char *TAG = "wifi";

#define GOT_IP_BIT BIT0

static EventGroupHandle_t s_events;
static char s_ip[16];
static int s_retry;

static void on_event(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        wifi_event_sta_disconnected_t *d = (wifi_event_sta_disconnected_t *)data;
        xEventGroupClearBits(s_events, GOT_IP_BIT);
        s_ip[0] = '\0';
        /* 공유기가 재부팅돼도 알아서 다시 붙도록 무한 재시도. 로그는 처음 몇 번만 */
        if (s_retry++ < 5 || s_retry % 30 == 0) {
            ESP_LOGW(TAG, "연결 끊김(reason %d), 재접속 시도 %d", d->reason, s_retry);
        }
        esp_wifi_connect();
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *e = (ip_event_got_ip_t *)data;
        snprintf(s_ip, sizeof(s_ip), IPSTR, IP2STR(&e->ip_info.ip));
        s_retry = 0;
        ESP_LOGI(TAG, "IP 받음: %s", s_ip);
        xEventGroupSetBits(s_events, GOT_IP_BIT);
    }
}

static void start_mdns(void)
{
    if (mdns_init() != ESP_OK) {
        ESP_LOGW(TAG, "mDNS 시작 실패 (IP 주소로 접속하세요)");
        return;
    }
    mdns_hostname_set(CONFIG_APP_MDNS_HOSTNAME);
    mdns_instance_name_set("ESP32-P4 USB Camera");
    mdns_service_add(NULL, "_http", "_tcp", 80, NULL, 0);
}

esp_err_t wifi_start(void)
{
    s_events = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, on_event, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, on_event, NULL, NULL));

    wifi_config_t wc = { 0 };
    strlcpy((char *)wc.sta.ssid, CONFIG_APP_WIFI_SSID, sizeof(wc.sta.ssid));
    strlcpy((char *)wc.sta.password, CONFIG_APP_WIFI_PASSWORD, sizeof(wc.sta.password));
    wc.sta.threshold.authmode = strlen(CONFIG_APP_WIFI_PASSWORD) ? WIFI_AUTH_WPA2_PSK : WIFI_AUTH_OPEN;
    wc.sta.sae_pwe_h2e = WPA3_SAE_PWE_BOTH;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wc));
    ESP_ERROR_CHECK(esp_wifi_start());
    /* 절전 모드는 영상 지연을 크게 늘린다 */
    esp_wifi_set_ps(WIFI_PS_NONE);

    ESP_LOGI(TAG, "\"%s\" 에 연결 중...", CONFIG_APP_WIFI_SSID);
    xEventGroupWaitBits(s_events, GOT_IP_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
    start_mdns();
    return ESP_OK;
}

bool wifi_is_connected(void)
{
    return s_events && (xEventGroupGetBits(s_events) & GOT_IP_BIT);
}

const char *wifi_ip_str(void)
{
    return s_ip;
}
