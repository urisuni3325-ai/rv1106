package com.rv1106.camview

import com.rv1106.camview.codec.H265SpsParser
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Test

class H265SpsParserTest {

    @Test
    fun `크롭이 없는 해상도를 읽는다`() {
        assertEquals(1280 to 720, H265SpsParser.parseSize(SpsWriter.build(1280, 720)))
    }

    @Test
    fun `1080p 를 읽는다`() {
        // H.265 는 최소 블록이 8이라 1080 을 그대로 적을 수 있다.
        assertEquals(1920 to 1080, H265SpsParser.parseSize(SpsWriter.build(1920, 1080)))
    }

    @Test
    fun `conformance window 로 잘라낸 해상도를 읽는다`() {
        // 인코더가 1088 로 잡고 아래 8픽셀을 잘라내는 경우.
        val sps = SpsWriter.build(1920, 1088, cropBottom = 4)
        assertEquals(1920 to 1080, H265SpsParser.parseSize(sps))
    }

    @Test
    fun `SC3336 기본 해상도 2304x1296 을 읽는다`() {
        assertEquals(2304 to 1296, H265SpsParser.parseSize(SpsWriter.build(2304, 1296)))
    }

    @Test
    fun `상위 계층이 있는 SPS 도 읽는다`() {
        val sps = SpsWriter.build(1920, 1080, maxSubLayersMinus1 = 2)
        assertEquals(1920 to 1080, H265SpsParser.parseSize(sps))
    }

    @Test
    fun `4바이트 start code 가 붙어 있어도 읽는다`() {
        val sps = byteArrayOf(0, 0, 0, 1) + SpsWriter.build(640, 480)
        assertEquals(640 to 480, H265SpsParser.parseSize(sps))
    }

    @Test
    fun `SPS 가 아닌 NAL 은 null 을 돌려준다`() {
        val vps = byteArrayOf(0x40, 0x01, 0x0C, 0x01) // nal type 32
        assertNull(H265SpsParser.parseSize(vps))
    }

    @Test
    fun `빈 배열은 null 을 돌려준다`() {
        assertNull(H265SpsParser.parseSize(ByteArray(0)))
    }

    /** 테스트용 최소 HEVC SPS 생성기(H.265 7.3.2.2 순서대로 기록). */
    private object SpsWriter {

        fun build(
            width: Int,
            height: Int,
            cropBottom: Int = 0,
            maxSubLayersMinus1: Int = 0,
        ): ByteArray {
            val w = BitWriter()
            w.u(4, 0) // sps_video_parameter_set_id
            w.u(3, maxSubLayersMinus1)
            w.u(1, 1) // sps_temporal_id_nesting_flag

            // profile_tier_level(1, maxSubLayersMinus1)
            w.zeros(88) // general_profile_space ~ 예약 비트
            w.u(8, 120) // general_level_idc
            val profilePresent = BooleanArray(maxSubLayersMinus1) { true }
            val levelPresent = BooleanArray(maxSubLayersMinus1) { true }
            for (i in 0 until maxSubLayersMinus1) {
                w.u(1, if (profilePresent[i]) 1 else 0)
                w.u(1, if (levelPresent[i]) 1 else 0)
            }
            if (maxSubLayersMinus1 > 0) w.zeros(2 * (8 - maxSubLayersMinus1))
            for (i in 0 until maxSubLayersMinus1) {
                if (profilePresent[i]) w.zeros(88)
                if (levelPresent[i]) w.u(8, 120)
            }

            w.ue(0) // sps_seq_parameter_set_id
            w.ue(1) // chroma_format_idc = 4:2:0
            w.ue(width)
            w.ue(height)
            if (cropBottom != 0) {
                w.u(1, 1) // conformance_window_flag
                w.ue(0) // left
                w.ue(0) // right
                w.ue(0) // top
                w.ue(cropBottom)
            } else {
                w.u(1, 0)
            }
            w.u(1, 1) // rbsp_stop_one_bit
            // NAL 헤더: type 33(SPS), layer 0, tid 1
            return byteArrayOf(0x42, 0x01) + w.toByteArray()
        }

        private class BitWriter {
            private val bytes = ArrayList<Byte>()
            private var current = 0
            private var bitsUsed = 0

            fun u(bits: Int, value: Int) {
                for (i in bits - 1 downTo 0) {
                    val bit = (value ushr i) and 1
                    current = (current shl 1) or bit
                    bitsUsed++
                    if (bitsUsed == 8) {
                        bytes.add(current.toByte())
                        current = 0
                        bitsUsed = 0
                    }
                }
            }

            fun zeros(bits: Int) {
                repeat(bits) { u(1, 0) }
            }

            fun ue(value: Int) {
                val v = value + 1
                var bits = 0
                while ((v ushr bits) != 0) bits++
                u(bits - 1, 0)
                u(bits, v)
            }

            fun toByteArray(): ByteArray {
                if (bitsUsed > 0) {
                    current = current shl (8 - bitsUsed)
                    bytes.add(current.toByte())
                    current = 0
                    bitsUsed = 0
                }
                return bytes.toByteArray()
            }
        }
    }
}
