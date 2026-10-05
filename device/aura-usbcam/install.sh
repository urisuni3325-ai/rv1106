#!/bin/sh
# Luckfox Aura(RV1126B, Debian 13 이미지)에 USB 카메라 RTSP 서버를 설치한다.
#
#   sudo ./install.sh
#
# 설치 후 부팅할 때마다 자동으로 rtsp://<보드IP>:554/live/0 을 내보낸다.
set -eu

if [ "$(id -u)" -ne 0 ]; then
    echo "root 권한이 필요합니다: sudo ./install.sh"
    exit 1
fi
if ! command -v apt-get >/dev/null 2>&1; then
    echo "apt-get 이 없습니다. Luckfox Aura 의 Debian 13 이미지에서 실행하세요."
    echo "(Buildroot 이미지는 README 의 '이미지 확인' 항목을 보세요)"
    exit 1
fi

DIR=$(cd "$(dirname "$0")" && pwd)

echo "== 1/4 패키지 설치 (GStreamer, RTSP 서버) =="
apt-get update
apt-get install -y \
    python3-gi gir1.2-gst-rtsp-server-1.0 \
    gstreamer1.0-tools gstreamer1.0-plugins-base gstreamer1.0-plugins-good \
    gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly v4l-utils

echo "== 2/4 하드웨어 가속 플러그인 확인 =="
if ! gst-inspect-1.0 mpph264enc >/dev/null 2>&1; then
    # 저장소에 Rockchip 플러그인이 있으면 설치한다. 없으면 조용히 넘어간다.
    apt-get install -y gstreamer1.0-rockchip1 >/dev/null 2>&1 || true
fi
if gst-inspect-1.0 mpph264enc >/dev/null 2>&1 && gst-inspect-1.0 mppjpegdec >/dev/null 2>&1; then
    echo "하드웨어 가속 사용: mppjpegdec + mpph264enc (1080p30)"
else
    echo "주의: Rockchip 하드웨어 플러그인이 없어 소프트웨어로 인코딩합니다 (720p30)."
    echo "      README 의 '하드웨어 가속' 항목을 보세요."
fi

echo "== 3/4 서버 설치 =="
install -d /opt/usbcam
install -m 755 "$DIR/usbcam_rtsp.py" /opt/usbcam/usbcam_rtsp.py
install -m 644 "$DIR/usbcam-rtsp.service" /etc/systemd/system/usbcam-rtsp.service
if [ ! -f /etc/default/usbcam-rtsp ]; then
    echo 'USBCAM_ARGS=""' > /etc/default/usbcam-rtsp
fi

if pgrep -x rkipc >/dev/null 2>&1; then
    echo "주의: rkipc(MIPI 카메라용 기본 RTSP 서버)가 554 포트를 쓰고 있습니다."
    echo "      USB 카메라만 쓸 거라면 rkipc 를 끄고 다시 실행하세요."
fi

echo "== 4/4 시작 =="
systemctl daemon-reload
systemctl enable usbcam-rtsp
systemctl restart usbcam-rtsp
sleep 3
journalctl -u usbcam-rtsp -n 15 --no-pager || true

echo
echo "완료. 위 로그의 '앱에 넣을 주소' 를 패드 앱 설정에 넣으세요."
echo "로그 보기: journalctl -u usbcam-rtsp -f"
