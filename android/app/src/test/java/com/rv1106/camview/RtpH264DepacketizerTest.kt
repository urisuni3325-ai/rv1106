package com.rv1106.camview

import com.rv1106.camview.rtsp.RtpDepacketizer
import com.rv1106.camview.rtsp.RtpH264Depacketizer
import org.junit.Assert.assertArrayEquals
import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test

class RtpH264DepacketizerTest {

    private class Collector : RtpDepacketizer.Callback {
        val units = ArrayList<Triple<ByteArray, Boolean, Long>>()
        override fun onNalUnits(au: ByteArray, isKeyFrame: Boolean, rtpTimestamp: Long) {
            units.add(Triple(au, isKeyFrame, rtpTimestamp))
        }
    }

    private val startCode = byteArrayOf(0, 0, 0, 1)

    @Test
    fun `단일 NAL 패킷을 Annex-B 로 내보낸다`() {
        val collector = Collector()
        val d = RtpH264Depacketizer(collector)
        val idr = byteArrayOf(0x65, 1, 2, 3, 4)

        d.process(rtp(seq = 100, timestamp = 9000, marker = true, payload = idr), 12 + idr.size)

        assertEquals(1, collector.units.size)
        assertArrayEquals(startCode + idr, collector.units[0].first)
        assertTrue(collector.units[0].second)
        assertEquals(9000L, collector.units[0].third)
    }

    @Test
    fun `FU-A 로 쪼개진 NAL 을 다시 합친다`() {
        val collector = Collector()
        val d = RtpH264Depacketizer(collector)

        // 원본 NAL: 헤더 0x65(IDR, nal_ref_idc=3) + 페이로드 6바이트
        val first = byteArrayOf(0x7C, 0x85.toByte(), 10, 11)   // S=1, type=5
        val middle = byteArrayOf(0x7C, 0x05, 12, 13)           // 중간
        val last = byteArrayOf(0x7C, 0x45, 14, 15)             // E=1

        d.process(rtp(200, 18000, false, first), 12 + first.size)
        d.process(rtp(201, 18000, false, middle), 12 + middle.size)
        d.process(rtp(202, 18000, true, last), 12 + last.size)

        assertEquals(1, collector.units.size)
        assertArrayEquals(
            startCode + byteArrayOf(0x65, 10, 11, 12, 13, 14, 15),
            collector.units[0].first,
        )
        assertTrue(collector.units[0].second)
    }

    @Test
    fun `STAP-A 안의 SPS PPS 와 IDR 이 한 액세스 유닛으로 묶인다`() {
        val collector = Collector()
        val d = RtpH264Depacketizer(collector)

        val sps = byteArrayOf(0x67, 0x42, 0x00, 0x28)
        val pps = byteArrayOf(0x68, 0xCE.toByte(), 0x38, 0x80.toByte())
        val stap = ByteArray(1 + 2 + sps.size + 2 + pps.size)
        stap[0] = 0x78 // STAP-A
        stap[1] = 0
        stap[2] = sps.size.toByte()
        System.arraycopy(sps, 0, stap, 3, sps.size)
        stap[3 + sps.size] = 0
        stap[4 + sps.size] = pps.size.toByte()
        System.arraycopy(pps, 0, stap, 5 + sps.size, pps.size)

        val idr = byteArrayOf(0x65, 9, 9, 9)

        d.process(rtp(300, 27000, false, stap), 12 + stap.size)
        d.process(rtp(301, 27000, true, idr), 12 + idr.size)

        assertEquals(1, collector.units.size)
        assertArrayEquals(
            startCode + sps + startCode + pps + startCode + idr,
            collector.units[0].first,
        )
        assertTrue(collector.units[0].second)
    }

