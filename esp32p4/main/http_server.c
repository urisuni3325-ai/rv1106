#include <string.h>
#include <stdlib.h>
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "cJSON.h"
#include "camera.h"
#include "video_enc.h"
#include "webrtc.h"
#include "wifi.h"
#include "http_server.h"

static const char *TAG = "http";

#define MAX_SDP_LEN (16 * 1024)

extern const char index_html_start[] asm("_binary_index_html_start");
extern const char index_html_end[] asm("_binary_index_html_end");

static esp_err_t send_err(httpd_req_t *req, const char *status, const char *msg)
{
    httpd_resp_set_status(req, status);
    httpd_resp_set_type(req, "text/plain; charset=utf-8");
    return httpd_resp_sendstr(req, msg);
}

static esp_err_t index_get(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    httpd_resp_set_hdr(req, "Cache-Control", "no-cache");
    return httpd_resp_send(req, index_html_start, index_html_end - index_html_start);
}

static uint32_t query_session(httpd_req_t *req)
{
    char q[64], val[16];
    if (httpd_req_get_url_query_str(req, q, sizeof(q)) != ESP_OK ||
        httpd_query_key_value(q, "session", val, sizeof(val)) != ESP_OK) {
        return 0;
    }
    return (uint32_t)strtoul(val, NULL, 10);
}

static esp_err_t offer_get(httpd_req_t *req)
{
    char *sdp = NULL;
    uint32_t session = 0;
    esp_err_t err = webrtc_create_offer(&sdp, &session, 8000);
    if (err != ESP_OK) {
        return send_err(req, "503 Service Unavailable", "WebRTC offer 생성 실패");
    }
    char sid[12];
    snprintf(sid, sizeof(sid), "%lu", (unsigned long)session);
    httpd_resp_set_type(req, "application/sdp");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    httpd_resp_set_hdr(req, "X-Session", sid);
    err = httpd_resp_sendstr(req, sdp);
    free(sdp);
    return err;
}

static char *recv_body(httpd_req_t *req, size_t max)
{
    if (req->content_len == 0 || req->content_len > max) {
        return NULL;
    }
    char *buf = malloc(req->content_len + 1);
    if (buf == NULL) {
        return NULL;
    }
    size_t got = 0;
    while (got < req->content_len) {
        int r = httpd_req_recv(req, buf + got, req->content_len - got);
        if (r == HTTPD_SOCK_ERR_TIMEOUT) {
            continue;
        }
        if (r <= 0) {
            free(buf);
            return NULL;
        }
        got += r;
    }
    buf[got] = '\0';
    return buf;
}

static esp_err_t answer_post(httpd_req_t *req)
{
    char *sdp = recv_body(req, MAX_SDP_LEN);
    if (sdp == NULL) {
        return send_err(req, "400 Bad Request", "SDP 본문이 없거나 너무 큼");
    }
    esp_err_t err = webrtc_set_answer(query_session(req), sdp);
    free(sdp);
    if (err == ESP_ERR_INVALID_STATE) {
        return send_err(req, "409 Conflict", "다른 기기가 먼저 연결했습니다");
    }
    if (err != ESP_OK) {
        return send_err(req, "400 Bad Request", "잘못된 SDP answer");
    }
    return httpd_resp_sendstr(req, "OK");
}

static esp_err_t hangup_post(httpd_req_t *req)
{
    /* sendBeacon 은 본문을 보내므로 읽어서 버린다 */
    char tmp[64];
    size_t left = req->content_len;
    while (left > 0) {
        int r = httpd_req_recv(req, tmp, left < sizeof(tmp) ? left : sizeof(tmp));
        if (r <= 0) {
            break;
        }
        left -= r;
    }
    webrtc_hangup(query_session(req));
    return httpd_resp_sendstr(req, "OK");
}

static bool query_flag(httpd_req_t *req, const char *key)
{
    char q[64], val[8];
    if (httpd_req_get_url_query_str(req, q, sizeof(q)) != ESP_OK) {
        return false;
    }
    if (httpd_query_key_value(q, key, val, sizeof(val)) != ESP_OK) {
        return false;
    }
    return strcmp(val, "1") == 0 || strcmp(val, "true") == 0;
}

