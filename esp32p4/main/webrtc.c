#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_peer.h"
#include "esp_peer_default.h"
#include "sdp_util.h"
#include "webrtc.h"

static const char *TAG = "webrtc";

typedef enum {
    CMD_OFFER,
    CMD_ANSWER,
    CMD_HANGUP,
} cmd_type_t;

typedef struct {
    cmd_type_t type;
    char *sdp;
} cmd_t;

typedef enum {
    ST_IDLE,
    ST_OFFERING,
    ST_CONNECTING,
    ST_CONNECTED,
    ST_FAILED,
} conn_state_t;

static esp_peer_handle_t s_peer;
static QueueHandle_t s_cmd_q;
static SemaphoreHandle_t s_offer_sem;
/* esp_peer_send_video 와 disconnect/new_connection 이 겹치지 않게 */
static SemaphoreHandle_t s_peer_lock;
static char *s_local_sdp;
static volatile conn_state_t s_state = ST_IDLE;
static bool s_started;
static webrtc_keyframe_cb_t s_keyframe_cb;
/* HTTP 태스크에서만 읽고 쓴다 */
static uint32_t s_session;

static void request_keyframe(void)
{
    if (s_keyframe_cb) {
        s_keyframe_cb();
    }
}

static int on_state(esp_peer_state_t state, void *ctx)
{
    switch (state) {
    case ESP_PEER_STATE_CONNECTED:
        ESP_LOGI(TAG, "시청자 연결됨");
        s_state = ST_CONNECTED;
        request_keyframe();
        break;
    case ESP_PEER_STATE_DISCONNECTED:
        if (s_state == ST_CONNECTED || s_state == ST_CONNECTING) {
            ESP_LOGI(TAG, "시청자 연결 끊김");
            s_state = ST_IDLE;
        }
        break;
    case ESP_PEER_STATE_CONNECT_FAILED:
        /* 이전 연결의 늦은 이벤트가 새 offer 상태를 덮어쓰지 않게 */
        if (s_state == ST_CONNECTING) {
            ESP_LOGW(TAG, "연결 실패 (ICE/DTLS)");
            s_state = ST_FAILED;
        }
        break;
    case ESP_PEER_STATE_PAIRED:
        ESP_LOGI(TAG, "ICE 연결됨, DTLS 핸드셰이크 중");
        break;
    case ESP_PEER_STATE_VIDEO_PLI_RECEIVED:
        /* 브라우저가 패킷 유실로 디코딩을 못 이어가면 키프레임을 달라고 한다 */
        request_keyframe();
        break;
    default:
        break;
    }
    return 0;
}

static int on_msg(esp_peer_msg_t *msg, void *ctx)
{
    if (msg->type == ESP_PEER_MSG_TYPE_SDP && s_state == ST_OFFERING) {
        free(s_local_sdp);
        s_local_sdp = strndup((const char *)msg->data, msg->size);
        xSemaphoreGive(s_offer_sem);
    }
    return 0;
}

static int on_video_info(esp_peer_video_stream_info_t *info, void *ctx)
{
    return 0;
}

static int on_audio_info(esp_peer_audio_stream_info_t *info, void *ctx)
{
    return 0;
}

static int on_video_data(esp_peer_video_frame_t *frame, void *ctx)
{
    return 0;
}

static int on_audio_data(esp_peer_audio_frame_t *frame, void *ctx)
{
    return 0;
}

static void feed_candidate(const char *cand, void *ctx)
{
    esp_peer_msg_t m = {
        .type = ESP_PEER_MSG_TYPE_CANDIDATE,
        .data = (uint8_t *)cand,
        .size = strlen(cand),
    };
    esp_peer_send_msg(s_peer, &m);
}

static void disconnect_locked(void)
{
    xSemaphoreTake(s_peer_lock, portMAX_DELAY);
    s_state = ST_IDLE;
    if (s_started) {
        esp_peer_disconnect(s_peer);
        s_started = false;
    }
    xSemaphoreGive(s_peer_lock);
}

static void handle_cmd(cmd_t *cmd)
{
    switch (cmd->type) {
    case CMD_OFFER:
        disconnect_locked();
        xSemaphoreTake(s_peer_lock, portMAX_DELAY);
        s_state = ST_OFFERING;
        /* 후보 수집이 끝나면 main_loop 안에서 on_msg 로 offer 가 나온다 */
        if (esp_peer_new_connection(s_peer) == ESP_PEER_ERR_NONE) {
            s_started = true;
        } else {
            ESP_LOGE(TAG, "새 연결 생성 실패");
            s_state = ST_FAILED;
        }
        xSemaphoreGive(s_peer_lock);
        break;
    case CMD_ANSWER: {
        if (s_state != ST_OFFERING) {
            ESP_LOGW(TAG, "offer 없이 answer 가 와서 무시");
            break;
        }
        s_state = ST_CONNECTING;
        esp_peer_msg_t m = {
            .type = ESP_PEER_MSG_TYPE_SDP,
            .data = (uint8_t *)cmd->sdp,
            .size = strlen(cmd->sdp),
        };
        esp_peer_send_msg(s_peer, &m);
        /* SDP 안의 후보도 따로 넣어 준다 (esp-webrtc-solution 의 HTTP 시그널링과 같은 방식) */
        int n = sdp_foreach_candidate(cmd->sdp, feed_candidate, NULL);
        ESP_LOGI(TAG, "answer 수신, 후보 %d개", n);
        break;
    }
    case CMD_HANGUP:
        if (s_state != ST_IDLE) {
            ESP_LOGI(TAG, "시청자가 나감");
        }
        disconnect_locked();
        break;
    }
    free(cmd->sdp);
}

