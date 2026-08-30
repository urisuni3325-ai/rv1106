# 보드 설정 자세히

Luckfox 공식 이미지(rkipc 포함)를 쓰는 RV1106 + SC3336 보드 기준입니다.

## 1. 보드에 접속하기

USB 로 PC 에 연결한 상태라면 보통 다음 중 하나로 들어갑니다.

```sh
# ADB (Luckfox 이미지에 기본 포함)
adb shell

# USB 시리얼 (예: /dev/ttyUSB0, 115200 8N1)
picocom -b 115200 /dev/ttyUSB0
```

USB 이더넷(RNDIS)이 잡혀 있으면 `ssh root@172.32.0.93` 같은 주소로도 들어갑니다.
Luckfox 기본 계정은 보통 `root` / `luckfox` 입니다.

## 2. 스크립트 복사

```sh
# PC 에서
adb push device/scripts /root
adb shell chmod +x /root/scripts/*.sh
```

## 3. WiFi 연결

```sh
cd /root/scripts
./wifi-setup.sh -s "공유기이름" -p "비밀번호"
```

스크립트가 하는 일:

1. `wlan*` 인터페이스 자동 탐지
2. `/etc/wpa_supplicant.conf` 작성 (기존 파일은 백업). `wpa_passphrase` 가 있으면
   평문 비밀번호 대신 PSK 해시로 저장
3. 기존 `wpa_supplicant`/`udhcpc` 정리 후 재접속
4. `udhcpc` 로 IP 할당
5. 앱에 넣을 RTSP 주소 출력

### 인터페이스가 안 보일 때

```sh
dmesg | grep -i -E 'aic|wifi|wlan|sdio'
ls /sys/class/net/
lsmod
```

내장 WiFi 모듈(Pico Ultra W 는 AIC8800 계열) 드라이버가 안 올라온 것이므로,
쓰고 있는 이미지에 해당 모듈이 포함돼 있는지 확인해야 합니다.

### 부팅할 때 자동 접속

```sh
./wifi-autostart.sh -i wlan0
```

`/etc/init.d/S80wifi` 를 설치합니다. 공유기가 보드보다 늦게 켜지는 경우를 대비해
연결 확인을 15초까지 재시도합니다.

### 고정 IP 로 쓰기

폰에서 매번 주소를 바꾸기 싫으면 공유기에서 이 보드의 MAC 에 DHCP 고정 할당을
걸어두는 게 가장 편합니다. 보드 쪽에서 직접 고정하려면:

```sh
ifconfig wlan0 192.168.0.50 netmask 255.255.255.0
route add default gw 192.168.0.1
```

## 4. 스트리밍 확인

```sh
./check-stream.sh
```

정상이면 이런 항목이 확인됩니다.

- `wlan0` 에 IP 가 붙어 있음
- `rkipc` 프로세스가 떠 있음
- `/dev/video*` 가 존재
- 554 포트가 열려 있음

## 5. rkipc 설정 바꾸기

```sh
./stream-tune.sh --show                 # 현재 값 보기
./stream-tune.sh --preset wifi          # WiFi 용 값 적용 후 rkipc 재시작
./stream-tune.sh --width 1280 --height 720 --fps 30 --bitrate 2048 --gop 30
```

- rkipc 버전에 따라 `rkipc.ini` 의 키 이름이 다릅니다. 스크립트는 **이미 존재하는
  키만** 수정하고, 못 찾은 키는 이름을 알려줍니다. 그 경우 `--show` 로 실제 키
  목록을 보고 대응하는 이름을 직접 고치세요.
- 원본은 `rkipc.ini.bak.<타임스탬프>` 로 백업됩니다. 되돌리려면 그 파일을 덮어쓰고
  rkipc 를 재시작하면 됩니다.
- `[video.0]` 이 주 스트림, `[video.1]` 은 보조(저해상도) 스트림인 경우가 많습니다.
  주소도 보통 `/live/0`, `/live/1` 로 나뉩니다. 폰에서 저해상도로 가볍게 보고 싶으면
  앱 설정에서 `/live/1` 을 쓰면 됩니다.

## 6. 인증이 걸려 있는 경우

rkipc 설정에서 RTSP 인증을 켜 두었다면 앱 설정 화면의 아이디/비밀번호 칸을 채우면
됩니다. Basic 과 Digest(MD5) 둘 다 지원합니다.
