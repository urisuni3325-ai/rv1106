#!/usr/bin/env python3
"""USB(UVC) 카메라 영상을 H.264 RTSP 로 내보내는 서버.

    USB 카메라 ─MJPEG→ JPEG 디코딩 → H.264 인코딩 → rtsp://<보드IP>:554/live/0

안드로이드 앱(android/)의 기본 주소와 같은 포트·경로를 쓰므로, 앱 설정에
보드 IP 만 넣으면 바로 보인다.

Rockchip 하드웨어 플러그인(mppjpegdec / mpph264enc)이 있으면 그걸 쓰고,
없으면 소프트웨어(jpegdec / x264enc)로 대신한다. 소프트웨어일 때는 CPU 부담을
줄이려고 기본 해상도를 720p 로 낮춘다.

    sudo python3 usbcam_rtsp.py              # 카메라 자동 검색
    sudo python3 usbcam_rtsp.py --check      # 무엇을 쓸지 출력만 하고 종료
    python3 usbcam_rtsp.py --source test --port 8554   # 카메라 없이 시험
"""

import argparse
import glob
import os
import subprocess
import sys

import gi

gi.require_version("Gst", "1.0")
gi.require_version("GstRtspServer", "1.0")
from gi.repository import GLib, Gst, GstRtspServer  # noqa: E402

MOUNT = "/live/0"


def log(msg):
    print(f"[usbcam] {msg}", flush=True)


# ---------------------------------------------------------------- 카메라 찾기

def _jpeg_sizes(caps):
    """caps 에서 image/jpeg 해상도 목록을 뽑는다."""
    sizes = set()
    if caps is None:
        return sizes
    for i in range(caps.get_size()):
        s = caps.get_structure(i)
        if s.get_name() != "image/jpeg":
            continue
        ok_w, w = s.get_int("width")
        ok_h, h = s.get_int("height")
        if ok_w and ok_h:
            sizes.add((w, h))
    return sizes


def find_camera():
    """MJPEG 를 내보내는 첫 번째 V4L2 장치의 경로와 해상도 목록."""
    mon = Gst.DeviceMonitor.new()
    mon.add_filter("Video/Source", Gst.Caps.from_string("image/jpeg"))
    if mon.start():
        try:
            for dev in mon.get_devices():
                props = dev.get_properties()
                path = None
                if props is not None:
                    for key in ("api.v4l2.path", "device.path"):
                        if props.has_field(key):
                            path = props.get_string(key)
                            break
                if path:
                    return path, _jpeg_sizes(dev.get_caps()), dev.get_display_name()
        finally:
            mon.stop()

    # 장치 모니터가 못 찾으면 uvcvideo 노드를 직접 본다.
    for node in sorted(glob.glob("/sys/class/video4linux/video*")):
        driver = os.path.realpath(os.path.join(node, "device", "driver"))
        if driver.endswith("uvcvideo"):
            index_file = os.path.join(node, "index")
            try:
                with open(index_file) as f:
                    if f.read().strip() != "0":
                        continue  # 메타데이터 노드는 건너뜀
            except OSError:
                pass
            return "/dev/" + os.path.basename(node), set(), "uvcvideo"
    return None, set(), None


# ---------------------------------------------------------------- 요소 고르기

def has_element(name):
    return Gst.ElementFactory.find(name) is not None


def _enum_has(prop, nick):
    try:
        return any(v.value_nick == nick for v in prop.enum_class.__enum_values__.values())
    except AttributeError:
        return False


def element_with_props(name, wanted):
    """요소에 실제로 있는 속성만 골라 'name a=1 b=2' 문자열로 만든다.

    플러그인 버전마다 속성 이름이 달라서, 없는 속성을 넣으면 파이프라인이
    아예 안 만들어진다. 그래서 하나씩 확인한다.
    """
    el = Gst.ElementFactory.make(name, None)
    parts = [name]
    skipped = []
    for key, value in wanted.items():
        prop = el.find_property(key) if el is not None else None
        if prop is None:
            skipped.append(key)
            continue
        if isinstance(value, str) and hasattr(prop, "enum_class") and not _enum_has(prop, value):
            skipped.append(f"{key}={value}")
            continue
        parts.append(f"{key}={value}")
    if skipped:
        log(f"{name}: 지원하지 않아 건너뜀 → {', '.join(skipped)}")
    return " ".join(parts)


