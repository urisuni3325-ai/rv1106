package com.rv1106.camview.capture

/**
 * JPEG 파일에 해상도(dpi)를 적어 넣는다.
 *
 * 안드로이드의 [android.graphics.Bitmap.compress] 는 JFIF 헤더의 밀도 단위를
 * "단위 없음(가로세로 비율만)"으로 써 버린다. 그래서 저장한 파일을 열면
 * 뷰어에 따라 96dpi 나 1dpi 로 보인다. 분석 쪽에 "72dpi" 로 넘기려면
 * APP0(JFIF) 세그먼트의 세 칸을 고쳐 주어야 한다.
 *
 *   APP0 = FF E0 | 길이(2) | "JFIF\0"(5) | 버전(2) | 단위(1) | Xdensity(2) | Ydensity(2) | ...
 *   단위 0 = 없음, 1 = dpi(인치당), 2 = dpcm(센티미터당)
 *
 * 픽셀 데이터는 건드리지 않는다. dpi 는 인쇄·표시 크기를 위한 메타데이터일 뿐이라
 * 이미지 내용과 화질은 그대로다.
 */
object JpegDensity {

    private const val MARKER = 0xFF.toByte()
    private const val SOI = 0xD8.toByte()
    private const val APP0 = 0xE0.toByte()

    /** 새로 끼워 넣을 JFIF APP0 의 전체 크기 — 마커 2바이트 + 세그먼트 16바이트. */
    private const val APP0_LENGTH = 18

    /**
     * [jpeg] 의 해상도를 [dpi] 로 맞춘 바이트열을 돌려준다.
     * 이미 JFIF APP0 이 있으면 그 자리를 고치고, 없으면 SOI 뒤에 새로 끼운다.
     * JPEG 이 아니면 원본을 그대로 돌려준다.
     */
    fun apply(jpeg: ByteArray, dpi: Int): ByteArray {
        if (dpi !in 1..65535) return jpeg
        if (jpeg.size < 4 || jpeg[0] != MARKER || jpeg[1] != SOI) return jpeg

        val patched = jpeg.copyOf()
        if (patched[2] == MARKER && patched[3] == APP0 && hasJfifTag(patched, 4)) {
            writeDensity(patched, 4, dpi)
            return patched
        }
        return insertApp0(jpeg, dpi)
    }

    /** APP0 데이터가 "JFIF\0" 로 시작하는지. [at] 은 길이 필드의 위치. */
    private fun hasJfifTag(bytes: ByteArray, at: Int): Boolean {
        if (at + 7 > bytes.size) return false
        return bytes[at + 2] == 'J'.code.toByte() &&
            bytes[at + 3] == 'F'.code.toByte() &&
            bytes[at + 4] == 'I'.code.toByte() &&
            bytes[at + 5] == 'F'.code.toByte() &&
            bytes[at + 6] == 0.toByte()
    }

    /** [at] 은 APP0 길이 필드의 위치. 단위·Xdensity·Ydensity 를 채운다. */
    private fun writeDensity(bytes: ByteArray, at: Int, dpi: Int) {
        val units = at + 9          // 길이(2) + "JFIF\0"(5) + 버전(2)
        if (units + 4 >= bytes.size) return
        bytes[units] = 1            // 1 = 인치당 픽셀
        bytes[units + 1] = (dpi ushr 8).toByte()
        bytes[units + 2] = (dpi and 0xFF).toByte()
        bytes[units + 3] = (dpi ushr 8).toByte()
        bytes[units + 4] = (dpi and 0xFF).toByte()
    }

    /** JFIF APP0 이 없는 파일(EXIF 로만 시작하는 경우 등)에 SOI 바로 뒤로 끼워 넣는다. */
    private fun insertApp0(jpeg: ByteArray, dpi: Int): ByteArray {
        val out = ByteArray(jpeg.size + APP0_LENGTH)
        out[0] = MARKER
        out[1] = SOI
        out[2] = MARKER
        out[3] = APP0
        out[4] = 0                                  // 길이 = 16 (마커 2바이트 제외)
        out[5] = 16
        out[6] = 'J'.code.toByte()
        out[7] = 'F'.code.toByte()
        out[8] = 'I'.code.toByte()
        out[9] = 'F'.code.toByte()
        out[10] = 0
        out[11] = 1                                 // 버전 1.01
        out[12] = 1
        writeDensity(out, 4, dpi)                   // out[13..17]
        out[18] = 0                                 // 썸네일 없음
        out[19] = 0
        jpeg.copyInto(out, APP0_LENGTH + 2, 2, jpeg.size)
        return out
    }
}
