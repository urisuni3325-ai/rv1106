package com.rv1106.camview.rtsp


/**
 * RTP(RFC 7798) → H.265(HEVC) Annex-B 재조립기.
 *
 * H.264 와 달리 NAL 헤더가 2바이트다.
 * ```
 * byte0: F(1) | Type(6) | LayerId 상위 1비트
 * byte1: LayerId 하위 5비트 | TID(3)
 * ```
 * 페이로드 타입은 0~47 단일 NAL, 48 AP(집합), 49 FU(분할)이다.
 */
class RtpH265Depacketizer(callback: Callback) : RtpDepacketizer(callback) {

    private var fuHeaderByte0: Byte = 0
    private var fuHeaderByte1: Byte = 0

    /** 16~21 이 IRAP(BLA/IDR/CRA). 이 구간이 오면 디코딩을 새로 시작할 수 있다. */
    override fun isKeyFrameNal(nalType: Int): Boolean = nalType in 16..21

    override fun parsePayload(packet: ByteArray, offset: Int, end: Int) {
        if (offset + 2 > end) return
        when (val payloadType = (packet[offset].toInt() ushr 1) and 0x3F) {
            in 0..47 -> appendNal(packet, offset, end - offset, payloadType)
            AP -> parseAggregation(packet, offset + 2, end)
            FU -> parseFragment(packet, offset, end)
            else -> reportMalformed(payloadType)
        }
    }

    /** AP: 2바이트 페이로드 헤더 뒤에 [길이(2바이트) + NAL] 이 이어진다. */
    private fun parseAggregation(packet: ByteArray, start: Int, end: Int) {
        var p = start
        while (p + 2 <= end) {
            val size = ((packet[p].toInt() and 0xFF) shl 8) or (packet[p + 1].toInt() and 0xFF)
            p += 2
            if (size < 2 || p + size > end) return
            appendNal(packet, p, size, (packet[p].toInt() ushr 1) and 0x3F)
            p += size
        }
    }

    /** FU: 페이로드 헤더 2바이트 + FU 헤더 1바이트(S|E|FuType). */
    private fun parseFragment(packet: ByteArray, offset: Int, end: Int) {
        if (offset + 3 > end) return
        val fuHeader = packet[offset + 2].toInt()
        val start = (fuHeader and 0x80) != 0
        val stop = (fuHeader and 0x40) != 0
        val nalType = fuHeader and 0x3F

        if (start) {
            fu.reset()
            fuActive = true
            // 원래 NAL 헤더를 복원한다. byte0 의 Type 자리(비트 1~6)만 FU 타입으로 바꾼다.
            fuHeaderByte0 = ((packet[offset].toInt() and 0x81) or (nalType shl 1)).toByte()
            fuHeaderByte1 = packet[offset + 1]
            fu.write(fuHeaderByte0.toInt())
            fu.write(fuHeaderByte1.toInt())
        }
        if (!fuActive) return

        fu.write(packet, offset + 3, end - offset - 3)

        if (stop) {
            fuActive = false
            val nal = fu.toByteArray()
            fu.reset()
            appendNal(nal, 0, nal.size, nalType)
        }
    }

    companion object {
        private const val AP = 48
        private const val FU = 49
    }
}
