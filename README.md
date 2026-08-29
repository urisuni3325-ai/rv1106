# RV1106 + SC3336 WiFi 실시간 영상 → 안드로이드 앱

RV1106 보드(Luckfox Pico Ultra W 등)에 붙은 SC3336 카메라 영상을 WiFi 로 실시간
전송해서 안드로이드 폰으로 보고, **녹화 / 순간 캡처 / 저장한 영상 다시 보기**까지
하는 프로젝트입니다.

```
 SC3336 ──MIPI──> RV1106 ──H.264 인코딩──> RTSP 서버(rkipc)
                                              │
                                          WiFi(공유기)
                                              │
                                    안드로이드 앱 (이 저장소)
                                    ├── 실시간 보기 (MediaCodec 하드웨어 디코딩)
                                    ├── 녹화 (재인코딩 없이 MP4 저장)
                                    ├── 캡처 (JPEG 저장)
                                    └── 갤러리 (저장한 영상/사진 재생·삭제·공유)
```

USB 로 PC 에서 보던 것과 달리, 보드는 **표준 RTSP 서버**가 되고 앱이 그 스트림을
받아갑니다. 같은 공유기(WiFi)에 있으면 폰뿐 아니라 PC 의 VLC/ffplay 로도 같은
주소로 볼 수 있습니다.

## 저장소 구성

| 경로 | 내용 |
|---|---|
| `android/` | 안드로이드 앱 (Kotlin). 외부 스트리밍 라이브러리 없이 RTSP/RTP 를 직접 구현 |
| `device/scripts/` | 보드에서 실행하는 WiFi 접속 / 스트리밍 설정 / 점검 스크립트 |
| `docs/` | 보드 설정, 지연 시간 튜닝, 문제 해결 가이드 |

## 빠른 시작

> 처음이라면 [**처음부터 끝까지 따라 하기**](docs/getting-started.md) 를 보세요.
> 프로그램 설치부터 앱 실행까지 하나씩 짚어 둔 초보자용 가이드입니다.
> 아래는 익숙한 분을 위한 요약입니다.

### 1. 보드를 WiFi 에 연결

USB 시리얼이나 ADB 로 보드에 접속한 뒤, `device/scripts` 를 보드에 복사하고:

```sh
./wifi-setup.sh -s "공유기이름" -p "비밀번호"
./wifi-autostart.sh            # 부팅할 때마다 자동 접속
./check-stream.sh              # IP, rkipc, 554 포트 한 번에 점검
```

`check-stream.sh` 가 마지막에 출력하는 `rtsp://<보드IP>:554/live/0` 이 앱에 넣을
주소입니다.

### 2. PC 에서 먼저 확인 (권장)

앱을 설치하기 전에 스트림 자체가 정상인지 확인해 두면 문제를 반으로 줄일 수 있습니다.

```sh
ffplay -fflags nobuffer -flags low_delay -rtsp_transport tcp rtsp://<보드IP>:554/live/0
```

### 3. WiFi 에 맞게 화질/지연 설정 (선택)

```sh
./stream-tune.sh --preset wifi        # 1920x1080 / 25fps / 3Mbps / GOP 25
./stream-tune.sh --preset lowlatency  # 1280x720 / 30fps / 2Mbps / GOP 30
```

GOP(키프레임 간격)를 프레임레이트와 비슷하게 잡는 게 중요합니다. 앱이 접속하자마자
화면이 뜨고, 패킷이 유실돼도 1초 안에 복구됩니다.

### 4. 앱 빌드 & 설치

안드로이드 스튜디오로 `android/` 폴더를 열고 실행하거나:

```sh
cd android
./gradlew assembleDebug
adb install -r app/build/outputs/apk/debug/app-debug.apk
```

앱을 열고 **설정** 버튼 → RTSP 주소에 `rtsp://<보드IP>:554/live/0` 입력 → 저장.

## 앱 사용법

| 버튼 | 동작 |
|---|---|
| 연결 / 끊기 | 스트림 접속. 끊기면 자동으로 재연결을 시도합니다 |
| ● 녹화 | 받은 영상을 그대로 MP4 로 저장. 다시 누르면 정지 |
| 캡처 | 현재 화면을 JPEG 로 저장 |
| 갤러리 | 저장한 영상/사진 목록. 눌러서 재생, 길게 눌러 내보내기·공유·삭제 |
| 설정 | RTSP 주소, 아이디/비밀번호, 자동 연결 |

저장 위치는 앱 전용 폴더라 **저장소 권한이 필요 없습니다**.
다른 앱(기본 갤러리 등)에서도 보고 싶으면 갤러리 화면에서 항목을 길게 눌러
**갤러리로 내보내기**를 선택하세요.

- 영상: `Android/data/com.rv1106.camview/files/Movies/recordings/REC_*.mp4`
- 사진: `Android/data/com.rv1106.camview/files/Pictures/snapshots/IMG_*.jpg`

## 앱이 이렇게 만들어져 있습니다

- **RTSP 는 TCP interleaved 만 사용합니다.** UDP 를 쓰지 않아 공유기/NAT 환경에서
  안 붙는 문제가 없고, 패킷 유실도 훨씬 적습니다.
- **녹화는 재인코딩이 없습니다.** 보드가 보낸 H.264 를 그대로 MP4 컨테이너에
  담기 때문에 폰이 뜨겁지 않고 배터리도 거의 안 먹으며 원본 화질이 그대로 남습니다.
  대신 녹화 버튼을 누른 뒤 첫 키프레임이 올 때까지(최대 GOP 1개 길이) "준비 중"이
  표시됩니다.
- **지연을 줄이려고** 디코더 출력은 타임스탬프를 기다리지 않고 바로 화면에 그리고,
  입력이 밀리면 다음 키프레임까지 프레임을 버립니다. API 30+ 에서는
  `KEY_LOW_LATENCY` 도 켭니다.
- **패킷이 유실되면** 그 액세스 유닛을 통째로 버리고 다음 키프레임부터 재개합니다.
  깨진 화면(녹색 블록)이 남지 않습니다.
- 외부 스트리밍 라이브러리(ExoPlayer, VLC, ijkplayer)를 쓰지 않습니다. RTSP 협상,
  RTP 재조립(FU-A/STAP-A), SPS 파싱을 직접 구현해 두어서 지연·녹화 동작을 원하는
  대로 제어할 수 있습니다.

## 테스트

RTSP/H.264 처리 로직에는 JVM 단위 테스트가 붙어 있습니다(보드나 폰 없이 실행 가능).

```sh
cd android
./gradlew test
```

- `H264SpsParserTest` — SPS 에서 해상도 읽기(크롭이 있는 1080p, high profile 포함)
- `RtpH264DepacketizerTest` — FU-A 재조립, STAP-A, 패킷 유실 시 키프레임까지 건너뛰기
- `DigestAuthTest` — RTSP Digest 인증(RFC 2617 예제 벡터로 검증)
- `SdpInfoTest` — SDP 에서 비디오 트랙 control/payload type 찾기

## 더 읽을 거리

- **[처음부터 끝까지 따라 하기](docs/getting-started.md)** — 초보자용 단계별 가이드
- [보드 설정 자세히](docs/board-setup.md)
- [지연 시간 줄이기](docs/latency-tuning.md)
- [문제 해결](docs/troubleshooting.md)
