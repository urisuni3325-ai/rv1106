package com.rv1106.camview

import android.content.Intent
import android.graphics.Bitmap
import android.graphics.SurfaceTexture
import android.os.Bundle
import android.util.Log
import android.view.GestureDetector
import android.view.MotionEvent
import android.view.ScaleGestureDetector
import android.view.Surface
import android.view.TextureView
import android.view.View
import android.view.WindowManager
import android.widget.Toast
import androidx.appcompat.app.AlertDialog
import androidx.appcompat.app.AppCompatActivity
import com.rv1106.camview.capture.CaptureFormat
import com.rv1106.camview.capture.JpegDensity
import com.rv1106.camview.databinding.ActivityMainBinding
import com.rv1106.camview.databinding.DialogSettingsBinding
import com.rv1106.camview.gallery.GalleryActivity
import com.rv1106.camview.gallery.MediaRepository
import com.rv1106.camview.net.BoardFinder
import com.rv1106.camview.ui.ZoomState
import java.io.ByteArrayOutputStream
import java.io.File
import java.io.FileOutputStream
import java.util.Locale
import java.util.concurrent.Executors

class MainActivity : AppCompatActivity(), StreamController.Listener {

    private lateinit var binding: ActivityMainBinding
    private lateinit var prefs: Prefs
    private lateinit var controller: StreamController

    private val io = Executors.newSingleThreadExecutor()
    private var surface: Surface? = null

    private val zoom = ZoomState()
    private lateinit var scaleDetector: ScaleGestureDetector
    private lateinit var gestureDetector: GestureDetector

