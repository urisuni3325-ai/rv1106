package com.rv1106.camview.rtsp

import android.util.Log
import java.io.ByteArrayOutputStream

/**
 * RTP(RFC 6184) → H.264 Annex-B 재조립기.
 *
 * Single NAL / STAP-A(24) / FU-A(28) 를 지원한다. 같은 타임스탬프의 NAL 들을 모아
 * marker 비트나 타임스탬프 변화 시점에 액세스 유닛 하나로 넘긴다.
 */
class RtpH264Depacketizer(private val callback: Callback) {

    interface Callback {
        fun onNalUnits(au: ByteArray, isKeyFrame: Boolean, rtpTimestamp: Long)
    }

    private val au = ByteArrayOutputStream(256 * 1024)
    private var auTimestamp = -1L
    private var auHasKeyFrame = false
    private var auHasContent = false

    private val fu = ByteArrayOutputStream(256 * 1024)
    private var fuActive = false
    private var fuNalHeader: Byte = 0

    private var expectedSeq = -1
    private var corrupt = false
    private var waitingForKeyFrame = true

    fun reset() {
        au.reset()
        fu.reset()
        auTimestamp = -1L
        auHasKeyFrame = false
        auHasContent = false
        fuActive = false
        expectedSeq = -1
        corrupt = false
        waitingForKeyFrame = true
    }

    fun process(packet: ByteArray, length: Int) {
        if (length < 12) return
        val version = (packet[0].toInt() and 0xC0) ushr 6
        if (version != 2) return

        val hasPadding = (packet[0].toInt() and 0x20) != 0
        val hasExtension = (packet[0].toInt() and 0x10) != 0
        val csrcCount = packet[0].toInt() and 0x0F
        val marker = (packet[1].toInt() and 0x80) != 0
        val seq = ((packet[2].toInt() and 0xFF) shl 8) or (packet[3].toInt() and 0xFF)
        val timestamp = ((packet[4].toLong() and 0xFF) shl 24) or
            ((packet[5].toLong() and 0xFF) shl 16) or
            ((packet[6].toLong() and 0xFF) shl 8) or
            (packet[7].toLong() and 0xFF)

        var offset = 12 + csrcCount * 4
        if (hasExtension) {
            if (offset + 4 > length) return
            val extWords = ((packet[offset + 2].toInt() and 0xFF) shl 8) or
                (packet[offset + 3].toInt() and 0xFF)
            offset += 4 + extWords * 4
        }
        var end = length
        if (hasPadding && end > offset) {
            end -= packet[end - 1].toInt() and 0xFF
        }
        if (offset >= end) return

        // 시퀀스 번호가 튀면(패킷 유실) 현재 액세스 유닛을 버리고 다음 키프레임까지 기다린다.
        if (expectedSeq >= 0 && seq != expectedSeq) {
            Log.w(TAG, "패킷 유실: expected=$expectedSeq actual=$seq")
            corrupt = true
            fuActive = false
            fu.reset()
        }
        expectedSeq = (seq + 1) and 0xFFFF

        if (auTimestamp >= 0 && timestamp != auTimestamp) {
            emit()
        }
        auTimestamp = timestamp

        when (val nalType = packet[offset].toInt() and 0x1F) {
            in 1..23 -> appendNal(packet, offset, end - offset, nalType)
            STAP_A -> parseStapA(packet, offset + 1, end)
            FU_A -> parseFuA(packet, offset, end)
            else -> Log.w(TAG, "지원하지 않는 RTP 페이로드 타입: $nalType")
        }

        if (marker) emit()
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

    private fun appendNal(data: ByteArray, offset: Int, length: Int, nalType: Int) {
        if (length <= 0) return
        if (nalType == NAL_IDR) auHasKeyFrame = true
        au.write(START_CODE, 0, 4)
        au.write(data, offset, length)
        auHasContent = true
    }

    private fun emit() {
        if (!auHasContent) {
            resetAu()
            return
        }
        val data = au.toByteArray()
        val keyFrame = auHasKeyFrame
        val ts = auTimestamp
        val wasCorrupt = corrupt
        resetAu()

        if (wasCorrupt) {
            // 깨진 액세스 유닛은 디코더에 넣지 않는다. 다음 키프레임부터 재개.
            waitingForKeyFrame = true
            corrupt = false
            return
        }
        if (waitingForKeyFrame) {
            if (!keyFrame) return
            waitingForKeyFrame = false
        }
        callback.onNalUnits(data, keyFrame, ts)
    }

    private fun resetAu() {
        au.reset()
        auHasKeyFrame = false
        auHasContent = false
        auTimestamp = -1L
    }

    companion object {
        private const val TAG = "RtpH264"
        private const val STAP_A = 24
        private const val FU_A = 28
        private const val NAL_IDR = 5
        private val START_CODE = byteArrayOf(0, 0, 0, 1)
    }
}
