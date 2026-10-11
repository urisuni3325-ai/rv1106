/* ESP32-P4 + USB UVC 카메라(Arducam B0290 / IMX298) → WiFi WebRTC 실시간 영상 + 스냅샷
 *
 *  USB 카메라 ──MJPEG──> JPEG 디코더(HW) ──YUV420──> H.264 인코더(HW) ──> WebRTC(esp_peer) ──WiFi──> 브라우저
 *               (카메라 / 디코드 / 인코드 / 전송이 각각 별도 태스크 — pipeline.c)
 *       └──────────── 스냅샷: MJPEG 원본(최대 4656x3496) 그대로 ──────────────> HTTP /api/snapshot
 */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "camera.h"
#include "pipeline.h"
#include "webrtc.h"
#include "wifi.h"
#include "http_server.h"

static const char *TAG = "main";

void app_main(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    ESP_ERROR_CHECK(pipeline_start(CONFIG_APP_STREAM_FPS, CONFIG_APP_H264_BITRATE_KBPS * 1000,
                                   CONFIG_APP_H264_MIN_BITRATE_KBPS * 1000, CONFIG_APP_H264_GOP));
    ESP_ERROR_CHECK(camera_start(CONFIG_APP_STREAM_WIDTH, CONFIG_APP_STREAM_HEIGHT, CONFIG_APP_STREAM_FPS,
                                 pipeline_on_camera_frame, pipeline_request_idr));

    ESP_ERROR_CHECK(wifi_start());
    ESP_ERROR_CHECK(webrtc_init(CONFIG_APP_STREAM_WIDTH, CONFIG_APP_STREAM_HEIGHT, CONFIG_APP_STREAM_FPS));
    const webrtc_callbacks_t cbs = {
        .on_connected = pipeline_reset_bitrate,
        .on_keyframe_request = pipeline_request_idr,
        .on_network_feedback = pipeline_on_network_feedback,
    };
    webrtc_set_callbacks(&cbs);
    ESP_ERROR_CHECK(http_server_start());

    ESP_LOGI(TAG, "==============================================");
    ESP_LOGI(TAG, " 브라우저로 접속:  http://%s/", wifi_ip_str());
    ESP_LOGI(TAG, "            또는:  http://%s.local/", CONFIG_APP_MDNS_HOSTNAME);
    ESP_LOGI(TAG, "==============================================");
}
