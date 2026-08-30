package com.rv1106.camview.codec

import android.media.MediaCodec
import android.media.MediaFormat
import android.os.Build
import android.os.Handler
import android.os.HandlerThread
import android.util.Log
import android.view.Surface
/**
 * H.264 하드웨어 디코더. 들어온 액세스 유닛을 곧바로 Surface 에 그린다.
 *
 * 지연을 줄이기 위해 (1) 출력 버퍼를 pts 와 무관하게 즉시 렌더하고,
 * (2) 입력이 밀리면 다음 키프레임까지 프레임을 버린다.
 */
class VideoDecoder(
    private val onError: (String) -> Unit,
    private val onFirstFrame: () -> Unit,
) {

    private var codec: MediaCodec? = null
    private var callbackThread: HandlerThread? = null

    private val lock = Object()
    private val availableInputs = ArrayDeque<Int>()
    private val pendingFrames = ArrayDeque<Frame>()
    private var firstFrameRendered = false

    @Volatile var isRunning = false
        private set

    var width = 0
        private set
    var height = 0
        private set

    private class Frame(val data: ByteArray, val ptsUs: Long, val isKeyFrame: Boolean)

    fun start(surface: Surface, streamFormat: StreamFormat) {
        stop()
        width = if (streamFormat.width > 0) streamFormat.width else 1280
        height = if (streamFormat.height > 0) streamFormat.height else 720

        val format = streamFormat.toMediaFormat().apply {
            setInteger(MediaFormat.KEY_MAX_INPUT_SIZE, width * height)
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
                setInteger(MediaFormat.KEY_LOW_LATENCY, 1)
            }
            // 일부 벤더(락칩 포함) 디코더가 인식하는 저지연 힌트
            setInteger("vdec-lowlatency", 1)
        }

        val thread = HandlerThread("video-decoder").also { it.start() }
        callbackThread = thread

        val mc = MediaCodec.createDecoderByType(streamFormat.mime)
        mc.setCallback(object : MediaCodec.Callback() {
            override fun onInputBufferAvailable(c: MediaCodec, index: Int) {
                synchronized(lock) { availableInputs.addLast(index) }
                drain()
            }

            override fun onOutputBufferAvailable(
                c: MediaCodec,
                index: Int,
                info: MediaCodec.BufferInfo,
            ) {
                try {
                    c.releaseOutputBuffer(index, info.size > 0)
                } catch (e: IllegalStateException) {
                    return
                }
                if (!firstFrameRendered && info.size > 0) {
                    firstFrameRendered = true
                    onFirstFrame()
                }
            }

            override fun onError(c: MediaCodec, e: MediaCodec.CodecException) {
                Log.e(TAG, "디코더 오류", e)
                this@VideoDecoder.onError(e.message ?: "디코더 오류")
            }

            override fun onOutputFormatChanged(c: MediaCodec, format: MediaFormat) {
                width = runCatching { format.getInteger(MediaFormat.KEY_WIDTH) }.getOrDefault(width)
                height = runCatching { format.getInteger(MediaFormat.KEY_HEIGHT) }.getOrDefault(height)
                Log.i(TAG, "출력 포맷: ${width}x$height")
            }
        }, Handler(thread.looper))

        mc.configure(format, surface, null, 0)
        mc.start()
        codec = mc
        firstFrameRendered = false
        isRunning = true
        Log.i(TAG, "디코더 시작 ${streamFormat.codecName} ${width}x$height")
    }

    fun queue(au: ByteArray, ptsUs: Long, isKeyFrame: Boolean) {
        if (!isRunning) return
        synchronized(lock) {
            pendingFrames.addLast(Frame(au, ptsUs, isKeyFrame))
            // 밀리면 오래된 프레임부터 버리되, 키프레임 경계를 지켜 화면 깨짐을 막는다.
            while (pendingFrames.size > MAX_PENDING) {
                pendingFrames.removeFirst()
                while (pendingFrames.isNotEmpty() && !pendingFrames.first().isKeyFrame) {
                    pendingFrames.removeFirst()
                }
            }
        }
        drain()
    }

    private fun drain() {
        val mc = codec ?: return
        while (true) {
            var index = -1
            var frame: Frame? = null
            synchronized(lock) {
                if (availableInputs.isEmpty() || pendingFrames.isEmpty()) return
                index = availableInputs.removeFirst()
                frame = pendingFrames.removeFirst()
            }
            val f = frame ?: return
            try {
                val buffer = mc.getInputBuffer(index) ?: return
                buffer.clear()
                if (buffer.capacity() < f.data.size) {
                    Log.w(TAG, "입력 버퍼가 작아 프레임을 건너뜁니다 (${f.data.size})")
                    mc.queueInputBuffer(index, 0, 0, f.ptsUs, 0)
                    continue
                }
                buffer.put(f.data)
                val flags = if (f.isKeyFrame) MediaCodec.BUFFER_FLAG_KEY_FRAME else 0
                mc.queueInputBuffer(index, 0, f.data.size, f.ptsUs, flags)
            } catch (e: IllegalStateException) {
                return
            }
        }
    }

    fun stop() {
        isRunning = false
        val mc = codec
        codec = null
        if (mc != null) {
            runCatching { mc.stop() }
            runCatching { mc.release() }
        }
        callbackThread?.quitSafely()
        callbackThread = null
        synchronized(lock) {
            availableInputs.clear()
            pendingFrames.clear()
        }
        firstFrameRendered = false
    }

    companion object {
        private const val TAG = "VideoDecoder"
        private const val MAX_PENDING = 8
    }
}
