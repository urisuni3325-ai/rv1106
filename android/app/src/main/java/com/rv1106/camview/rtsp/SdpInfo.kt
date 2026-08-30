package com.rv1106.camview.rtsp

import android.util.Base64
import android.util.Log

/** DESCRIBE 응답(SDP)에서 비디오 트랙 정보만 뽑아낸다. */
class SdpInfo(
    val control: String?,
    val payloadType: Int,
    /** rtpmap 에 적힌 코덱 이름. 예: `H264`, `H265`. 없으면 null. */
    val encoding: String?,
    /** H.265 의 sprop-vps. H.264 는 항상 null. */
    val vps: ByteArray?,
    val sps: ByteArray?,
    val pps: ByteArray?,
) {
    companion object {
        private const val TAG = "SdpInfo"

        fun parse(sdp: String): SdpInfo? {
            var inVideo = false
            var control: String? = null
            var payloadType = 96
            var vps: ByteArray? = null
            var sps: ByteArray? = null
            var pps: ByteArray? = null
            var sessionControl: String? = null
            var encoding: String? = null

            for (raw in sdp.split('\n')) {
                val line = raw.trim()
                if (line.isEmpty()) continue

                if (line.startsWith("m=")) {
                    inVideo = line.startsWith("m=video")
                    if (inVideo) {
                        payloadType = line.split(' ').lastOrNull()?.toIntOrNull() ?: 96
                    }
                    continue
                }
                if (!line.startsWith("a=")) continue
                val attr = line.substring(2)

                when {
                    attr.startsWith("control:") -> {
                        val v = attr.substring(8).trim()
                        if (inVideo) control = v else sessionControl = v
                    }
                    inVideo && attr.startsWith("rtpmap:") -> {
                        // "rtpmap:96 H264/90000" → "H264"
                        encoding = attr.substringAfter(' ', "")
                            .substringBefore('/')
                            .trim()
                            .ifEmpty { null }
                    }
                    inVideo && attr.startsWith("fmtp:") -> {
                        for (prop in attr.substringAfter(' ', "").split(';')) {
                            val p = prop.trim()
                            val name = p.substringBefore('=').lowercase()
                            when (name) {
                                // H.264 는 SPS,PPS 를 콤마로 이어 한 항목에 담는다.
                                "sprop-parameter-sets" -> {
                                    val sets = p.substringAfter('=').split(',')
                                    sps = decode(sets.getOrNull(0))
                                    pps = decode(sets.getOrNull(1))
                                }
                                // H.265 는 항목이 셋으로 나뉜다.
                                "sprop-vps" -> vps = decode(p.substringAfter('='))
                                "sprop-sps" -> sps = decode(p.substringAfter('='))
                                "sprop-pps" -> pps = decode(p.substringAfter('='))
                            }
                        }
                    }
                }
            }
            if (!sdp.contains("m=video")) return null
            return SdpInfo(control ?: sessionControl, payloadType, encoding, vps, sps, pps)
        }

        private fun decode(b64: String?): ByteArray? {
            if (b64.isNullOrBlank()) return null
            return try {
                Base64.decode(b64.trim(), Base64.NO_WRAP)
            } catch (e: IllegalArgumentException) {
                Log.w(TAG, "파라미터셋 디코딩 실패: ${e.message}")
                null
            }
        }
    }
}
