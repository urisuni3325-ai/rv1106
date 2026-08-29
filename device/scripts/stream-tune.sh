#!/bin/sh
# Luckfox 공식 이미지의 rkipc 스트리밍 설정을 바꾼다(해상도/비트레이트/GOP).
#
#   ./stream-tune.sh --preset wifi          # WiFi 로 폰에서 보기 좋은 값
#   ./stream-tune.sh --preset quality       # 화질 우선
#   ./stream-tune.sh --width 1280 --height 720 --bitrate 2048 --gop 25
#   ./stream-tune.sh --show                 # 지금 값만 보기
#
# GOP 를 프레임레이트와 비슷하게(=1초) 잡는 게 핵심이다. 앱이 붙자마자 화면이
# 나오고, 패킷이 유실돼도 1초 안에 복구된다.
set -e

SECTION="video.0"
WIDTH=""; HEIGHT=""; FPS=""; BITRATE=""; GOP=""
SHOW_ONLY=0
NO_RESTART=0
CONF=""

usage() {
    cat >&2 <<EOF
사용법: $0 [옵션]
  --preset wifi|quality|lowlatency   미리 정해둔 값 적용
  --width N --height N               해상도
  --fps N                            프레임레이트
  --bitrate N                        비트레이트(kbps)
  --gop N                            키프레임 간격(프레임 수)
  --section NAME                     설정 섹션 (기본: video.0)
  --config PATH                      rkipc.ini 경로 직접 지정
  --show                             현재 값만 출력
  --no-restart                       설정만 바꾸고 rkipc 를 재시작하지 않음
EOF
    exit 1
}

while [ $# -gt 0 ]; do
    case "$1" in
        --preset)
            case "$2" in
                wifi)       WIDTH=1920; HEIGHT=1080; FPS=25; BITRATE=3072; GOP=25 ;;
                quality)    WIDTH=2304; HEIGHT=1296; FPS=25; BITRATE=6144; GOP=50 ;;
                lowlatency) WIDTH=1280; HEIGHT=720;  FPS=30; BITRATE=2048; GOP=30 ;;
                *) echo "알 수 없는 프리셋: $2" >&2; usage ;;
            esac
            shift 2 ;;
        --width)    WIDTH="$2"; shift 2 ;;
        --height)   HEIGHT="$2"; shift 2 ;;
        --fps)      FPS="$2"; shift 2 ;;
        --bitrate)  BITRATE="$2"; shift 2 ;;
        --gop)      GOP="$2"; shift 2 ;;
        --section)  SECTION="$2"; shift 2 ;;
        --config)   CONF="$2"; shift 2 ;;
        --show)     SHOW_ONLY=1; shift ;;
        --no-restart) NO_RESTART=1; shift ;;
        -h|--help)  usage ;;
        *) echo "알 수 없는 옵션: $1" >&2; usage ;;
    esac
done

# 설정 파일 찾기
if [ -z "$CONF" ]; then
    for c in /etc/rkipc.ini /userdata/rkipc.ini /oem/usr/share/rkipc.ini /oem/etc/rkipc.ini /tmp/rkipc.ini; do
        [ -f "$c" ] && { CONF="$c"; break; }
    done
fi
if [ -z "$CONF" ] || [ ! -f "$CONF" ]; then
    echo "rkipc.ini 를 찾지 못했습니다. --config 로 직접 지정하세요." >&2
    echo "찾아보기:  find / -name 'rkipc*.ini' 2>/dev/null" >&2
    exit 1
fi
echo "설정 파일: $CONF"

show_section() {
    awk -v sect="[$SECTION]" '
        { t=$0; gsub(/^[ \t]+|[ \t]+$/,"",t)
          if (t ~ /^\[.*\]$/) { insect = (t==sect); if (insect) print "  " t; next }
          if (insect && t != "") print "  " t }
    ' "$CONF"
}

if [ "$SHOW_ONLY" = "1" ]; then
    echo "현재 [$SECTION] 값:"
    show_section
    exit 0
fi

# 바꿀 키 목록 만들기 (rkipc.ini 의 키 이름 기준)
KV=""
[ -n "$WIDTH" ]   && KV="$KV;width=$WIDTH"
[ -n "$HEIGHT" ]  && KV="$KV;height=$HEIGHT"
[ -n "$FPS" ]     && KV="$KV;frame_rate=$FPS"
[ -n "$BITRATE" ] && KV="$KV;max_rate=$BITRATE"
[ -n "$GOP" ]     && KV="$KV;gop=$GOP"
[ -n "$KV" ] || { echo "바꿀 값이 없습니다." >&2; usage; }

BACKUP="$CONF.bak.$(date +%s)"
cp "$CONF" "$BACKUP"
echo "백업: $BACKUP"

TMP="$CONF.tmp.$$"
awk -v sect="[$SECTION]" -v kv="$KV" '
BEGIN {
    n = split(kv, arr, ";")
    for (i = 1; i <= n; i++) {
        if (arr[i] == "") continue
        p = index(arr[i], "=")
        want[substr(arr[i], 1, p - 1)] = substr(arr[i], p + 1)
    }
}
{
    t = $0
    gsub(/^[ \t]+|[ \t]+$/, "", t)
    if (t ~ /^\[.*\]$/) { insect = (t == sect); print; next }
    if (insect && index(t, "=") > 0 && substr(t, 1, 1) != "#") {
        p = index(t, "=")
        k = substr(t, 1, p - 1)
        gsub(/[ \t]/, "", k)
        if (k in want) {
            print k " = " want[k]
            found[k] = 1
            next
        }
    }
    print
}
END {
    for (k in want) if (!(k in found)) print "MISSING " k > "/dev/stderr"
}
' "$CONF" > "$TMP" 2> "$TMP.miss"

if [ -s "$TMP.miss" ]; then
    echo ""
    echo "주의: [$SECTION] 안에서 다음 키를 찾지 못해 그냥 두었습니다:"
    sed 's/^MISSING /  - /' "$TMP.miss"
    echo "  rkipc 버전마다 키 이름이 다릅니다. 아래 목록에서 대응하는 이름을 찾아"
    echo "  직접 수정하세요:  $0 --config $CONF --section $SECTION --show"
fi
rm -f "$TMP.miss"
mv "$TMP" "$CONF"

echo ""
echo "적용 후 [$SECTION] 값:"
show_section

if [ "$NO_RESTART" = "1" ]; then
    echo ""
    echo "재시작은 하지 않았습니다. 바뀐 값은 rkipc 재시작 후에 적용됩니다."
    exit 0
fi

echo ""
echo "rkipc 재시작"
if command -v RkLunch-stop.sh >/dev/null 2>&1; then
    RkLunch-stop.sh || true
    sleep 2
    RkLunch.sh &
elif [ -x /etc/init.d/S99_rkipc ]; then
    /etc/init.d/S99_rkipc restart
else
    echo "  자동 재시작 스크립트를 찾지 못했습니다. 보드를 재부팅하세요: reboot"
    exit 0
fi
sleep 3
pgrep rkipc >/dev/null 2>&1 && echo "  rkipc 재시작 완료" || echo "  경고: rkipc 가 올라오지 않았습니다. 백업으로 되돌리려면: cp $BACKUP $CONF"
