package com.rv1106.camview.rtsp


/**
 * RTP(RFC 6184) → H.264 Annex-B 재조립기.
 *
 * Single NAL / STAP-A(24) / FU-A(28) 를 지원한다.
 */
class RtpH264Depacketizer(callback: Callback) : RtpDepacketizer(callback) {

    private var fuNalHeader: Byte = 0

    override fun isKeyFrameNal(nalType: Int): Boolean = nalType == NAL_IDR

    override fun parsePayload(packet: ByteArray, offset: Int, end: Int) {
        when (val nalType = packet[offset].toInt() and 0x1F) {
            in 1..23 -> appendNal(packet, offset, end - offset, nalType)
            STAP_A -> parseStapA(packet, offset + 1, end)
            FU_A -> parseFuA(packet, offset, end)
            else -> reportMalformed(nalType)
        }
    }

    private fun parseStapA(packet: ByteArray, start: Int, end: Int) {
        var p = start
        while (p + 2 <= end) {
            val size = ((packet[p].toInt() and 0xFF) shl 8) or (packet[p + 1].toInt() and 0xFF)
            p += 2
            if (size <= 0 || p + size > end) return
            appendNal(packet, p, size, packet[p].toInt() and 0x1F)
            p += size
        }
    }

    private fun parseFuA(packet: ByteArray, offset: Int, end: Int) {
        if (offset + 2 > end) return
        val indicator = packet[offset]
        val header = packet[offset + 1]
        val start = (header.toInt() and 0x80) != 0
        val stop = (header.toInt() and 0x40) != 0
        val nalType = header.toInt() and 0x1F

        if (start) {
            fu.reset()
            fuActive = true
            fuNalHeader = ((indicator.toInt() and 0xE0) or nalType).toByte()
            fu.write(fuNalHeader.toInt())
        }
        if (!fuActive) return

        fu.write(packet, offset + 2, end - offset - 2)

        if (stop) {
            fuActive = false
            val nal = fu.toByteArray()
            fu.reset()
            appendNal(nal, 0, nal.size, nalType)
        }
    }

    companion object {
        private const val STAP_A = 24
        private const val FU_A = 28
        private const val NAL_IDR = 5
    }
}
