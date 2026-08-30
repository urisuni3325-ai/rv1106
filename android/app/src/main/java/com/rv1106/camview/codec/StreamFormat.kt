package com.rv1106.camview.codec

import android.media.MediaFormat

/**
 * 디코더와 먹서에 넘길 코덱 정보. H.264 와 H.265 를 같은 방식으로 다루기 위한 그릇이다.
 *
 * csd 값은 모두 Annex-B(start code 포함) 형태로 담는다.
 */
class StreamFormat(
    val mime: String,
    /** H.264 는 SPS, H.265 는 VPS+SPS+PPS 를 이어붙인 것. */
    val csd0: ByteArray,
    /** H.264 의 PPS. H.265 는 csd-0 에 모두 담으므로 null. */
    val csd1: ByteArray?,
    val width: Int,
    val height: Int,
) {

    val isH265: Boolean get() = mime == MediaFormat.MIMETYPE_VIDEO_HEVC

    /** 코덱 이름만 짧게(로그용). */
    val codecName: String get() = if (isH265) "H.265" else "H.264"

    fun sameAs(other: StreamFormat?): Boolean {
        if (other == null) return false
        if (mime != other.mime) return false
        if (!csd0.contentEquals(other.csd0)) return false
        val a = csd1
        val b = other.csd1
        return if (a == null || b == null) a == null && b == null else a.contentEquals(b)
    }

    /** MediaCodec / MediaMuxer 에 넘길 MediaFormat 을 만든다. */
    fun toMediaFormat(): MediaFormat {
        val w = if (width > 0) width else 1280
        val h = if (height > 0) height else 720
        return MediaFormat.createVideoFormat(mime, w, h).apply {
            setByteBuffer("csd-0", java.nio.ByteBuffer.wrap(csd0))
            csd1?.let { setByteBuffer("csd-1", java.nio.ByteBuffer.wrap(it)) }
        }
    }

    companion object {
        val START_CODE = byteArrayOf(0, 0, 0, 1)

        /** start code 가 없으면 붙여서 돌려준다. */
        fun withStartCode(nal: ByteArray): ByteArray {
            if (nal.size >= 3 && nal[0] == 0.toByte() && nal[1] == 0.toByte() &&
                (nal[2] == 1.toByte() || (nal.size >= 4 && nal[2] == 0.toByte() && nal[3] == 1.toByte()))
            ) {
                return nal
            }
            return START_CODE + nal
        }

        fun h264(sps: ByteArray, pps: ByteArray, width: Int, height: Int) = StreamFormat(
            MediaFormat.MIMETYPE_VIDEO_AVC,
            withStartCode(sps),
            withStartCode(pps),
            width,
            height,
        )

        fun h265(vps: ByteArray, sps: ByteArray, pps: ByteArray, width: Int, height: Int) =
            StreamFormat(
                MediaFormat.MIMETYPE_VIDEO_HEVC,
                withStartCode(vps) + withStartCode(sps) + withStartCode(pps),
                null,
                width,
                height,
            )
    }
}
