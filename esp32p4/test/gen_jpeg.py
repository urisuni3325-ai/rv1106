"""테스트용 JPEG 생성 / 검증 (Pillow 필요)."""
import sys
from PIL import Image


def gen(out_dir):
    img = Image.new("RGB", (1280, 720))
    px = img.load()
    for y in range(720):
        for x in range(1280):
            px[x, y] = (x % 256, y % 256, (x + y) % 256)
    # optimize=False → 표준 허프만 테이블로 인코딩 (MJPEG 카메라와 같은 조건)
    img.save(f"{out_dir}/src_420.jpg", quality=85, subsampling=2, optimize=False)
    img.resize((640, 480)).save(f"{out_dir}/src_422.jpg", quality=85, subsampling=1, optimize=False)


def check(out_dir):
    for name, ref in (("1280x720", "src_420.jpg"), ("640x480", "src_422.jpg")):
        fixed = Image.open(f"{out_dir}/fixed_{name}.jpg")
        fixed.load()
        a = fixed.tobytes()
        b = Image.open(f"{out_dir}/{ref}").tobytes()
        assert a == b, f"{name}: 디코드 결과가 원본과 다름"
        try:
            Image.open(f"{out_dir}/mjpeg_{name}.jpg").load()
            # libjpeg 이 표준 테이블을 알아서 쓰는 경우도 있으니 실패를 강제하진 않는다
        except OSError:
            pass
    print("gen_jpeg check: OK")


if __name__ == "__main__":
    {"gen": gen, "check": check}[sys.argv[1]](sys.argv[2])
