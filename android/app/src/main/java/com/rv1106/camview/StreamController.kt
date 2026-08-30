package com.rv1106.camview

import android.os.Handler
import android.os.Looper
import android.util.Log
import android.view.Surface
import com.rv1106.camview.codec.StreamFormat
import com.rv1106.camview.codec.VideoDecoder
import com.rv1106.camview.record.Mp4Recorder
import com.rv1106.camview.rtsp.RtspClient
import java.io.File

/**
 * RTSP 수신 → 디코딩(화면 표시) → 녹화 를 하나로 묶는 컨트롤러.
 *
 * 콜백은 모두 메인 스레드로 넘겨서 UI 코드가 스레드를 신경 쓰지 않게 한다.
 */
class StreamController(private val listener: Listener) {

    interface Listener {
        fun onStatus(status: Status, message: String?)
        fun onVideoSize(width: Int, height: Int)
        fun onFirstFrame()
        fun onRecordingTick(durationMs: Long, waitingForKeyFrame: Boolean)
    }

    enum class Status { IDLE, CONNECTING, PLAYING, RECONNECTING, ERROR }

    private val main = Handler(Looper.getMainLooper())
    private val lock = Object()

    private var client: RtspClient? = null
    /**
     * 연결 세대. 끊고 바로 다시 연결하면 이전 연결이 뒤늦게 보내는 콜백이
     * 새 연결의 상태를 덮어쓸 수 있어서, 세대가 다른 콜백은 무시한다.
     */
    private var generation = 0
    private val recorder = Mp4Recorder()
    private var decoder: VideoDecoder? = null

    private var surface: Surface? = null
    private var format: StreamFormat? = null

    @Volatile var status: Status = Status.IDLE
        private set

    val isRecording: Boolean get() = recorder.isRecording
    val isConnected: Boolean get() = client?.isRunning == true

    private val recordTicker = object : Runnable {
        override fun run() {
            if (recorder.isRecording) {
                listener.onRecordingTick(recorder.durationMs, recorder.waitingForKeyFrame)
                main.postDelayed(this, 500)
            }
        }
    }

    // ------------------------------------------------------------------ 연결

    fun connect(url: String, username: String?, password: String?) {
        disconnect()
        synchronized(lock) { format = null }
        generation++
        val c = RtspClient(url, username?.ifBlank { null }, password, ClientListener(generation))
        client = c
        c.start()
    }

    fun disconnect() {
        generation++
        client?.stop()
        client = null
        stopRecording()
        synchronized(lock) {
            decoder?.stop()
            decoder = null
        }
        updateStatus(Status.IDLE, null)
    }

    fun setSurface(newSurface: Surface?) {
        synchronized(lock) {
            surface = newSurface
            if (newSurface == null) {
                decoder?.stop()
                decoder = null
            } else {
                startDecoderLocked()
            }
        }
    }

    // ------------------------------------------------------------------ 녹화

    /** @return 녹화를 시작했으면 true. 아직 영상 정보가 없으면 false. */
    fun startRecording(target: File): Boolean {
        val f = synchronized(lock) { format }
        if (f == null || f.width <= 0 || f.height <= 0) return false
        val started = recorder.start(target, f)
        if (started) {
            main.removeCallbacks(recordTicker)
            main.post(recordTicker)
        }
        return started
    }

    fun stopRecording(): File? {
        if (!recorder.isRecording) return null
        main.removeCallbacks(recordTicker)
        return recorder.stop()
    }

    fun release() {
        disconnect()
        main.removeCallbacks(recordTicker)
    }

    // ------------------------------------------------------- RtspClient.Listener

    private inner class ClientListener(private val gen: Int) : RtspClient.Listener {

        override fun onFormat(format: StreamFormat) {
            if (gen == generation) this@StreamController.onFormat(format)
        }

        override fun onAccessUnit(au: ByteArray, isKeyFrame: Boolean, ptsUs: Long) {
            if (gen == generation) this@StreamController.onAccessUnit(au, isKeyFrame, ptsUs)
        }

        override fun onState(state: RtspClient.State, message: String?) {
            if (gen == generation) this@StreamController.onState(state, message)
        }
    }

    private fun onFormat(newFormat: StreamFormat) {
        synchronized(lock) {
            val changed = !newFormat.sameAs(format)
            format = newFormat
            if (changed) {
                decoder?.stop()
                decoder = null
                startDecoderLocked()
            }
        }
        if (newFormat.width > 0 && newFormat.height > 0) {
            main.post { listener.onVideoSize(newFormat.width, newFormat.height) }
        }
    }

    private fun onAccessUnit(au: ByteArray, isKeyFrame: Boolean, ptsUs: Long) {
        val d = synchronized(lock) { decoder }
        d?.queue(au, ptsUs, isKeyFrame)
        recorder.write(au, isKeyFrame, ptsUs)
    }

    private fun onState(state: RtspClient.State, message: String?) {
        val mapped = when (state) {
            RtspClient.State.CONNECTING -> Status.CONNECTING
            RtspClient.State.PLAYING -> Status.PLAYING
            RtspClient.State.RECONNECTING -> Status.RECONNECTING
            RtspClient.State.STOPPED -> Status.IDLE
            RtspClient.State.ERROR -> Status.ERROR
        }
        if (state == RtspClient.State.RECONNECTING) {
            // 연결이 끊기면 디코더는 다음 파라미터셋을 받고 새로 시작한다.
            synchronized(lock) {
                decoder?.stop()
                decoder = null
                format = null
            }
        }
        updateStatus(mapped, message)
    }

    // ------------------------------------------------------------------ 내부

    private fun startDecoderLocked() {
        val surf = surface
        val f = format
        if (surf == null || f == null) {
            Log.i(TAG, "디코더 대기 중: surface=${surf != null} format=${f != null}")
            return
        }
        if (decoder?.isRunning == true) return
        val d = VideoDecoder(
            onError = { msg ->
                Log.e(TAG, "디코더 오류: $msg")
                main.post { listener.onStatus(Status.ERROR, msg) }
            },
            onFirstFrame = { main.post { listener.onFirstFrame() } },
        )
        try {
            d.start(surf, f)
            decoder = d
        } catch (e: Exception) {
            Log.e(TAG, "디코더 시작 실패", e)
            main.post { listener.onStatus(Status.ERROR, "디코더를 시작할 수 없습니다: ${e.message}") }
        }
    }

    private fun updateStatus(newStatus: Status, message: String?) {
        status = newStatus
        main.post { listener.onStatus(newStatus, message) }
    }

    companion object {
        private const val TAG = "StreamController"
    }
}
