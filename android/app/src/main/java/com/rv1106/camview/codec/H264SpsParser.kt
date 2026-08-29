package com.rv1106.camview.codec

/** SPS NAL 에서 해상도를 뽑아낸다. MediaMuxer 트랙을 만들 때 필요하다. */
object H264SpsParser {

    /** @return (width, height), 파싱 실패 시 null */
    fun parseSize(spsNal: ByteArray): Pair<Int, Int>? {
        if (spsNal.isEmpty()) return null
        // 앞의 start code 와 NAL 헤더(1바이트)를 건너뛴다.
        var offset = 0
        while (offset + 3 < spsNal.size &&
            spsNal[offset] == 0.toByte() && spsNal[offset + 1] == 0.toByte()
        ) {
            offset += if (spsNal[offset + 2] == 1.toByte()) 3 else 1
        }
        if (offset >= spsNal.size) return null
        if ((spsNal[offset].toInt() and 0x1F) != 7) return null
        offset += 1
        if (offset >= spsNal.size) return null

        val rbsp = unescape(spsNal, offset)
        return try {
            parseRbsp(BitReader(rbsp))
        } catch (e: IndexOutOfBoundsException) {
            null
        }
    }

    private fun parseRbsp(r: BitReader): Pair<Int, Int>? {
        val profileIdc = r.u(8)
        r.u(8) // constraint flags + reserved
        r.u(8) // level_idc
        r.ue() // seq_parameter_set_id

        var chromaFormatIdc = 1
        if (profileIdc in HIGH_PROFILES) {
            chromaFormatIdc = r.ue()
            if (chromaFormatIdc == 3) r.u(1) // separate_colour_plane_flag
            r.ue() // bit_depth_luma_minus8
            r.ue() // bit_depth_chroma_minus8
            r.u(1) // qpprime_y_zero_transform_bypass_flag
            if (r.u(1) == 1) { // seq_scaling_matrix_present_flag
                val count = if (chromaFormatIdc != 3) 8 else 12
                for (i in 0 until count) {
                    if (r.u(1) == 1) skipScalingList(r, if (i < 6) 16 else 64)
                }
            }
        }

        r.ue() // log2_max_frame_num_minus4
        when (r.ue()) { // pic_order_cnt_type
            0 -> r.ue() // log2_max_pic_order_cnt_lsb_minus4
            1 -> {
                r.u(1) // delta_pic_order_always_zero_flag
                r.se() // offset_for_non_ref_pic
                r.se() // offset_for_top_to_bottom_field
                val cycle = r.ue()
                for (i in 0 until cycle) r.se()
            }
        }
        r.ue() // max_num_ref_frames
        r.u(1) // gaps_in_frame_num_value_allowed_flag

        val picWidthInMbs = r.ue() + 1
        val picHeightInMapUnits = r.ue() + 1
        val frameMbsOnly = r.u(1)
        if (frameMbsOnly == 0) r.u(1) // mb_adaptive_frame_field_flag
        r.u(1) // direct_8x8_inference_flag

        var cropLeft = 0
        var cropRight = 0
        var cropTop = 0
        var cropBottom = 0
        if (r.u(1) == 1) { // frame_cropping_flag
            cropLeft = r.ue()
            cropRight = r.ue()
            cropTop = r.ue()
            cropBottom = r.ue()
        }

        val width = picWidthInMbs * 16
        val height = (2 - frameMbsOnly) * picHeightInMapUnits * 16
        val subWidthC = if (chromaFormatIdc == 3) 1 else 2
        val subHeightC = if (chromaFormatIdc == 1) 2 else 1
        val cropUnitX = if (chromaFormatIdc == 0) 1 else subWidthC
        val cropUnitY = (if (chromaFormatIdc == 0) 1 else subHeightC) * (2 - frameMbsOnly)

        val w = width - (cropLeft + cropRight) * cropUnitX
        val h = height - (cropTop + cropBottom) * cropUnitY
        if (w <= 0 || h <= 0) return null
        return w to h
    }

    private fun skipScalingList(r: BitReader, size: Int) {
        var lastScale = 8
        var nextScale = 8
        for (i in 0 until size) {
            if (nextScale != 0) {
                val delta = r.se()
                nextScale = (lastScale + delta + 256) % 256
            }
            lastScale = if (nextScale == 0) lastScale else nextScale
        }
    }

    /** emulation prevention byte(0x03) 제거 */
    private fun unescape(data: ByteArray, from: Int): ByteArray {
        val out = ByteArray(data.size - from)
        var len = 0
        var zeros = 0
        var i = from
        while (i < data.size) {
            val b = data[i]
            if (zeros == 2 && b == 3.toByte()) {
                zeros = 0
            } else {
                out[len++] = b
                zeros = if (b == 0.toByte()) zeros + 1 else 0
            }
            i++
        }
        return out.copyOf(len)
    }

    private val HIGH_PROFILES = intArrayOf(100, 110, 122, 244, 44, 83, 86, 118, 128, 138, 139, 134, 135)

    private class BitReader(private val data: ByteArray) {
        private var bitPos = 0

        fun u(bits: Int): Int {
            var value = 0
            for (i in 0 until bits) {
                val byteIndex = bitPos ushr 3
                if (byteIndex >= data.size) throw IndexOutOfBoundsException()
                val bit = (data[byteIndex].toInt() ushr (7 - (bitPos and 7))) and 1
                value = (value shl 1) or bit
                bitPos++
            }
            return value
        }

        fun ue(): Int {
            var leadingZeros = 0
            while (u(1) == 0) {
                leadingZeros++
                if (leadingZeros > 31) throw IndexOutOfBoundsException()
            }
            if (leadingZeros == 0) return 0
            return (1 shl leadingZeros) - 1 + u(leadingZeros)
        }

        fun se(): Int {
            val k = ue()
            val sign = if (k and 1 == 1) 1 else -1
            return sign * ((k + 1) / 2)
        }
    }
}
