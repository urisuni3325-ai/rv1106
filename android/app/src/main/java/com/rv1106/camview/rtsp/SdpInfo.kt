package com.rv1106.camview.rtsp

import android.util.Base64
import android.util.Log

/** DESCRIBE 응답(SDP)에서 H.264 비디오 트랙 정보만 뽑아낸다. */
class SdpInfo(
    val control: String?,
    val payloadType: Int,
    /** rtpmap 에 적힌 코덱 이름. 예: `H264`, `H265`. 없으면 null. */
    val encoding: String?,
    val sps: ByteArray?,
    val pps: ByteArray?,
) {
    companion object {
        private const val TAG = "SdpInfo"

        fun parse(sdp: String): SdpInfo? {
            var inVideo = false
            var control: String? = null
            var payloadType = 96
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
                        val props = attr.substringAfter(' ', "").split(';')
                        for (prop in props) {
                            val p = prop.trim()
                            if (p.startsWith("sprop-parameter-sets=", ignoreCase = true)) {
                                val sets = p.substringAfter('=').split(',')
                                sps = decode(sets.getOrNull(0))
                                pps = decode(sets.getOrNull(1))
                            }
                        }
                    }
                }
            }
            if (!sdp.contains("m=video")) return null
            return SdpInfo(control ?: sessionControl, payloadType, encoding, sps, pps)
        }

        private fun decode(b64: String?): ByteArray? {
            if (b64.isNullOrBlank()) return null
            return try {
                Base64.decode(b64.trim(), Base64.NO_WRAP)
            } catch (e: IllegalArgumentException) {
                Log.w(TAG, "sprop-parameter-sets 디코딩 실패: ${e.message}")
                null
            }
        }
    }
}
