#!/bin/sh
# 부팅할 때 wifi-setup.sh 가 만들어 둔 설정으로 자동 접속하게 만든다.
# /etc/init.d/S80wifi 를 설치한다(Luckfox 이미지의 BusyBox init 방식).
set -e

IFACE="wlan0"
while [ $# -gt 0 ]; do
    case "$1" in
        -i) IFACE="$2"; shift 2 ;;
        *) echo "사용법: $0 [-i 인터페이스]" >&2; exit 1 ;;
    esac
done

[ "$(id -u)" = "0" ] || { echo "root 로 실행하세요." >&2; exit 1; }
[ -f /etc/wpa_supplicant.conf ] || {
    echo "/etc/wpa_supplicant.conf 가 없습니다. 먼저 wifi-setup.sh 를 실행하세요." >&2
    exit 1
}

TARGET=/etc/init.d/S80wifi
cat > "$TARGET" <<EOF
#!/bin/sh
IFACE="$IFACE"

case "\$1" in
    start)
        echo "WiFi(\$IFACE) 접속 중..."
        ifconfig "\$IFACE" up
        wpa_supplicant -B -i "\$IFACE" -c /etc/wpa_supplicant.conf -D nl80211,wext
        # 공유기가 뜨기 전에 부팅이 끝나는 경우가 있어 몇 번 재시도한다.
        i=0
        while [ \$i -lt 15 ]; do
            wpa_cli -i "\$IFACE" status 2>/dev/null | grep -q "wpa_state=COMPLETED" && break
            i=\$((i + 1))
            sleep 1
        done
        udhcpc -i "\$IFACE" -b -t 10
        ;;
    stop)
        killall udhcpc 2>/dev/null
        killall wpa_supplicant 2>/dev/null
        ifconfig "\$IFACE" down 2>/dev/null
        ;;
    restart)
        \$0 stop
        sleep 1
        \$0 start
        ;;
    *)
        echo "사용법: \$0 {start|stop|restart}"
        exit 1
        ;;
esac
EOF
chmod +x "$TARGET"
echo "$TARGET 설치 완료. 다음 부팅부터 자동으로 접속합니다."
echo "지금 바로 시험하려면:  $TARGET restart"