static esp_err_t snapshot_get(httpd_req_t *req)
{
    bool hires = query_flag(req, "hires");
    camera_snapshot_t snap;
    int64_t t0 = esp_timer_get_time();
    esp_err_t err = camera_snapshot(hires, &snap, hires ? 15000 : 3000);
    if (err == ESP_ERR_INVALID_STATE) {
        return send_err(req, "503 Service Unavailable", "카메라가 연결되지 않았습니다");
    }
    if (err != ESP_OK) {
        return send_err(req, "500 Internal Server Error", esp_err_to_name(err));
    }
    ESP_LOGI(TAG, "스냅샷 %dx%d %uKB (%lldms)%s", snap.width, snap.height, (unsigned)(snap.len / 1024),
             (esp_timer_get_time() - t0) / 1000, snap.fallback ? " — 고해상도 실패, 실시간 해상도로 대체" : "");

    char hdr[32];
    httpd_resp_set_type(req, "image/jpeg");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    snprintf(hdr, sizeof(hdr), "%dx%d", snap.width, snap.height);
    httpd_resp_set_hdr(req, "X-Resolution", hdr);
    httpd_resp_set_hdr(req, "X-Fallback", snap.fallback ? "1" : "0");
    httpd_resp_set_hdr(req, "Access-Control-Expose-Headers", "X-Resolution, X-Fallback");
    err = httpd_resp_send(req, (const char *)snap.data, snap.len);
    camera_snapshot_free(&snap);
    return err;
}

static esp_err_t status_get(httpd_req_t *req)
{
    camera_status_t cam;
    video_enc_stats_t enc;
    camera_get_status(&cam);
    video_enc_get_stats(&enc);

    cJSON *root = cJSON_CreateObject();
    cJSON *c = cJSON_AddObjectToObject(root, "camera");
    cJSON_AddBoolToObject(c, "connected", cam.connected);
    cJSON_AddNumberToObject(c, "width", cam.width);
    cJSON_AddNumberToObject(c, "height", cam.height);
    cJSON_AddNumberToObject(c, "fps", (int)(cam.fps * 10) / 10.0);
    cJSON_AddNumberToObject(c, "max_width", cam.max_width);
    cJSON_AddNumberToObject(c, "max_height", cam.max_height);
    cJSON_AddNumberToObject(c, "dropped", cam.dropped);
    cJSON *e = cJSON_AddObjectToObject(root, "encoder");
    cJSON_AddNumberToObject(e, "width", enc.width);
    cJSON_AddNumberToObject(e, "height", enc.height);
    cJSON_AddBoolToObject(e, "ppa_path", enc.ppa_path);
    cJSON_AddNumberToObject(e, "frames", enc.frames);
    cJSON_AddNumberToObject(e, "bytes", enc.bytes);
    cJSON_AddNumberToObject(e, "last_ms", enc.last_ms);
    cJSON_AddNumberToObject(e, "errors", enc.errors);
    cJSON_AddStringToObject(root, "webrtc", webrtc_state_str());
    cJSON_AddStringToObject(root, "ip", wifi_ip_str());
    cJSON_AddNumberToObject(root, "heap_internal", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
    cJSON_AddNumberToObject(root, "heap_psram", heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
    cJSON_AddNumberToObject(root, "uptime_s", esp_timer_get_time() / 1000000);

    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (json == NULL) {
        return send_err(req, "500 Internal Server Error", "no mem");
    }
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    esp_err_t err = httpd_resp_sendstr(req, json);
    free(json);
    return err;
}

esp_err_t http_server_start(void)
{
    httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
    cfg.stack_size = 8192;
    cfg.max_open_sockets = 7;
    cfg.lru_purge_enable = true;
    /* 16MP 스냅샷(수 MB)을 느린 WiFi 로 보낼 때 끊기지 않게 */
    cfg.send_wait_timeout = 30;
    cfg.recv_wait_timeout = 10;
    cfg.core_id = 1;

    httpd_handle_t server = NULL;
    esp_err_t err = httpd_start(&server, &cfg);
    if (err != ESP_OK) {
        return err;
    }
    const httpd_uri_t uris[] = {
        { .uri = "/", .method = HTTP_GET, .handler = index_get },
        { .uri = "/api/offer", .method = HTTP_GET, .handler = offer_get },
        { .uri = "/api/answer", .method = HTTP_POST, .handler = answer_post },
        { .uri = "/api/hangup", .method = HTTP_POST, .handler = hangup_post },
        { .uri = "/api/snapshot", .method = HTTP_GET, .handler = snapshot_get },
        { .uri = "/api/status", .method = HTTP_GET, .handler = status_get },
    };
    for (size_t i = 0; i < sizeof(uris) / sizeof(uris[0]); i++) {
        httpd_register_uri_handler(server, &uris[i]);
    }
    return ESP_OK;
}
