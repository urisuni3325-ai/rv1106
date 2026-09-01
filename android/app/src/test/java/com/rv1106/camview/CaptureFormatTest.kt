package com.rv1106.camview

import com.rv1106.camview.capture.CaptureFormat
import com.rv1106.camview.capture.JpegDensity
import org.junit.Assert.assertArrayEquals
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Test

class CaptureFormatTest {

    @Test
    fun `16대9 프레임은 가운데 정사각형으로 잘린다`() {
        val crop = CaptureFormat.centerSquare(2304, 1296)!!
        assertEquals(1296, crop.size)
        assertEquals((2304 - 1296) / 2, crop.x)
        assertEquals(0, crop.y)
    }

    @Test
    fun `세로가 긴 프레임도 짧은 변을 따라간다`() {
        val crop = CaptureFormat.centerSquare(720, 1280)!!
        assertEquals(720, crop.size)
        assertEquals(0, crop.x)
        assertEquals(280, crop.y)
    }

    @Test
    fun `이미 정사각형이면 그대로 쓴다`() {
        val crop = CaptureFormat.centerSquare(1000, 1000)!!
        assertEquals(1000, crop.size)
        assertEquals(0, crop.x)
        assertEquals(0, crop.y)
    }

    @Test
    fun `크기가 0이면 잘라낼 수 없다`() {
        assertNull(CaptureFormat.centerSquare(0, 1080))
        assertNull(CaptureFormat.centerSquare(1920, -1))
    }

    @Test
    fun `JFIF 헤더가 있으면 그 자리에 72dpi 를 적는다`() {
        val jpeg = jfifJpeg(units = 0, density = 1)
        val out = JpegDensity.apply(jpeg, 72)

        assertEquals(jpeg.size, out.size)
        assertEquals(1, out[13].toInt())            // 단위 = 인치당
        assertEquals(72, be16(out, 14))             // Xdensity
        assertEquals(72, be16(out, 16))             // Ydensity
        // 헤더 뒤의 실제 이미지 데이터는 그대로여야 한다.
        assertArrayEquals(jpeg.copyOfRange(20, jpeg.size), out.copyOfRange(20, out.size))
    }

    @Test
    fun `JFIF 헤더가 없으면 SOI 뒤에 새로 끼운다`() {
        // EXIF(APP1)로 시작하는 파일을 흉내 낸다.
        val body = byteArrayOf(0xFF.toByte(), 0xE1.toByte(), 0x00, 0x08, 1, 2, 3, 4, 5, 6,
            0xFF.toByte(), 0xD9.toByte())
        val jpeg = byteArrayOf(0xFF.toByte(), 0xD8.toByte()) + body

        val out = JpegDensity.apply(jpeg, 72)

        assertEquals(jpeg.size + 18, out.size)
        assertEquals(0xE0, out[3].toInt() and 0xFF)
        assertEquals(16, be16(out, 4))              // APP0 길이
        assertEquals(1, out[13].toInt())
        assertEquals(72, be16(out, 14))
        assertEquals(72, be16(out, 16))
        assertArrayEquals(body, out.copyOfRange(20, out.size))
    }

    @Test
    fun `JPEG 이 아니면 손대지 않는다`() {
        val notJpeg = byteArrayOf(0x89.toByte(), 0x50, 0x4E, 0x47)
        assertArrayEquals(notJpeg, JpegDensity.apply(notJpeg, 72))
    }

    @Test
    fun `말이 안 되는 dpi 는 무시한다`() {
        val jpeg = jfifJpeg(units = 0, density = 1)
        assertArrayEquals(jpeg, JpegDensity.apply(jpeg, 0))
    }

    /** 안드로이드가 만들어 내는 형태의 최소 JFIF 헤더 + 더미 데이터. */
    private fun jfifJpeg(units: Int, density: Int): ByteArray = byteArrayOf(
        0xFF.toByte(), 0xD8.toByte(),                       // SOI
        0xFF.toByte(), 0xE0.toByte(), 0x00, 0x10,           // APP0, 길이 16
        'J'.code.toByte(), 'F'.code.toByte(), 'I'.code.toByte(), 'F'.code.toByte(), 0,
        1, 1,                                               // 버전 1.01
        units.toByte(),
        (density ushr 8).toByte(), (density and 0xFF).toByte(),
        (density ushr 8).toByte(), (density and 0xFF).toByte(),
        0, 0,                                               // 썸네일 없음
        0xFF.toByte(), 0xDB.toByte(), 0x00, 0x04, 0x11, 0x22,
        0xFF.toByte(), 0xD9.toByte()
    )

    private fun be16(bytes: ByteArray, at: Int): Int =
        ((bytes[at].toInt() and 0xFF) shl 8) or (bytes[at + 1].toInt() and 0xFF)
}
