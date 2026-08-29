package com.rv1106.camview

import com.rv1106.camview.codec.H264SpsParser
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Test

class H264SpsParserTest {

    @Test
    fun `16의 배수 해상도를 그대로 읽는다`() {
        val sps = SpsWriter.baseline(1280, 720)
        assertEquals(1280 to 720, H264SpsParser.parseSize(sps))
    }

    @Test
    fun `크롭이 필요한 1080p 를 읽는다`() {
        // 1080 은 16 의 배수가 아니라 SPS 에서 1088 로 잡고 아래쪽 8픽셀을 크롭한다.
        val sps = SpsWriter.baseline(1920, 1080)
        assertEquals(1920 to 1080, H264SpsParser.parseSize(sps))
    }

    @Test
    fun `SC3336 기본 해상도 2304x1296 을 읽는다`() {
        val sps = SpsWriter.baseline(2304, 1296)
        assertEquals(2304 to 1296, H264SpsParser.parseSize(sps))
    }

    @Test
    fun `4바이트 start code 가 붙어 있어도 읽는다`() {
        val sps = byteArrayOf(0, 0, 0, 1) + SpsWriter.baseline(640, 480)
        assertEquals(640 to 480, H264SpsParser.parseSize(sps))
    }

    @Test
    fun `high profile SPS 도 읽는다`() {
        val sps = SpsWriter.high(1920, 1080)
        assertEquals(1920 to 1080, H264SpsParser.parseSize(sps))
    }

    @Test
    fun `SPS 가 아닌 NAL 은 null 을 돌려준다`() {
        val pps = byteArrayOf(0, 0, 0, 1, 0x68, 0xCE.toByte(), 0x38, 0x80.toByte())
        assertNull(H264SpsParser.parseSize(pps))
    }

    @Test
    fun `빈 배열은 null 을 돌려준다`() {
        assertNull(H264SpsParser.parseSize(ByteArray(0)))
    }

    /** 테스트용 최소 SPS 생성기(H.264 7.3.2.1 순서대로 기록). */
    private object SpsWriter {

        fun baseline(width: Int, height: Int): ByteArray = build(width, height, highProfile = false)

        fun high(width: Int, height: Int): ByteArray = build(width, height, highProfile = true)

        private fun build(width: Int, height: Int, highProfile: Boolean): ByteArray {
            val w = BitWriter()
            w.u(8, if (highProfile) 100 else 66) // profile_idc
            w.u(8, 0) // constraint flags
            w.u(8, 40) // level_idc
            w.ue(0) // seq_parameter_set_id
            if (highProfile) {
                w.ue(1) // chroma_format_idc = 4:2:0
                w.ue(0) // bit_depth_luma_minus8
                w.ue(0) // bit_depth_chroma_minus8
                w.u(1, 0) // qpprime_y_zero_transform_bypass_flag
                w.u(1, 0) // seq_scaling_matrix_present_flag
            }
            w.ue(0) // log2_max_frame_num_minus4
            w.ue(0) // pic_order_cnt_type
            w.ue(0) // log2_max_pic_order_cnt_lsb_minus4
            w.ue(1) // max_num_ref_frames
            w.u(1, 0) // gaps_in_frame_num_value_allowed_flag

            val mbWidth = (width + 15) / 16
            val mbHeight = (height + 15) / 16
            w.ue(mbWidth - 1)
            w.ue(mbHeight - 1)
            w.u(1, 1) // frame_mbs_only_flag
            w.u(1, 1) // direct_8x8_inference_flag

            val cropRight = (mbWidth * 16 - width) / 2 // cropUnitX = 2 (4:2:0)
            val cropBottom = (mbHeight * 16 - height) / 2 // cropUnitY = 2 (frame_mbs_only)
            if (cropRight != 0 || cropBottom != 0) {
                w.u(1, 1) // frame_cropping_flag
                w.ue(0) // left
                w.ue(cropRight)
                w.ue(0) // top
                w.ue(cropBottom)
            } else {
                w.u(1, 0)
            }
            w.u(1, 0) // vui_parameters_present_flag
            w.u(1, 1) // rbsp_stop_one_bit
            return byteArrayOf(0x67) + w.toByteArray()
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
