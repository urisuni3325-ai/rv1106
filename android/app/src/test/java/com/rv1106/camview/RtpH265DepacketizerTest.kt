package com.rv1106.camview

import com.rv1106.camview.rtsp.RtpDepacketizer
import com.rv1106.camview.rtsp.RtpH265Depacketizer
import org.junit.Assert.assertArrayEquals
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

class RtpH265DepacketizerTest {

    private class Collector : RtpDepacketizer.Callback {
        val units = ArrayList<Triple<ByteArray, Boolean, Long>>()
        override fun onNalUnits(au: ByteArray, isKeyFrame: Boolean, rtpTimestamp: Long) {
            units.add(Triple(au, isKeyFrame, rtpTimestamp))
        }
    }

    private val startCode = byteArrayOf(0, 0, 0, 1)

    /** H.265 NAL 헤더 2바이트. type 은 byte0 의 비트 1~6 에 들어간다. */
    private fun nalHeader(type: Int) = byteArrayOf((type shl 1).toByte(), 0x01)

    @Test
    fun `단일 NAL IDR 을 Annex-B 로 내보낸다`() {
        val collector = Collector()
        val d = RtpH265Depacketizer(collector)
        val idr = nalHeader(19) + byteArrayOf(1, 2, 3) // IDR_W_RADL

        d.process(rtp(100, 9000, true, idr), 12 + idr.size)

        assertEquals(1, collector.units.size)
        assertArrayEquals(startCode + idr, collector.units[0].first)
        assertTrue(collector.units[0].second)
        assertEquals(9000L, collector.units[0].third)
    }

    @Test
    fun `FU 로 쪼개진 NAL 을 다시 합친다`() {
        val collector = Collector()
        val d = RtpH265Depacketizer(collector)

        // 페이로드 헤더는 타입 49(FU), FU 헤더에 원래 타입 19 가 들어 있다.
        val hdr = nalHeader(49)
        val first = hdr + byteArrayOf(0x93.toByte(), 10, 11)  // S=1, type=19
        val middle = hdr + byteArrayOf(0x13, 12, 13)          // 중간
        val last = hdr + byteArrayOf(0x53, 14, 15)            // E=1

        d.process(rtp(200, 18000, false, first), 12 + first.size)
        d.process(rtp(201, 18000, false, middle), 12 + middle.size)
        d.process(rtp(202, 18000, true, last), 12 + last.size)

        assertEquals(1, collector.units.size)
        assertArrayEquals(
            startCode + nalHeader(19) + byteArrayOf(10, 11, 12, 13, 14, 15),
            collector.units[0].first,
        )
        assertTrue(collector.units[0].second)
    }

    @Test
    fun `AP 안의 VPS SPS PPS 와 IDR 이 한 액세스 유닛으로 묶인다`() {
        val collector = Collector()
        val d = RtpH265Depacketizer(collector)

        val vps = nalHeader(32) + byteArrayOf(0x0C, 0x01)
        val sps = nalHeader(33) + byteArrayOf(0x01, 0x60)
        val pps = nalHeader(34) + byteArrayOf(0xC1.toByte())
        val ap = nalHeader(48) + sized(vps) + sized(sps) + sized(pps)
        val idr = nalHeader(19) + byteArrayOf(9, 9)

        d.process(rtp(300, 27000, false, ap), 12 + ap.size)
        d.process(rtp(301, 27000, true, idr), 12 + idr.size)

        assertEquals(1, collector.units.size)
        assertArrayEquals(
            startCode + vps + startCode + sps + startCode + pps + startCode + idr,
            collector.units[0].first,
        )
        assertTrue(collector.units[0].second)
    }

    @Test
    fun `IRAP 구간 전체를 키프레임으로 인식한다`() {
        // 16~21 이 BLA/IDR/CRA. 그 밖은 키프레임이 아니다.
        for (type in 16..21) {
            val collector = Collector()
            val d = RtpH265Depacketizer(collector)
            val nal = nalHeader(type) + byteArrayOf(7)
            d.process(rtp(400, 9000, true, nal), 12 + nal.size)
            assertEquals("type=$type", 1, collector.units.size)
            assertTrue("type=$type", collector.units[0].second)
        }
    }

    @Test
    fun `첫 키프레임 전의 P 프레임은 버린다`() {
        val collector = Collector()
        val d = RtpH265Depacketizer(collector)
        val trail = nalHeader(1) + byteArrayOf(7, 7) // TRAIL_R

        d.process(rtp(500, 9000, true, trail), 12 + trail.size)

        assertEquals(0, collector.units.size)
    }

    @Test
    fun `키프레임 뒤의 일반 프레임은 통과시킨다`() {
        val collector = Collector()
        val d = RtpH265Depacketizer(collector)
        val idr = nalHeader(19) + byteArrayOf(1)
        val trail = nalHeader(1) + byteArrayOf(2)

        d.process(rtp(600, 9000, true, idr), 12 + idr.size)
        d.process(rtp(601, 12000, true, trail), 12 + trail.size)

        assertEquals(2, collector.units.size)
        assertTrue(collector.units[0].second)
        assertFalse(collector.units[1].second)
    }

    @Test
    fun `패킷이 유실되면 다음 키프레임까지 건너뛴다`() {
        val collector = Collector()
        val d = RtpH265Depacketizer(collector)
        val idr = nalHeader(19) + byteArrayOf(1)
        val trail = nalHeader(1) + byteArrayOf(2)
        val idr2 = nalHeader(19) + byteArrayOf(3)

        d.process(rtp(700, 9000, true, idr), 12 + idr.size)
        d.process(rtp(702, 12000, true, trail), 12 + trail.size)   // 701 유실
        d.process(rtp(703, 15000, true, idr2), 12 + idr2.size)

        assertEquals(2, collector.units.size)
        assertArrayEquals(startCode + idr, collector.units[0].first)
        assertArrayEquals(startCode + idr2, collector.units[1].first)
    }

    /** AP 안에 넣을 [2바이트 길이 + NAL] 형태로 감싼다. */
    private fun sized(nal: ByteArray): ByteArray =
        byteArrayOf((nal.size ushr 8).toByte(), (nal.size and 0xFF).toByte()) + nal

    private fun rtp(seq: Int, timestamp: Long, marker: Boolean, payload: ByteArray): ByteArray {
        val packet = ByteArray(12 + payload.size)
        packet[0] = 0x80.toByte()
        packet[1] = (if (marker) 0xE0 else 0x60).toByte()
        packet[2] = ((seq ushr 8) and 0xFF).toByte()
        packet[3] = (seq and 0xFF).toByte()
        packet[4] = ((timestamp ushr 24) and 0xFF).toByte()
        packet[5] = ((timestamp ushr 16) and 0xFF).toByte()
        packet[6] = ((timestamp ushr 8) and 0xFF).toByte()
        packet[7] = (timestamp and 0xFF).toByte()
        System.arraycopy(payload, 0, packet, 12, payload.size)
        return packet
    }
}
