# ESP32-P4 + USB 카메라(IMX298) → WiFi WebRTC 실시간 영상 + 스냅샷

ESP32-P4 보드에 Arducam **B0290 (16MP IMX298 오토포커스 USB 카메라)** 를 꽂으면,
같은 공유기에 있는 PC/폰 브라우저에서 **WebRTC 로 실시간 영상**을 보고
**스냅샷(최대 4656x3496, 16MP)** 을 찍어 저장할 수 있습니다.
앱 설치도, 외부 서버도 필요 없습니다. 보드가 웹 페이지와 WebRTC 시그널링을 직접 제공합니다.

```
 B0290 (IMX298) ──USB2.0 MJPEG──> ESP32-P4
                                   ├─ JPEG 디코더(HW) → YUV420 → H.264 인코더(HW) → WebRTC(esp_peer)
                                   │                                                  │
                                   └─ 스냅샷: MJPEG 원본 그대로 (16MP 까지)          │
                                                │                                     │
                                         HTTP /api/snapshot                     SRTP (UDP)
                                                └────────── WiFi (ESP32-C6, SDIO) ────┘
                                                                     │
                                                         브라우저  http://p4cam.local/
                                                         ├─ 실시간 영상 (H.264, 기본 1280x720 30fps)
                                                         ├─ 스냅샷 (실시간 해상도 / 고해상도 16MP)
                                                         └─ 사진 저장 (다운로드)
```

## 준비물

| 항목 | 내용 |
|---|---|
| 보드 | **ESP32-P4-Function-EV-Board** (P4 + WiFi 용 ESP32-C6 내장). PSRAM 32MB |
| 카메라 | Arducam B0290 — IMX298, USB2.0 UVC, MJPEG/YUY2 |
| 연결 | 카메라 USB 를 보드의 **USB 2.0 HS 호스트 포트(USB-A)** 에 꽂습니다 |
| 전원 | 카메라가 5V 200mA 를 씁니다. 보드는 PC USB 보다 **5V 2A 어댑터**로 주는 게 안전합니다 |
| PC | ESP-IDF **v5.5 계열** (v5.5.1 에서 빌드 확인) |

> 다른 ESP32-P4 보드도 됩니다. WiFi 가 ESP32-C6 + SDIO 가 아니면 `idf.py menuconfig` →
> `Component config → ESP-Hosted` 에서 핀/인터페이스를 맞춰 주세요.

### 카메라가 내보내는 포맷 (데이터시트)

| 포맷 | 해상도 / 프레임레이트 |
|---|---|
| MJPEG | 4656x3496 · 4160x3120 · 3264x2448 · 2592x1944 @10fps |
| MJPEG | 2320x1744 · 1920x1080 · 1280x720 @30fps |
| YUY2 | 1024x768 · 800x600 · 640x480 @10fps |

- **실시간 영상**은 MJPEG 1280x720@30 (기본) 또는 1920x1080@30 을 받아 H.264 로 바꿔 보냅니다.
  ESP32-P4 의 H.264 하드웨어 인코더는 가로 1920 이 한계라 2320 이상은 실시간에 못 씁니다.
- **고해상도 스냅샷**은 잠깐 4656x3496 MJPEG 로 바꿔서 한 장 받고 돌아옵니다
  (카메라가 다시 노출을 잡는 동안 1~2초 영상이 멈춥니다).
  재인코딩 없이 카메라가 만든 JPEG 원본을 그대로 저장합니다.

## 빌드 & 플래시

```sh
. $HOME/esp/esp-idf/export.sh          # ESP-IDF 환경 (설치 경로에 맞게)
cd esp32p4
idf.py set-target esp32p4
idf.py menuconfig                       # → "P4 USB 카메라 WebRTC" → WiFi 이름/비밀번호 입력
idf.py build flash monitor
```

필요한 컴포넌트(`usb_host_uvc`, `esp_h264`, `esp_peer`, `esp_wifi_remote`/`esp_hosted`, `mdns`)는
첫 빌드 때 ESP 컴포넌트 레지스트리에서 자동으로 받아옵니다.

부팅하면 시리얼 로그에 주소가 찍힙니다.

```
I (5123) main: ==============================================
I (5123) main:  브라우저로 접속:  http://192.168.0.23/
I (5123) main:             또는:  http://p4cam.local/
I (5123) main: ==============================================
```

### 칩 리비전 주의 (부팅이 안 될 때)

ESP-IDF v5.5.2 부터는 기본값이 **ESP32-P4 v3.x 칩**용입니다.
플래시할 때 `Chip is ESP32-P4 (revision v1.0)` 처럼 **v1.x 이하**로 나오면:

