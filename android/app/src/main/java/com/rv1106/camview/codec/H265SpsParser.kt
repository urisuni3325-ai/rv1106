package com.rv1106.camview.codec

/**
 * H.265(HEVC) SPS 에서 해상도를 뽑아낸다. MediaMuxer 트랙을 만들 때 필요하다.
 *
 * H.264 와 달리 NAL 헤더가 2바이트이고, 해상도 앞에 profile_tier_level 이
 * 놓여 있어 그 길이를 정확히 건너뛰어야 한다.
 */
object H265SpsParser {

    private const val NAL_SPS = 33

    /** @return (width, height), 파싱 실패 시 null */
    fun parseSize(spsNal: ByteArray): Pair<Int, Int>? {
        if (spsNal.isEmpty()) return null
        val offset = skipStartCode(spsNal)
        if (offset + 2 > spsNal.size) return null
        val nalType = (spsNal[offset].toInt() ushr 1) and 0x3F
        if (nalType != NAL_SPS) return null

        // NAL 헤더 2바이트를 건너뛴다.
        val rbsp = unescapeRbsp(spsNal, offset + 2)
        return try {
            parseRbsp(BitReader(rbsp))
        } catch (e: IndexOutOfBoundsException) {
            null
        }
    }

    private fun parseRbsp(r: BitReader): Pair<Int, Int>? {
        r.u(4) // sps_video_parameter_set_id
        val maxSubLayersMinus1 = r.u(3)
        r.u(1) // sps_temporal_id_nesting_flag
        skipProfileTierLevel(r, maxSubLayersMinus1)

        r.ue() // sps_seq_parameter_set_id
        val chromaFormatIdc = r.ue()
        if (chromaFormatIdc == 3) r.u(1) // separate_colour_plane_flag

        val width = r.ue()
        val height = r.ue()

        var cropLeft = 0
        var cropRight = 0
        var cropTop = 0
        var cropBottom = 0
        if (r.u(1) == 1) { // conformance_window_flag
            cropLeft = r.ue()
            cropRight = r.ue()
            cropTop = r.ue()
            cropBottom = r.ue()
        }

        // 4:2:0 은 가로·세로 모두 2, 4:2:2 는 가로만 2, 4:4:4 와 흑백은 1.
        val subWidthC = if (chromaFormatIdc == 1 || chromaFormatIdc == 2) 2 else 1
        val subHeightC = if (chromaFormatIdc == 1) 2 else 1

        val w = width - subWidthC * (cropLeft + cropRight)
        val h = height - subHeightC * (cropTop + cropBottom)
        if (w <= 0 || h <= 0) return null
        return w to h
    }

    /**
     * profile_tier_level(1, maxSubLayersMinus1) 을 건너뛴다.
     *
     * 기본 계층은 프로필 88비트 + level_idc 8비트로 고정 96비트다. 상위 계층은
     * 존재 플래그를 읽어 있는 것만 건너뛴다.
     */
    private fun skipProfileTierLevel(r: BitReader, maxSubLayersMinus1: Int) {
        r.skip(88) // general_profile_space ~ general_reserved
        r.u(8) // general_level_idc

        val profilePresent = BooleanArray(maxSubLayersMinus1)
        val levelPresent = BooleanArray(maxSubLayersMinus1)
        for (i in 0 until maxSubLayersMinus1) {
            profilePresent[i] = r.u(1) == 1
            levelPresent[i] = r.u(1) == 1
        }
        if (maxSubLayersMinus1 > 0) {
            // 8개 자리를 맞추기 위한 예약 비트
            r.skip(2 * (8 - maxSubLayersMinus1))
        }
        for (i in 0 until maxSubLayersMinus1) {
            if (profilePresent[i]) r.skip(88)
            if (levelPresent[i]) r.u(8)
        }
    }
}