    @Test
    fun `타임스탬프가 바뀌면 이전 액세스 유닛을 내보낸다`() {
        val collector = Collector()
        val d = RtpH264Depacketizer(collector)
        val idr = byteArrayOf(0x65, 1)
        val p = byteArrayOf(0x41, 2)

        // marker 없이 타임스탬프만 바뀌는 서버도 있다.
        d.process(rtp(400, 9000, false, idr), 12 + idr.size)
        d.process(rtp(401, 12000, false, p), 12 + p.size)

        assertEquals(1, collector.units.size)
        assertArrayEquals(startCode + idr, collector.units[0].first)
    }

    @Test
    fun `첫 키프레임 전의 P 프레임은 버린다`() {
        val collector = Collector()
        val d = RtpH264Depacketizer(collector)
        val p = byteArrayOf(0x41, 7, 7)

        d.process(rtp(500, 9000, true, p), 12 + p.size)

        assertEquals(0, collector.units.size)
    }

    @Test
    fun `패킷이 유실되면 다음 키프레임까지 건너뛴다`() {
        val collector = Collector()
        val d = RtpH264Depacketizer(collector)
        val idr = byteArrayOf(0x65, 1)
        val p1 = byteArrayOf(0x41, 2)
        val p2 = byteArrayOf(0x41, 3)
        val idr2 = byteArrayOf(0x65, 4)

        d.process(rtp(600, 9000, true, idr), 12 + idr.size)      // 정상 통과
        d.process(rtp(602, 12000, true, p1), 12 + p1.size)       // 601 유실 → 버림
        d.process(rtp(603, 15000, true, p2), 12 + p2.size)       // 키프레임 아님 → 버림
        d.process(rtp(604, 18000, true, idr2), 12 + idr2.size)   // 키프레임 → 재개

        assertEquals(2, collector.units.size)
        assertArrayEquals(startCode + idr, collector.units[0].first)
        assertArrayEquals(startCode + idr2, collector.units[1].first)
    }

    @Test
    fun `RTP 확장 헤더와 CSRC 를 건너뛴다`() {
        val collector = Collector()
        val d = RtpH264Depacketizer(collector)
        val idr = byteArrayOf(0x65, 42)

        // CSRC 1개 + 확장 헤더 1워드
        val payloadStart = 12 + 4 + 8
        val packet = ByteArray(payloadStart + idr.size)
        packet[0] = 0x91.toByte() // V=2, X=1, CC=1
        packet[1] = 0xE0.toByte() // marker=1, PT=96
        packet[2] = 0x02; packet[3] = 0x58 // seq = 600
        packet[7] = 0x64 // timestamp = 100
        packet[16] = 0xBE.toByte(); packet[17] = 0xDE.toByte() // 확장 프로필
        packet[18] = 0x00; packet[19] = 0x01 // 확장 길이 1워드
        System.arraycopy(idr, 0, packet, payloadStart, idr.size)

        d.process(packet, packet.size)

        assertEquals(1, collector.units.size)
        assertArrayEquals(startCode + idr, collector.units[0].first)
        assertEquals(100L, collector.units[0].third)
    }

    @Test
    fun `reset 후에는 다시 키프레임부터 시작한다`() {
        val collector = Collector()
        val d = RtpH264Depacketizer(collector)
        val idr = byteArrayOf(0x65, 1)
        val p = byteArrayOf(0x41, 2)

        d.process(rtp(700, 9000, true, idr), 12 + idr.size)
        d.reset()
        d.process(rtp(701, 12000, true, p), 12 + p.size)

        assertEquals(1, collector.units.size)
    }

    /** 최소 RTP 헤더(12바이트) + 페이로드 패킷을 만든다. */
    private fun rtp(seq: Int, timestamp: Long, marker: Boolean, payload: ByteArray): ByteArray {
        val packet = ByteArray(12 + payload.size)
        packet[0] = 0x80.toByte() // V=2, P=0, X=0, CC=0
        packet[1] = (if (marker) 0xE0 else 0x60).toByte() // marker + PT=96
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
