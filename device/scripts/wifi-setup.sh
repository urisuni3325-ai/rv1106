#!/bin/sh
# RV1106 보드(Luckfox Pico Ultra W 등) 내장 WiFi 를 공유기에 붙인다.
#
#   ./wifi-setup.sh -s "우리집공유기" -p "비밀번호"
#   ./wifi-setup.sh -s "OpenAP"                  # 암호 없는 AP
#
# 보드에서 root 로 실행한다. BusyBox 환경(ash)에서 동작하도록 작성했다.
set -e

SSID=""
PSK=""
IFACE=""
COUNTRY="KR"
CONF="/etc/wpa_supplicant.conf"

usage() {
    echo "사용법: $0 -s SSID [-p 비밀번호] [-i 인터페이스] [-c 국가코드]" >&2
    exit 1
}

while [ $# -gt 0 ]; do
    case "$1" in
        -s) SSID="$2"; shift 2 ;;
        -p) PSK="$2"; shift 2 ;;
        -i) IFACE="$2"; shift 2 ;;
        -c) COUNTRY="$2"; shift 2 ;;
        -h|--help) usage ;;
        *) echo "알 수 없는 옵션: $1" >&2; usage ;;
    esac
done

[ -n "$SSID" ] || usage
[ "$(id -u)" = "0" ] || { echo "root 로 실행하세요." >&2; exit 1; }

# 인터페이스 자동 탐지
if [ -z "$IFACE" ]; then
    for i in /sys/class/net/wlan*; do
        [ -e "$i" ] || continue
        IFACE=$(basename "$i")
        break
    done
fi
if [ -z "$IFACE" ]; then
    echo "무선 인터페이스(wlan*)를 찾지 못했습니다." >&2
    echo "내장 WiFi 드라이버가 올라왔는지 확인하세요:  dmesg | grep -i -E 'aic|wifi|wlan'" >&2
    exit 1
fi
echo "[1/5] 인터페이스: $IFACE"

# 설정 파일 작성
echo "[2/5] $CONF 작성"
[ -f "$CONF" ] && cp "$CONF" "$CONF.bak.$(date +%s)"
{
    echo "ctrl_interface=/var/run/wpa_supplicant"
    echo "update_config=1"
    echo "country=$COUNTRY"
    echo ""
    if [ -n "$PSK" ]; then
        if command -v wpa_passphrase >/dev/null 2>&1; then
            # 평문 비밀번호 대신 PSK 해시로 저장한다.
            wpa_passphrase "$SSID" "$PSK" | grep -v '^\s*#'
        else
            echo "network={"
            echo "    ssid=\"$SSID\""
            echo "    psk=\"$PSK\""
            echo "}"
        fi
    else
        echo "network={"
        echo "    ssid=\"$SSID\""
        echo "    key_mgmt=NONE"
        echo "}"
    fi
} > "$CONF"
chmod 600 "$CONF"

# 기존 프로세스 정리
echo "[3/5] 기존 연결 정리"
killall wpa_supplicant 2>/dev/null || true
killall udhcpc 2>/dev/null || true
sleep 1
ifconfig "$IFACE" up

# 접속
echo "[4/5] $SSID 에 접속"
wpa_supplicant -B -i "$IFACE" -c "$CONF" -D nl80211,wext

CONNECTED=0
i=0
while [ $i -lt 20 ]; do
    if iwconfig "$IFACE" 2>/dev/null | grep -q "ESSID:\"$SSID\""; then CONNECTED=1; break; fi
    if wpa_cli -i "$IFACE" status 2>/dev/null | grep -q "wpa_state=COMPLETED"; then CONNECTED=1; break; fi
    i=$((i + 1))
    sleep 1
done
[ "$CONNECTED" = "1" ] || echo "  경고: 연결 확인에 실패했습니다. 비밀번호와 SSID 를 확인하세요."

# 주소 받기
echo "[5/5] DHCP 주소 요청"
udhcpc -i "$IFACE" -n -q -t 8 || {
    echo "DHCP 실패. 고정 IP 로 쓰려면 예: ifconfig $IFACE 192.168.0.50 netmask 255.255.255.0" >&2
    exit 1
}

IP=$(ifconfig "$IFACE" | sed -n 's/.*inet addr:\([0-9.]*\).*/\1/p')
[ -n "$IP" ] || IP=$(ip -4 addr show "$IFACE" 2>/dev/null | sed -n 's/.*inet \([0-9.]*\).*/\1/p')

echo ""
echo "연결 완료. 보드 IP: $IP"
echo "핸드폰 앱 설정에 넣을 주소:  rtsp://$IP:554/live/0"
echo ""
echo "부팅할 때마다 자동으로 붙게 하려면:  ./wifi-autostart.sh -i $IFACE"
