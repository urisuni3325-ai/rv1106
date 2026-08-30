# 보드(RV1106) 쪽 스크립트

Luckfox 공식 이미지(rkipc 포함)를 쓰는 보드에서 실행합니다.
BusyBox 환경에서 동작하도록 POSIX `sh` 로 작성했습니다.

보드에 복사하고 실행 권한을 줍니다.

```sh
adb push device/scripts /root
adb shell chmod +x /root/scripts/*.sh
```

| 스크립트 | 하는 일 |
|---|---|
| `wifi-setup.sh` | 내장 WiFi 를 공유기에 연결하고 앱에 넣을 RTSP 주소를 출력 |
| `wifi-autostart.sh` | `/etc/init.d/S80wifi` 를 설치해 부팅 시 자동 접속 |
| `check-stream.sh` | IP·WiFi 신호·rkipc·`/dev/video*`·554 포트를 한 번에 점검 |
| `stream-tune.sh` | rkipc.ini 의 해상도/프레임레이트/비트레이트/GOP 수정 후 재시작 |

```sh
cd /root/scripts
./wifi-setup.sh -s "공유기이름" -p "비밀번호"
./wifi-autostart.sh
./stream-tune.sh --preset wifi
./check-stream.sh
```

## 안전장치

- `wifi-setup.sh` 와 `stream-tune.sh` 는 수정 전 원본을 `*.bak.<타임스탬프>` 로
  백업합니다.
- `stream-tune.sh` 는 지정한 섹션에 **이미 존재하는 키만** 바꿉니다. rkipc 버전에
  따라 키 이름이 다르기 때문에, 못 찾은 키는 이름을 알려주고 그냥 둡니다.
- 잘못 바꿨을 때 되돌리기:
  ```sh
  cp /etc/rkipc.ini.bak.<타임스탬프> /etc/rkipc.ini
  RkLunch-stop.sh; RkLunch.sh &
  ```

자세한 내용은 [docs/board-setup.md](../docs/board-setup.md) 를 보세요.