```
idf.py menuconfig
  → Component config → Hardware Settings → Chip revision
  → [*] Select ESP32-P4 revisions <3.0 (No >=3.x Support)
```

(v5.5.1 은 v1.x 전용이라 이 설정이 없습니다. v3.x 칩이면 v5.5.2 이상을 쓰세요.)

## 사용법

브라우저(크롬/엣지/사파리/파이어폭스, 폰도 됨)로 `http://<보드IP>/` 를 열면 자동으로 연결됩니다.

| 버튼 | 동작 |
|---|---|
| 연결 / 끊기 | WebRTC 연결. 끊기면 3초 뒤 자동 재연결 |
| 스냅샷 | 지금 실시간 해상도(1280x720)로 즉시 한 장. 영상 끊김 없음 |
| 고해상도 스냅샷 | 카메라 최대 해상도(4656x3496)로 한 장. 1~2초 영상 멈춤 |
| 전체 화면 | 영상만 전체 화면 |
| 찍으면 바로 저장 | 체크하면 찍을 때마다 바로 다운로드 (`IMG_날짜_시각_해상도.jpg`) |

찍은 사진은 아래 목록에 쌓이고, **저장**을 누르면 PC/폰에 내려받습니다.
썸네일을 누르면 크게 봅니다.

- 시청자는 **한 번에 한 명**입니다. 다른 기기가 접속하면 이전 기기는 "다른 기기가 보는 중" 으로
  바뀌고 자동으로 다시 뺏어 오지 않습니다. 연결을 누르면 이쪽으로 가져옵니다.
- 아무도 안 보고 있으면 H.264 인코딩을 멈춰 전력을 아낍니다 (스냅샷은 언제든 가능).

### HTTP API (다른 프로그램에서 쓰기)

| 경로 | 설명 |
|---|---|
| `GET /api/snapshot` | 실시간 해상도 JPEG |
| `GET /api/snapshot?hires=1` | 최대 해상도 JPEG (응답 헤더 `X-Resolution`, 실패 시 `X-Fallback: 1`) |
| `GET /api/status` | 카메라/인코더/WebRTC 상태 JSON |
| `GET /api/offer`, `POST /api/answer?session=N` | WebRTC 시그널링 (SDP 텍스트). offer 응답 헤더 `X-Session` 의 번호를 answer 에 붙입니다 |

```sh
curl -o shot.jpg "http://p4cam.local/api/snapshot?hires=1"
```

## 설정 (`idf.py menuconfig` → P4 USB 카메라 WebRTC)

| 항목 | 기본값 | 설명 |
|---|---|---|
| WiFi SSID / 비밀번호 | | 2.4GHz 공유기 (C6 는 WiFi 6 2.4GHz) |
| mDNS 이름 | `p4cam` | `http://p4cam.local/` |
| 실시간 해상도 | 1280x720 | 1920x1080 도 가능 (16 배수로 잘려서 1920x1072 로 인코딩) |
| 프레임레이트 | 30 | |
| H.264 비트레이트 | 2500 kbps | WiFi 가 약하면 낮추세요 (1500 정도) |
| 키프레임 간격 | 30 | 프레임레이트와 같게 두면 패킷 유실 후 1초 안에 복구 |
| USB 프레임 버퍼 크기 | 4096 KB | 16MP JPEG 한 장이 들어갈 크기. PSRAM 16MB 보드면 2048~3072 |
| USB 프레임 버퍼 개수 | 3 | |
| 고해상도 전환 후 버릴 프레임 | 2 | 전환 직후 어두운 프레임을 버림 |
| STUN 서버 | (비움) | 같은 공유기 안에서만 볼 거면 비워 두세요 |

## 동작 방식

- **USB 카메라 수신** — `usb_host_uvc` 드라이버로 MJPEG 프레임을 PSRAM 버퍼에 받습니다.
  카메라를 뺐다 꽂아도 자동으로 다시 붙습니다.
- **하드웨어로만 변환** — MJPEG 가 4:2:0 이면 JPEG 디코더가 H.264 인코더 입력 포맷(O_UYY_E_VYY)으로
  바로 뽑고, 4:2:2 같은 다른 샘플링이면 RGB565 로 디코드한 뒤 PPA(픽셀 처리 가속기)로 YUV420 변환을
  합니다. CPU 는 거의 쓰지 않습니다. 어느 경로를 쓰는지는 로그(`venc: ... 경로 사용`)와
  `/api/status` 의 `ppa_path` 로 확인할 수 있습니다.