    /** 스트림 해상도. 캡처를 원본 크기로 저장하는 데 쓴다. */
    private var videoWidth = 0
    private var videoHeight = 0

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)
        window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)

        prefs = Prefs(this)
        controller = StreamController(this)

        binding.textureView.surfaceTextureListener = object : TextureView.SurfaceTextureListener {
            override fun onSurfaceTextureAvailable(st: SurfaceTexture, width: Int, height: Int) {
                surface = Surface(st)
                controller.setSurface(surface)
            }

            override fun onSurfaceTextureSizeChanged(st: SurfaceTexture, width: Int, height: Int) = Unit

            override fun onSurfaceTextureDestroyed(st: SurfaceTexture): Boolean {
                controller.setSurface(null)
                surface?.release()
                surface = null
                return true
            }

            override fun onSurfaceTextureUpdated(st: SurfaceTexture) = Unit
        }

        setUpZoomGestures()

        binding.btnConnect.setOnClickListener { toggleConnection() }
        binding.btnRecord.setOnClickListener { toggleRecording() }
        binding.btnSnapshot.setOnClickListener { takeSnapshot() }
        binding.btnGallery.setOnClickListener {
            startActivity(Intent(this, GalleryActivity::class.java))
        }
        binding.btnSettings.setOnClickListener { showSettings() }

        binding.textUrl.text = prefs.url
        updateButtons()
    }

    /**
     * 두피처럼 가까이 봐야 하는 대상은 화면에서 한 번 더 키우는 일이 잦다.
     * 손가락으로 확대·이동하고, 두 번 누르면 원래대로 돌아간다.
     */
    private fun setUpZoomGestures() {
        scaleDetector = ScaleGestureDetector(
            this,
            object : ScaleGestureDetector.SimpleOnScaleGestureListener() {
                override fun onScale(detector: ScaleGestureDetector): Boolean {
                    zoom.zoomBy(
                        detector.scaleFactor,
                        detector.focusX,
                        detector.focusY,
                        binding.textureView.width,
                        binding.textureView.height,
                    )
                    applyZoom()
                    return true
                }
            },
        )
        gestureDetector = GestureDetector(
            this,
            object : GestureDetector.SimpleOnGestureListener() {
                override fun onScroll(
                    e1: MotionEvent?,
                    e2: MotionEvent,
                    distanceX: Float,
                    distanceY: Float,
                ): Boolean {
                    zoom.panBy(
                        -distanceX,
                        -distanceY,
                        binding.textureView.width,
                        binding.textureView.height,
                    )
                    applyZoom()
                    return true
                }

                override fun onDoubleTap(e: MotionEvent): Boolean {
                    zoom.reset()
                    applyZoom()
                    return true
                }
            },
        )
        binding.videoContainer.setOnTouchListener { _, event ->
            scaleDetector.onTouchEvent(event)
            gestureDetector.onTouchEvent(event)
            true
        }
    }

    private fun applyZoom() {
        val view = binding.textureView
        if (view.width == 0 || view.height == 0) return
        view.setTransform(zoom.toMatrix(view.width, view.height))
        view.invalidate()
        binding.textZoom.visibility = if (zoom.isZoomed) View.VISIBLE else View.GONE
        binding.textZoom.text = zoom.label()
    }

    override fun onStart() {
        super.onStart()
        if (prefs.autoConnect && !controller.isConnected) connect()
    }

    override fun onStop() {
        super.onStop()
        // 화면을 벗어나면 녹화를 정리하고 연결을 끊어 배터리/트래픽을 아낀다.
        if (controller.isRecording) stopRecording()
        controller.disconnect()
        updateButtons()
    }

    override fun onDestroy() {
        super.onDestroy()
        controller.release()
        io.shutdown()
    }

    // ------------------------------------------------------------------ 동작

    private fun toggleConnection() {
        if (controller.isConnected) {
            if (controller.isRecording) stopRecording()
            controller.disconnect()
        } else {
            connect()
        }
        updateButtons()
    }

    private fun connect() {
        val url = prefs.url
        if (url.isBlank() || !url.startsWith("rtsp://")) {
            Toast.makeText(this, R.string.msg_invalid_url, Toast.LENGTH_LONG).show()
            showSettings()
            return
        }
        binding.textUrl.text = url
        controller.connect(url, prefs.username, prefs.password)
        updateButtons()
    }

    private fun toggleRecording() {
        if (controller.isRecording) {
            stopRecording()
        } else {
            val file = MediaRepository.newVideoFile(this)
            if (controller.startRecording(file)) {
                binding.recordingIndicator.visibility = View.VISIBLE
                binding.textRecordTime.text = getString(R.string.record_preparing)
            } else {
                Toast.makeText(this, R.string.msg_record_not_ready, Toast.LENGTH_SHORT).show()
            }
        }
        updateButtons()
    }

    private fun stopRecording() {
        val file = controller.stopRecording()
        binding.recordingIndicator.visibility = View.GONE
        if (file != null) {
            Toast.makeText(this, getString(R.string.msg_saved_video, file.name), Toast.LENGTH_SHORT).show()
        } else {
            Toast.makeText(this, R.string.msg_record_empty, Toast.LENGTH_SHORT).show()
        }
        updateButtons()
    }

    private fun takeSnapshot() {
        if (controller.status != StreamController.Status.PLAYING) {
            Toast.makeText(this, R.string.msg_snapshot_not_ready, Toast.LENGTH_SHORT).show()
            return
        }
        // 화면 크기가 아니라 스트림 원본 해상도로 받는다. 두피처럼 세밀한 대상은
        // 해상도가 그대로 관찰 품질이 된다.
        val bitmap: Bitmap? = if (videoWidth > 0 && videoHeight > 0) {
            runCatching { binding.textureView.getBitmap(videoWidth, videoHeight) }.getOrNull()
                ?: binding.textureView.bitmap
        } else {
            binding.textureView.bitmap
        }
        if (bitmap == null) {
            Toast.makeText(this, R.string.msg_snapshot_not_ready, Toast.LENGTH_SHORT).show()
            return
        }

        val aiFormat = prefs.captureForAi
        val keepOriginal = prefs.captureOriginalToo
        val photo = MediaRepository.newPhotoFile(this)
        val originalPhoto = MediaRepository.variantOf(photo, "_full")

        io.execute {
            val aiBitmap = if (aiFormat) toAiFormat(bitmap) else null
            // AI 규격 변환이 안 되면(프레임 크기가 이상한 경우) 원본이라도 남긴다.
            val saveOriginal = !aiFormat || keepOriginal || aiBitmap == null
            val originalFile = if (aiBitmap != null) originalPhoto else photo

            var first: SavedPhoto? = null
            var second: SavedPhoto? = null
            if (aiBitmap != null) {
                val saved = SavedPhoto(photo.name, aiBitmap.width, aiBitmap.height)
                if (saveJpeg(aiBitmap, photo)) first = saved
                if (aiBitmap !== bitmap) aiBitmap.recycle()
            }
            if (saveOriginal && saveJpeg(bitmap, originalFile)) {
                val saved = SavedPhoto(originalFile.name, bitmap.width, bitmap.height)
                if (first == null) first = saved else second = saved
            }

            runOnUiThread {
                val done = first
                val msg = when {
                    done == null -> getString(R.string.msg_snapshot_failed)
                    second != null -> getString(
                        R.string.msg_saved_photo_two,
                        done.name, done.width, done.height, second!!.name
                    )
                    else -> getString(R.string.msg_saved_photo, done.name, done.width, done.height)
                }
                Toast.makeText(this, msg, Toast.LENGTH_SHORT).show()
            }
        }
        // 촬영 피드백(화면 깜빡임)
        binding.flashOverlay.alpha = 1f
        binding.flashOverlay.animate().alpha(0f).setDuration(220).start()
    }

    private data class SavedPhoto(val name: String, val width: Int, val height: Int)

    /**
     * 스트림 프레임을 AI 분석 규격으로 바꾼다 — 가운데를 정사각형으로 잘라
     * 1000x1000 으로 줄인다. 크기를 못 구하면 null.
     */
    private fun toAiFormat(source: Bitmap): Bitmap? {
        val crop = CaptureFormat.centerSquare(source.width, source.height) ?: return null
        return runCatching {
            val square = Bitmap.createBitmap(source, crop.x, crop.y, crop.size, crop.size)
            val scaled =
                Bitmap.createScaledBitmap(square, CaptureFormat.SIZE, CaptureFormat.SIZE, true)
            // 자를 것이 없으면 createBitmap 이 원본을 그대로 돌려준다. 그건 남겨 둔다.
            if (scaled !== square && square !== source) square.recycle()
            scaled
        }.getOrNull()
    }

    /** 분석 쪽 규격에 맞춰 72dpi 를 적어 저장한다. */
    private fun saveJpeg(bitmap: Bitmap, file: File): Boolean = try {
        file.parentFile?.mkdirs()
        val buffer = ByteArrayOutputStream()
        bitmap.compress(Bitmap.CompressFormat.JPEG, 95, buffer)
        val jpeg = JpegDensity.apply(buffer.toByteArray(), CaptureFormat.DPI)
        FileOutputStream(file).use { out -> out.write(jpeg) }
        true
    } catch (e: Exception) {
        Log.e(TAG, "캡처 저장 실패", e)
        false
    }

    private fun showSettings() {
        val dialogBinding = DialogSettingsBinding.inflate(layoutInflater)
        dialogBinding.editUrl.setText(prefs.url)
        dialogBinding.editUser.setText(prefs.username)
        dialogBinding.editPassword.setText(prefs.password)
        dialogBinding.checkAutoConnect.isChecked = prefs.autoConnect
        dialogBinding.checkCaptureAi.isChecked = prefs.captureForAi
        dialogBinding.checkCaptureOriginal.isChecked = prefs.captureOriginalToo
        // AI 규격을 끄면 어차피 원본만 저장되므로 함께 저장 여부를 물을 필요가 없다.
        dialogBinding.checkCaptureOriginal.isEnabled = prefs.captureForAi
        dialogBinding.checkCaptureAi.setOnCheckedChangeListener { _, checked ->
            dialogBinding.checkCaptureOriginal.isEnabled = checked
        }
        dialogBinding.btnFind.setOnClickListener {
            findBoard(dialogBinding)
        }

        AlertDialog.Builder(this)
            .setTitle(R.string.settings_title)
            .setView(dialogBinding.root)
            .setPositiveButton(R.string.save) { _, _ ->
                prefs.url = dialogBinding.editUrl.text.toString()
                prefs.username = dialogBinding.editUser.text.toString()
                prefs.password = dialogBinding.editPassword.text.toString()
                prefs.autoConnect = dialogBinding.checkAutoConnect.isChecked
                prefs.captureForAi = dialogBinding.checkCaptureAi.isChecked
                prefs.captureOriginalToo = dialogBinding.checkCaptureOriginal.isChecked
                binding.textUrl.text = prefs.url
                if (controller.isRecording) stopRecording()
                controller.disconnect()
                connect()
            }
            .setNegativeButton(R.string.cancel, null)
            .show()
    }

    /**
     * 같은 WiFi 에서 RTSP 서버가 열린 기기를 찾아 주소 칸을 채운다.
     * 보드가 DHCP 로 IP 를 받으면 재부팅마다 주소가 바뀌기 때문에 필요하다.
     */
    private fun findBoard(dialogBinding: DialogSettingsBinding) {
        if (BoardFinder.localAddress() == null) {
            dialogBinding.textFindStatus.setText(R.string.settings_no_wifi)
            return
        }
        dialogBinding.btnFind.isEnabled = false
        dialogBinding.textFindStatus.text = getString(R.string.settings_finding, 0)
        io.execute {
            val found = BoardFinder.scan(onProgress = { percent ->
                runOnUiThread {
                    dialogBinding.textFindStatus.text =
                        getString(R.string.settings_finding, percent)
                }
            })
            runOnUiThread {
                dialogBinding.btnFind.isEnabled = true
                when {
                    found.isEmpty() ->
                        dialogBinding.textFindStatus.setText(R.string.settings_found_none)
                    found.size == 1 -> {
                        dialogBinding.editUrl.setText(rtspUrlFor(found[0]))
                        dialogBinding.textFindStatus.text =
                            getString(R.string.settings_found_one, found[0])
                    }
                    else -> {
                        dialogBinding.textFindStatus.text =
                            getString(R.string.settings_found_many, found.size)
                        AlertDialog.Builder(this)
                            .setTitle(R.string.settings_find_board)
                            .setItems(found.toTypedArray()) { _, which ->
                                dialogBinding.editUrl.setText(rtspUrlFor(found[which]))
                                dialogBinding.textFindStatus.text =
                                    getString(R.string.settings_found_one, found[which])
                            }
                            .show()
                    }
                }
            }
        }
    }

    /** 기존 주소의 경로를 살리고 호스트만 바꾼다. 경로가 없으면 rkipc 기본값을 쓴다. */
    private fun rtspUrlFor(ip: String): String {
        val path = runCatching { java.net.URI(prefs.url).path }.getOrNull()
            ?.takeIf { it.isNotBlank() } ?: "/live/0"
        return "rtsp://$ip:554$path"
    }

    private fun updateButtons() {
        binding.btnConnect.setText(
            if (controller.isConnected) R.string.disconnect else R.string.connect,
        )
        binding.btnRecord.setText(
            if (controller.isRecording) R.string.record_stop else R.string.record_start,
        )
        binding.btnRecord.isEnabled = controller.isConnected
        binding.btnSnapshot.isEnabled = controller.isConnected
    }

    // -------------------------------------------------- StreamController.Listener

    override fun onStatus(status: StreamController.Status, message: String?) {
        val text = when (status) {
            StreamController.Status.IDLE -> getString(R.string.status_idle)
            StreamController.Status.CONNECTING -> getString(R.string.status_connecting)
            StreamController.Status.PLAYING -> getString(R.string.status_playing)
            StreamController.Status.RECONNECTING ->
                getString(R.string.status_reconnecting, message ?: "")
            StreamController.Status.ERROR -> getString(R.string.status_error, message ?: "")
        }
        binding.textStatus.text = text
        binding.progressConnecting.visibility =
            if (status == StreamController.Status.CONNECTING || status == StreamController.Status.RECONNECTING) {
                View.VISIBLE
            } else {
                View.GONE
            }
        updateButtons()
    }

    override fun onVideoSize(width: Int, height: Int) {
        videoWidth = width
        videoHeight = height
        binding.textureView.setAspectRatio(width, height)
        applyZoom()
        binding.textResolution.text = getString(R.string.resolution_format, width, height)
    }

    override fun onFirstFrame() {
        binding.progressConnecting.visibility = View.GONE
    }

    override fun onRecordingTick(durationMs: Long, waitingForKeyFrame: Boolean) {
        binding.recordingIndicator.visibility = View.VISIBLE
        binding.textRecordTime.text = if (waitingForKeyFrame) {
            getString(R.string.record_preparing)
        } else {
            val totalSec = durationMs / 1000
            String.format(Locale.US, "%02d:%02d", totalSec / 60, totalSec % 60)
        }
    }

    companion object {
        private const val TAG = "MainActivity"
    }
}
