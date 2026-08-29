package com.rv1106.camview.record

import android.media.MediaCodec
import android.media.MediaFormat
import android.media.MediaMuxer
import android.util.Log
import com.rv1106.camview.codec.VideoDecoder
import java.io.File
import java.nio.ByteBuffer

/**
 * 수신한 H.264 스트림을 재인코딩 없이 그대로 MP4 로 저장한다.
 *
 * 재인코딩을 하지 않으므로 CPU/배터리 소모가 거의 없고 원본 화질이 그대로 남는다.
 * 대신 첫 키프레임(IDR)이 도착해야 파일 기록이 시작된다.
 */
class Mp4Recorder {

    private var muxer: MediaMuxer? = null
    private var trackIndex = -1
    private var file: File? = null
    private var firstPtsUs = -1L
    private var lastPtsUs = 0L
    private var sampleCount = 0
    private val bufferInfo = MediaCodec.BufferInfo()

    @Volatile var isRecording = false
        private set

    /** 아직 키프레임을 기다리는 중이면 true(사용자에게 "녹화 준비 중"을 보여주기 위함). */
    @Volatile var waitingForKeyFrame = false
        private set

    val outputFile: File? get() = file

    /** 녹화된 길이(ms). */
    val durationMs: Long
        get() = if (firstPtsUs < 0) 0 else (lastPtsUs - firstPtsUs) / 1000

    @Synchronized
    fun start(target: File, sps: ByteArray, pps: ByteArray, width: Int, height: Int): Boolean {
        if (isRecording) return false
        return try {
            target.parentFile?.mkdirs()
            val mx = MediaMuxer(target.absolutePath, MediaMuxer.OutputFormat.MUXER_OUTPUT_MPEG_4)
            val format = MediaFormat.createVideoFormat(MIME, width, height).apply {
                setByteBuffer("csd-0", ByteBuffer.wrap(VideoDecoder.withStartCode(sps)))
                setByteBuffer("csd-1", ByteBuffer.wrap(VideoDecoder.withStartCode(pps)))
            }
            trackIndex = mx.addTrack(format)
            mx.start()
            muxer = mx
            file = target
            firstPtsUs = -1L
            lastPtsUs = 0L
            sampleCount = 0
            waitingForKeyFrame = true
            isRecording = true
            Log.i(TAG, "녹화 시작: ${target.absolutePath} (${width}x$height)")
            true
        } catch (e: Exception) {
            Log.e(TAG, "녹화 시작 실패", e)
            runCatching { muxer?.release() }
            muxer = null
            file = null
            isRecording = false
            waitingForKeyFrame = false
            false
        }
    }

    @Synchronized
    fun write(au: ByteArray, isKeyFrame: Boolean, ptsUs: Long) {
        val mx = muxer ?: return
        if (!isRecording) return
        if (firstPtsUs < 0) {
            if (!isKeyFrame) return // 키프레임부터 시작해야 재생이 깨지지 않는다
            firstPtsUs = ptsUs
            waitingForKeyFrame = false
        }
        // pts 는 반드시 단조증가해야 한다(MediaMuxer 요구사항).
        val pts = (ptsUs - firstPtsUs).coerceAtLeast(lastPtsUs - firstPtsUs)
        try {
            bufferInfo.offset = 0
            bufferInfo.size = au.size
            bufferInfo.presentationTimeUs = pts
            bufferInfo.flags = if (isKeyFrame) MediaCodec.BUFFER_FLAG_KEY_FRAME else 0
            mx.writeSampleData(trackIndex, ByteBuffer.wrap(au), bufferInfo)
            lastPtsUs = firstPtsUs + pts
            sampleCount++
        } catch (e: Exception) {
            Log.e(TAG, "샘플 기록 실패", e)
        }
    }

    /** @return 저장된 파일. 저장할 내용이 없으면 null(파일은 삭제된다). */
    @Synchronized
    fun stop(): File? {
        val mx = muxer ?: return null
        val target = file
        muxer = null
        file = null
        isRecording = false
        waitingForKeyFrame = false

        var saved: File? = null
        try {
            if (sampleCount > 0) {
                mx.stop()
                saved = target
            }
        } catch (e: Exception) {
            Log.e(TAG, "녹화 종료 실패", e)
        } finally {
            runCatching { mx.release() }
        }
        if (saved == null) {
            target?.delete()
            Log.w(TAG, "저장된 프레임이 없어 파일을 삭제했습니다")
        } else {
            Log.i(TAG, "녹화 완료: ${saved.absolutePath} (${sampleCount}프레임)")
        }
        sampleCount = 0
        firstPtsUs = -1L
        lastPtsUs = 0L
        return saved
    }

    companion object {
        private const val TAG = "Mp4Recorder"
        private const val MIME = MediaFormat.MIMETYPE_VIDEO_AVC
    }
}
