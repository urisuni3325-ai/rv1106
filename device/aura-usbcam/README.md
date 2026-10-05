# Luckfox Aura + USB 카메라 → 패드에서 실시간 보기

Luckfox Aura(RV1126B)에 꽂은 USB 카메라(Arducam B0290, IMX298 16MP AF) 영상을
WiFi 로 패드에서 봅니다.

```
USB 카메라 ─MJPEG→ Aura ─H.264 RTSP→ WiFi(공유기) → 패드
                     rtsp://<보드IP>:554/live/0
```

주소의 포트와 경로가 이 저장소 안드로이드 앱의 기본값과 같아서, 앱에는 **보드 IP 만**
바꿔 넣으면 됩니다.

## 준비물

- Luckfox Aura, **Debian 13 이미지**로 설치된 상태
- USB 카메라를 Aura 의 **USB 2.0 포트**에 연결
- 패드와 Aura 가 **같은 공유기**에 연결

## 1. 이미지 확인

보드에 접속(SSH 또는 시리얼)해서 확인합니다.

```sh
cat /etc/os-release     # Debian 이 나와야 합니다
```

Buildroot 이미지라면 `apt-get` 이 없어서 아래 설치 스크립트가 동작하지 않습니다.
Luckfox 위키의 안내대로 Debian 13 이미지를 올리세요.

## 2. 카메라 인식 확인

```sh
lsusb                                   # Arducam / IMX298 장치가 보여야 함
ls /dev/video*
v4l2-ctl --list-devices                 # v4l-utils 가 없으면 3단계 후에 다시
```

## 3. WiFi 연결

```sh
sudo nmcli device wifi connect "공유기이름" password "비밀번호"
ip -4 addr show wlan0                   # inet 뒤의 주소가 보드 IP
```

한 번 연결하면 다음 부팅부터 자동으로 다시 붙습니다.

## 4. 설치 (한 번만)

이 폴더를 보드에 복사한 뒤 실행합니다.

```sh
# PC 에서
scp -r device/aura-usbcam <사용자>@<보드IP>:~/

# 보드에서
cd ~/aura-usbcam
sudo ./install.sh
```

설치 스크립트가 하는 일:

1. GStreamer 와 RTSP 서버 패키지 설치
2. Rockchip 하드웨어 가속 플러그인 확인
3. `/opt/usbcam/` 에 서버 설치, 부팅 시 자동 실행 등록
4. 서버 시작 후 로그 출력

로그에 이런 줄이 나오면 성공입니다.

```
[usbcam] 카메라: ... (/dev/video0)
[usbcam] 앱에 넣을 주소: rtsp://192.168.0.23:554/live/0
```

## 5. 패드에서 보기

- **안드로이드 패드:** 이 저장소의 앱 → 설정 → RTSP 주소에 위 주소를 넣고 저장.
  설정의 보드 찾기(554 포트 스캔)로도 찾을 수 있습니다.
- **아이패드 또는 앱 없이 확인:** VLC 앱 → 네트워크 스트림 → 위 주소 입력.

## 설정 바꾸기

`/etc/default/usbcam-rtsp` 에 옵션을 넣고 서비스를 다시 시작합니다.

```sh
sudo nano /etc/default/usbcam-rtsp
#   USBCAM_ARGS="--bitrate 6000"
sudo systemctl restart usbcam-rtsp
```

| 옵션 | 기본값 | 설명 |
|---|---|---|
| `--width` `--height` | 하드웨어 1920×1080, 소프트웨어 1280×720 | 카메라에 요청할 해상도 (MJPEG 지원 해상도만) |
| `--fps` | 30 | 프레임 수 |
| `--bitrate` | 하드웨어 4000, 소프트웨어 2500 | kbps. 화질이 뭉개지면 올리고, WiFi 가 끊기면 내림 |
| `--gop` | fps 와 같음 (1초) | 키프레임 간격. 짧을수록 접속 직후 화면이 빨리 뜸 |
| `--device` | 자동 검색 | 예: `/dev/video2` |
| `--port` | 554 | |

카메라가 지원하는 해상도는 `v4l2-ctl --list-formats-ext` 로 확인합니다.
B0290 은 1080p·720p 가 30 fps, 그 이상은 10 fps 입니다.

## 하드웨어 가속

로그에 `encoder: x264enc (소프트웨어)` 가 나오면 Rockchip 하드웨어 플러그인이 없는
상태입니다. 그래도 720p30 으로 동작하지만 CPU 를 많이 씁니다(`top` 으로 확인).

`mppjpegdec` 와 `mpph264enc` 가 있으면 자동으로 하드웨어를 써서 **1080p30**을
CPU 부담 없이 처리합니다. 확인:

```sh
gst-inspect-1.0 mpph264enc
gst-inspect-1.0 mppjpegdec
```

없으면 Luckfox 위키의 Debian 13 항목에서 GStreamer Rockchip(rkmpp) 플러그인 설치
방법을 확인하세요. 설치한 뒤 `sudo systemctl restart usbcam-rtsp` 만 하면
하드웨어 경로로 바뀝니다.

## 문제 해결

| 증상 | 확인 |
|---|---|
| `MJPEG USB 카메라를 찾지 못했습니다` | `lsusb`, `ls /dev/video*`. 다른 USB 포트에 꽂아 보기 |
| `554 포트를 열지 못했습니다` | `sudo ss -ltnp \| grep :554` 로 이미 쓰는 프로그램 확인 (rkipc 등) |
| 앱이 접속은 되는데 화면이 안 나옴 | `journalctl -u usbcam-rtsp -f` 에서 오류 확인. `--width 1280 --height 720` 으로 낮춰 보기 |
| 화면이 자주 멈춤 | 공유기와 거리, 5 GHz 연결 여부 확인. `--bitrate 2000` 으로 낮춰 보기 |
| 서비스 상태 | `systemctl status usbcam-rtsp` |

## 카메라 없이 시험

카메라가 없어도 시험 영상으로 앱 연결을 확인할 수 있습니다.

```sh
sudo systemctl stop usbcam-rtsp
sudo python3 /opt/usbcam/usbcam_rtsp.py --source test
```

움직이는 공이 보이면 네트워크와 앱 쪽은 정상입니다.

## 다음 단계

- 16MP 사진 캡처 (카메라의 JPEG 를 그대로 저장해 앱으로 전송)
- 브라우저에서 바로 보는 WebRTC
- BLE 로 WiFi 설정, LED 켜고 끄기, 배터리 상태
