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

if [ "$CONNECTED" != "1" ]; then
    # 붙지 못한 상태에서 DHCP 를 돌리면 아무 응답 없이 오래 걸리기만 한다.
    # 여기서 멈추고 원인을 좁혀 준다.
    echo ""
    echo "접속하지 못했습니다. 아래 정보로 원인을 좁혀 보세요."
    echo ""
    echo "--- 현재 상태 ---"
    wpa_cli -i "$IFACE" status 2>/dev/null | grep -E 'wpa_state|^ssid|bssid|freq' || echo "  상태를 읽지 못했습니다."

    echo ""
    echo "--- 주변 공유기 (bssid / 주파수 / 신호 / 보안 / 이름) ---"
    wpa_cli -i "$IFACE" scan >/dev/null 2>&1
    sleep 3
    SCAN=$(wpa_cli -i "$IFACE" scan_results 2>/dev/null)
    if [ -z "$SCAN" ] || [ "$(echo "$SCAN" | wc -l)" -le 1 ]; then
        echo "  하나도 잡히지 않았습니다. 드라이버나 안테나 문제일 수 있습니다."
        echo "  확인:  dmesg | tail -30"
        exit 1
    fi
    echo "$SCAN" | head -20

    echo ""
    LINE=$(echo "$SCAN" | grep "	$SSID\$" | head -1)
    if [ -n "$LINE" ]; then
        FLAGS=$(echo "$LINE" | awk '{print $4}')
        echo "'$SSID' 은(는) 보입니다 (보안: $FLAGS)."
        case "$FLAGS" in
            *SAE*)
                echo "  WPA3(SAE)가 켜져 있습니다. 비밀번호가 맞아도 붙지 않을 수 있으니"
                echo "  공유기 설정에서 보안을 WPA2 전용으로 바꿔 보세요."
                ;;
            *)
                echo "  공유기는 보이는데 인증이 안 된 것이므로 비밀번호일 가능성이 큽니다."
                echo "  공유기 밑면 스티커에서 0(영)과 O, 1(일)과 l 을 특히 확인하세요."
                ;;
        esac
    else
        echo "'$SSID' 이(가) 목록에 없습니다."
        echo "  - 이름 철자를 확인하세요(대소문자를 구분합니다)."
        if echo "$SCAN" | awk 'NR>1 {print $2}' | grep -q '^5'; then
            echo "  - 이 보드는 5GHz 도 볼 수 있습니다. 공유기가 꺼져 있거나 너무 멀 수 있습니다."
        else
            echo "  - 위 목록에 5000번대 주파수가 없으니 이 보드는 2.4GHz 전용입니다."
            echo "    5GHz 이름을 넣었다면 2.4GHz 쪽 이름으로 바꾸세요."
        fi
        echo "  - 보드를 공유기 가까이 옮겨서 다시 시도해 보세요."
    fi
    echo ""
    echo "고친 뒤 같은 명령을 다시 실행하면 됩니다."
    exit 1
fi

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