static void peer_task(void *arg)
{
    while (1) {
        cmd_t cmd;
        while (xQueueReceive(s_cmd_q, &cmd, 0) == pdTRUE) {
            handle_cmd(&cmd);
        }
        /* esp_peer 기본 구현은 내부 스레드가 없다. ICE/DTLS/RTCP 처리를 전부 여기서 돌린다 */
        esp_peer_main_loop(s_peer);
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

esp_err_t webrtc_init(int width, int height, int fps)
{
    s_cmd_q = xQueueCreate(4, sizeof(cmd_t));
    s_offer_sem = xSemaphoreCreateBinary();
    s_peer_lock = xSemaphoreCreateMutex();

    esp_peer_default_cfg_t def = {
        .agent_recv_timeout = 100,
        .keep_role = true,
        .rtp_cfg = {
            /* 1080p 키프레임 한 장이 200~300KB 까지 나오므로 넉넉히 */
            .send_pool_size = 1024 * 1024,
            .send_queue_num = 512,
            .max_resend_count = 3,
        },
    };
    static esp_peer_ice_server_cfg_t stun = {
        .stun_url = CONFIG_APP_STUN_URL,
    };
    esp_peer_cfg_t cfg = {
        .server_lists = &stun,
        .server_num = strlen(CONFIG_APP_STUN_URL) ? 1 : 0,
        .role = ESP_PEER_ROLE_CONTROLLING,
        .ice_trans_policy = ESP_PEER_ICE_TRANS_POLICY_ALL,
        .video_info = {
            .codec = ESP_PEER_VIDEO_CODEC_H264,
            .width = width,
            .height = height,
            .fps = fps,
        },
        .video_dir = ESP_PEER_MEDIA_DIR_SEND_ONLY,
        .audio_dir = ESP_PEER_MEDIA_DIR_NONE,
        .no_auto_reconnect = true,
        .enable_data_channel = false,
        .extra_cfg = &def,
        .extra_size = sizeof(def),
        .on_state = on_state,
        .on_msg = on_msg,
        .on_video_info = on_video_info,
        .on_audio_info = on_audio_info,
        .on_video_data = on_video_data,
        .on_audio_data = on_audio_data,
    };
    /* DTLS 인증서를 미리 만들어 두면 첫 연결이 1~2초 빨라진다 */
    esp_peer_pre_generate_cert();
    int ret = esp_peer_open(&cfg, esp_peer_get_default_impl(), &s_peer);
    if (ret != ESP_PEER_ERR_NONE) {
        ESP_LOGE(TAG, "esp_peer_open 실패 %d", ret);
        return ESP_FAIL;
    }
    /* DTLS 핸드셰이크(ECDSA)가 무거워서 스택을 넉넉히 */
    if (xTaskCreatePinnedToCore(peer_task, "peer", 12 * 1024, NULL, 6, NULL, 1) != pdPASS) {
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

esp_err_t webrtc_create_offer(char **sdp, uint32_t *session, int timeout_ms)
{
    xSemaphoreTake(s_offer_sem, 0);
    cmd_t cmd = { .type = CMD_OFFER };
    if (xQueueSend(s_cmd_q, &cmd, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    if (xSemaphoreTake(s_offer_sem, pdMS_TO_TICKS(timeout_ms)) != pdTRUE) {
        ESP_LOGE(TAG, "offer 생성 시간 초과");
        return ESP_ERR_TIMEOUT;
    }
    /* on_msg 는 peer 태스크에서 불리고, 다음 offer 전까지 s_local_sdp 를 다시 안 건드린다 */
    *sdp = strdup(s_local_sdp);
    if (*sdp == NULL) {
        return ESP_ERR_NO_MEM;
    }
    *session = ++s_session;
    return ESP_OK;
}

esp_err_t webrtc_set_answer(uint32_t session, const char *sdp)
{
    if (session != s_session) {
        return ESP_ERR_INVALID_STATE;
    }
    if (!sdp_has_video(sdp)) {
        return ESP_ERR_INVALID_ARG;
    }
    cmd_t cmd = { .type = CMD_ANSWER, .sdp = strdup(sdp) };
    if (cmd.sdp == NULL) {
        return ESP_ERR_NO_MEM;
    }
    if (xQueueSend(s_cmd_q, &cmd, pdMS_TO_TICKS(1000)) != pdTRUE) {
        free(cmd.sdp);
        return ESP_ERR_TIMEOUT;
    }
    return ESP_OK;
}

void webrtc_hangup(uint32_t session)
{
    if (session != s_session) {
        return;
    }
    cmd_t cmd = { .type = CMD_HANGUP };
    xQueueSend(s_cmd_q, &cmd, pdMS_TO_TICKS(1000));
}

void webrtc_send_video(const uint8_t *data, size_t len, uint32_t pts_ms)
{
    if (s_state != ST_CONNECTED) {
        return;
    }
    xSemaphoreTake(s_peer_lock, portMAX_DELAY);
    if (s_state == ST_CONNECTED) {
        esp_peer_video_frame_t f = {
            .pts = pts_ms,
            .data = (uint8_t *)data,
            .size = (int)len,
        };
        esp_peer_send_video(s_peer, &f);
    }
    xSemaphoreGive(s_peer_lock);
}

bool webrtc_is_streaming(void)
{
    return s_state == ST_CONNECTED;
}

const char *webrtc_state_str(void)
{
    switch (s_state) {
    case ST_OFFERING:
        return "offering";
    case ST_CONNECTING:
        return "connecting";
    case ST_CONNECTED:
        return "connected";
    case ST_FAILED:
        return "failed";
    default:
        return "idle";
    }
}

void webrtc_set_keyframe_cb(webrtc_keyframe_cb_t cb)
{
    s_keyframe_cb = cb;
}