- **WebRTC** — Espressif 의 `esp_peer` (ICE / DTLS-SRTP / RTP, NACK 재전송)로 H.264 를 보냅니다.
  시그널링은 보드의 HTTP 서버가 합니다: 브라우저가 `GET /api/offer` 로 보드의 SDP 를 받고
  `POST /api/answer` 로 자기 SDP 를 보냅니다. 같은 공유기 안에서는 host 후보끼리 바로 붙습니다.
- **키프레임** — 시청자가 붙는 순간, 브라우저가 PLI(키프레임 요청)를 보낼 때,
  고해상도 스냅샷 뒤에 IDR 을 강제로 넣어 화면이 바로 나오고 바로 복구됩니다.
- **스냅샷** — UVC 카메라의 MJPEG 는 보통 허프만 테이블(DHT)이 빠져 있어서 일부 뷰어가 못 엽니다.
  저장할 때 표준 DHT 를 끼워 넣어 어떤 뷰어에서든 열리게 합니다.

## 소스 구성

| 파일 | 내용 |
|---|---|
| `main/main.c` | 초기화, 카메라 → 인코더 → WebRTC 연결 |
| `main/camera.c` | USB 호스트 + UVC, 포맷 선택, 스냅샷(고해상도 전환 포함) |
| `main/video_enc.c` | JPEG 디코더 / PPA / H.264 인코더 (전부 하드웨어) |
| `main/webrtc.c` | esp_peer 래퍼 (시청자 1명, 송신 전용) |
| `main/http_server.c` | 웹 페이지, 시그널링, 스냅샷, 상태 API |
| `main/wifi.c` | WiFi STA (esp_hosted 로 C6 사용) + mDNS |
| `main/jpeg_util.c` | JPEG 헤더 파싱, 표준 DHT 삽입 |
| `main/sdp_util.c` | SDP 에서 ICE 후보 추출 |
| `main/web/index.html` | 브라우저 뷰어 (펌웨어에 내장) |

## 테스트 (보드 없이)

```sh
make -C esp32p4/test          # JPEG/DHT·SDP 유틸 단위 테스트 (gcc, python3 + Pillow)
make -C esp32p4/test web      # 웹 뷰어 브라우저 테스트 (node + playwright)
```

- 단위 테스트: Pillow 로 만든 4:2:0 / 4:2:2 JPEG 에서 DHT 를 빼(=MJPEG 프레임) 다시 끼운 결과가
  원본과 **픽셀 단위로 같게** 디코드되는지 확인합니다.
- 웹 테스트: 두 번째 브라우저 탭이 보드처럼 SDP offer 를 만들고 mock 서버가 `/api/*` 를 흉내 냅니다.
  연결 → 영상 재생 → 스냅샷 2종 → 저장 → 끊기 → 재연결 → 두 번째 기기 접속 시 서로 안 뺏는지까지
  실제 WebRTC 로 확인합니다.

## 문제 해결

| 증상 | 확인할 것 |
|---|---|
| WiFi 가 안 붙음 / `esp_hosted` 오류 | 보드의 ESP32-C6 펌웨어(esp-hosted 코프로세서)가 호스트 쪽 esp_hosted 버전과 맞아야 합니다. [esp-hosted-mcu](https://github.com/espressif/esp-hosted-mcu) 의 코프로세서 OTA 예제나 C6 직접 플래시로 업데이트하세요 |
| `USB 카메라를 기다리는 중...` 에서 멈춤 | 카메라를 HS 호스트 포트에 꽂았는지, 5V 전원이 충분한지. USB 허브를 쓰면 전원 있는 허브로 |
| 브라우저에서 "연결 중" 에서 안 넘어감 | PC/폰이 **같은 공유기**인지(게스트 WiFi 는 기기끼리 막혀 있음). 회사망처럼 UDP 가 막힌 곳은 안 됩니다 |
| 영상이 자주 멈추거나 깨짐 | 비트레이트를 1500 정도로 낮추거나 해상도를 1280x720 으로. 공유기와 거리 확인 |
| 고해상도 스냅샷이 실시간 해상도로 찍힘 | 프레임 버퍼보다 JPEG 가 큼(`프레임이 버퍼보다 큼` 로그). 버퍼 크기를 키우세요 |
| 색이 이상함(빨강/파랑 바뀜) | 4:2:2 MJPEG 를 PPA 로 변환하는 경로에서만 생길 수 있습니다. `main/video_enc.c` 의 `rgb_order` 를 `JPEG_DEC_RGB_ELEMENT_ORDER_RGB` 로 바꿔 보세요 |
