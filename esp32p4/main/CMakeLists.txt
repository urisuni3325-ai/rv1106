idf_component_register(
    SRCS "main.c"
         "camera.c"
         "video_enc.c"
         "webrtc.c"
         "http_server.c"
         "wifi.c"
         "jpeg_util.c"
         "sdp_util.c"
    INCLUDE_DIRS "."
    EMBED_TXTFILES "web/index.html"
    PRIV_REQUIRES esp_driver_jpeg esp_driver_ppa esp_http_server esp_wifi esp_netif esp_timer
                  nvs_flash json
)