def build_pipeline(args, device):
    hw_dec = has_element("mppjpegdec")
    hw_enc = has_element("mpph264enc")
    hardware = hw_dec and hw_enc

    width, height = args.width, args.height
    if width is None or height is None:
        width, height = (1920, 1080) if hardware else (1280, 720)
    bitrate = args.bitrate or (4000 if hardware else 2500)  # kbps
    gop = args.gop or args.fps

    if args.source == "test":
        src = (
            f"videotestsrc is-live=true pattern=ball "
            f"! video/x-raw,width={width},height={height},framerate={args.fps}/1 "
            f"! jpegenc quality=85"
        )
    else:
        src = (
            f"v4l2src device={device} do-timestamp=true "
            f"! image/jpeg,width={width},height={height},framerate={args.fps}/1"
        )

    # 화면 지연을 늘리지 않도록 처리 대기열은 짧게, 밀리면 오래된 프레임을 버린다.
    queue = "queue max-size-buffers=2 max-size-bytes=0 max-size-time=0 leaky=downstream"

    if hw_dec:
        dec = "mppjpegdec"
    else:
        dec = "jpegdec ! videoconvert ! video/x-raw,format=I420"

    if hw_enc:
        enc = element_with_props("mpph264enc", {
            "bps": bitrate * 1000,
            "bps-max": bitrate * 1500,
            "gop": gop,
            "rc-mode": "cbr",
            "header-mode": "each-idr",
        })
        if not hw_dec:
            enc = "videoconvert ! video/x-raw,format=NV12 ! " + enc
    else:
        enc = element_with_props("x264enc", {
            "tune": "zerolatency",
            "speed-preset": "ultrafast",
            "bitrate": bitrate,
            "key-int-max": gop,
            "threads": 4,
        }) + " ! video/x-h264,profile=constrained-baseline"

    pipeline = (
        f"( {src} ! {queue} ! {dec} ! {enc} "
        f"! h264parse config-interval=-1 "
        f"! rtph264pay name=pay0 pt=96 config-interval=-1 )"
    )
    info = {
        "decoder": "mppjpegdec (하드웨어)" if hw_dec else "jpegdec (소프트웨어)",
        "encoder": "mpph264enc (하드웨어)" if hw_enc else "x264enc (소프트웨어)",
        "size": f"{width}x{height}@{args.fps}",
        "bitrate": f"{bitrate} kbps, GOP {gop}",
    }
    return pipeline, info


# ---------------------------------------------------------------- 서버

def local_ipv4s():
    try:
        out = subprocess.run(["ip", "-o", "-4", "addr", "show"],
                             capture_output=True, text=True, timeout=3).stdout
    except (OSError, subprocess.SubprocessError):
        return []
    ips = []
    for line in out.splitlines():
        fields = line.split()
        if "inet" in fields:
            ip = fields[fields.index("inet") + 1].split("/")[0]
            if not ip.startswith("127."):
                ips.append(ip)
    return ips


def main():
    ap = argparse.ArgumentParser(description="USB 카메라 → H.264 RTSP 서버")
    ap.add_argument("--device", help="카메라 장치 (기본: 자동 검색, 예 /dev/video0)")
    ap.add_argument("--source", choices=["camera", "test"], default="camera",
                    help="test = 카메라 없이 시험 영상")
    ap.add_argument("--width", type=int)
    ap.add_argument("--height", type=int)
    ap.add_argument("--fps", type=int, default=30)
    ap.add_argument("--bitrate", type=int, help="kbps (기본: 하드웨어 4000, 소프트웨어 2500)")
    ap.add_argument("--gop", type=int, help="키프레임 간격 (기본: fps 와 같게 = 1초)")
    ap.add_argument("--port", type=int, default=554)
    ap.add_argument("--check", action="store_true", help="설정만 출력하고 종료")
    args = ap.parse_args()

    if (args.width is None) != (args.height is None):
        ap.error("--width 와 --height 는 함께 지정하세요")

    Gst.init(None)

    device = args.device
    if args.source == "camera" and device is None:
        device, sizes, name = find_camera()
        if device is None:
            log("MJPEG USB 카메라를 찾지 못했습니다. 연결과 'ls /dev/video*' 를 확인하세요.")
            return 1
        log(f"카메라: {name} ({device})")
        if sizes:
            listed = ", ".join(f"{w}x{h}" for w, h in sorted(sizes, reverse=True))
            log(f"MJPEG 해상도: {listed}")

    pipeline, info = build_pipeline(args, device)
    for key, value in info.items():
        log(f"{key}: {value}")
    if "소프트웨어" in info["encoder"]:
        log("Rockchip 하드웨어 플러그인(mpph264enc)이 없어 소프트웨어로 인코딩합니다. "
            "README 의 '하드웨어 가속' 항목을 보세요.")
    log(f"파이프라인: {pipeline}")
    if args.check:
        return 0

    server = GstRtspServer.RTSPServer()
    server.set_service(str(args.port))
    factory = GstRtspServer.RTSPMediaFactory()
    factory.set_launch(pipeline)
    factory.set_shared(True)  # 여러 기기가 봐도 카메라는 한 번만 연다
    server.get_mount_points().add_factory(MOUNT, factory)

    def on_client(_server, client):
        conn = client.get_connection()
        ip = conn.get_ip() if conn is not None else "?"
        log(f"접속: {ip}")
        client.connect("closed", lambda _c: log(f"종료: {ip}"))

    server.connect("client-connected", on_client)

    if server.attach(None) == 0:
        hint = " (554 포트는 root 권한이 필요합니다. sudo 로 실행하거나 --port 8554)" \
            if args.port < 1024 and os.geteuid() != 0 else \
            " (이미 다른 프로그램이 쓰고 있을 수 있습니다: 'ss -ltnp | grep :%d')" % args.port
        log(f"{args.port} 포트를 열지 못했습니다{hint}")
        return 1

    ips = local_ipv4s() or ["<보드IP>"]
    for ip in ips:
        log(f"앱에 넣을 주소: rtsp://{ip}:{args.port}{MOUNT}")

    loop = GLib.MainLoop()
    try:
        loop.run()
    except KeyboardInterrupt:
        pass
    return 0


if __name__ == "__main__":
    sys.exit(main())
