#!/bin/sh
# 보드가 스트리밍할 준비가 됐는지 한 번에 점검한다.
echo "=========== RV1106 스트리밍 점검 ==========="

echo ""
echo "[네트워크]"
for i in /sys/class/net/*; do
    N=$(basename "$i")
    [ "$N" = "lo" ] && continue
    IP=$(ifconfig "$N" 2>/dev/null | sed -n 's/.*inet addr:\([0-9.]*\).*/\1/p')
    STATE=$(cat "$i/operstate" 2>/dev/null)
    echo "  $N: ip=${IP:-없음} state=${STATE:-?}"
done

WLAN=""
for i in /sys/class/net/wlan*; do
    [ -e "$i" ] || continue
    WLAN=$(basename "$i")
    break
done
if [ -n "$WLAN" ]; then
    echo "  WiFi 상태: $(wpa_cli -i "$WLAN" status 2>/dev/null | grep -E 'wpa_state|ssid=' | tr '\n' ' ')"
    echo "  신호 세기: $(iwconfig "$WLAN" 2>/dev/null | sed -n 's/.*Signal level[=:]\([^ ]*\).*/\1/p')"
else
    echo "  경고: 무선 인터페이스가 없습니다. wifi-setup.sh 를 먼저 실행하세요."
fi

echo ""
echo "[카메라 / 스트리밍 프로세스]"
if pgrep rkipc >/dev/null 2>&1; then
    echo "  rkipc: 실행 중 (pid $(pgrep rkipc | tr '\n' ' '))"
else
    echo "  rkipc: 실행 중이 아님"
    echo "    -> RkLunch.sh 또는 /etc/init.d 의 rkipc 스크립트로 시작하세요."
fi
ls /dev/video* 2>/dev/null | sed 's/^/  /' || echo "  /dev/video* 없음 — 센서 드라이버를 확인하세요"

echo ""
echo "[RTSP 포트]"
if netstat -ltn 2>/dev/null | grep -q ':554 '; then
    echo "  554 포트 열림"
else
    echo "  554 포트가 열려 있지 않습니다. 스트리밍 서버가 떠 있는지 확인하세요."
fi

echo ""
echo "[부하]"
uptime 2>/dev/null | sed 's/^/  /'
free 2>/dev/null | sed -n '2p' | sed 's/^/  /'

IP=""
[ -n "$WLAN" ] && IP=$(ifconfig "$WLAN" 2>/dev/null | sed -n 's/.*inet addr:\([0-9.]*\).*/\1/p')
echo ""
if [ -n "$IP" ]; then
    echo "앱에 넣을 주소:  rtsp://$IP:554/live/0"
    echo "PC 에서 먼저 확인:  ffplay -fflags nobuffer -rtsp_transport tcp rtsp://$IP:554/live/0"
else
    echo "WiFi IP 를 확인하지 못했습니다."
fi
echo "==========================================="
