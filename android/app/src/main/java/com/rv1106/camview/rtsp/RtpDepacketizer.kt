package com.rv1106.camview.rtsp

import android.util.Log
import java.io.ByteArrayOutputStream

/**
 * RTP 패킷을 Annex-B 액세스 유닛으로 재조립하는 공통 부분.
 *
 * RTP 헤더 해석, 액세스 유닛 경계 판단(marker 비트 또는 타임스탬프 변화),
 * 패킷 유실 처리는 코덱과 무관하므로 여기에 모아 둔다. 코덱별로 다른
 * 페이로드 구조는 [parsePayload] 에서 처리한다.
 */
abstract class RtpDepacketizer(private val callback: Callback) {

    interface Callback {
        fun onNalUnits(au: ByteArray, isKeyFrame: Boolean, rtpTimestamp: Long)
    }

    private val au = ByteArrayOutputStream(256 * 1024)
    private var auTimestamp = -1L
    private var auHasKeyFrame = false
    private var auHasContent = false

    /** FU 조각을 모으는 버퍼. 하위 클래스가 채운다. */
    protected val fu = ByteArrayOutputStream(256 * 1024)
    protected var fuActive = false

    private var expectedSeq = -1
    private var corrupt = false
    private var waitingForKeyFrame = true

    /**
     * 이 코덱으로는 해석할 수 없는 페이로드를 만난 횟수.
     * 코덱 판단이 틀렸는지 가리는 신호로 쓴다.
     */
    @Volatile var malformedCount = 0
        private set

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
        malformedCount = 0
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

        parsePayload(packet, offset, end)

        if (marker) emit()
    }

    /** 코덱별 페이로드 해석. 조립한 NAL 은 [appendNal] 로 넘긴다. */
    protected abstract fun parsePayload(packet: ByteArray, offset: Int, end: Int)

    /** 이 NAL 이 키프레임(IDR/IRAP)인지. */
    protected abstract fun isKeyFrameNal(nalType: Int): Boolean

    /** 하위 클래스가 모르는 페이로드 타입을 만났을 때 부른다. */
    protected fun reportMalformed(payloadType: Int) {
        malformedCount++
        if (malformedCount <= 3) {
            Log.w(TAG, "해석할 수 없는 RTP 페이로드 타입: $payloadType (누적 $malformedCount)")
        }
    }

    protected fun appendNal(data: ByteArray, offset: Int, length: Int, nalType: Int) {
        if (length <= 0) return
        if (isKeyFrameNal(nalType)) auHasKeyFrame = true
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
        private const val TAG = "RtpDepacketizer"
        private val START_CODE = byteArrayOf(0, 0, 0, 1)
    }
}
