package com.rv1106.camview

import android.content.Intent
import android.graphics.Bitmap
import android.graphics.SurfaceTexture
import android.os.Bundle
import android.util.Log
import android.view.Surface
import android.view.TextureView
import android.view.View
import android.view.WindowManager
import android.widget.Toast
import androidx.appcompat.app.AlertDialog
import androidx.appcompat.app.AppCompatActivity
import com.rv1106.camview.databinding.ActivityMainBinding
import com.rv1106.camview.databinding.DialogSettingsBinding
import com.rv1106.camview.gallery.GalleryActivity
import com.rv1106.camview.gallery.MediaRepository
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
        val bitmap: Bitmap? = binding.textureView.bitmap
        if (bitmap == null || controller.status != StreamController.Status.PLAYING) {
            Toast.makeText(this, R.string.msg_snapshot_not_ready, Toast.LENGTH_SHORT).show()
            return
        }
        val file = MediaRepository.newPhotoFile(this)
        io.execute {
            val ok = saveJpeg(bitmap, file)
            runOnUiThread {
                val msg = if (ok) getString(R.string.msg_saved_photo, file.name)
                else getString(R.string.msg_snapshot_failed)
                Toast.makeText(this, msg, Toast.LENGTH_SHORT).show()
            }
        }
        // 촬영 피드백(화면 깜빡임)
        binding.flashOverlay.alpha = 1f
        binding.flashOverlay.animate().alpha(0f).setDuration(220).start()
    }

    private fun saveJpeg(bitmap: Bitmap, file: File): Boolean = try {
        file.parentFile?.mkdirs()
        FileOutputStream(file).use { out ->
            bitmap.compress(Bitmap.CompressFormat.JPEG, 95, out)
        }
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

        AlertDialog.Builder(this)
            .setTitle(R.string.settings_title)
            .setView(dialogBinding.root)
            .setPositiveButton(R.string.save) { _, _ ->
                prefs.url = dialogBinding.editUrl.text.toString()
                prefs.username = dialogBinding.editUser.text.toString()
                prefs.password = dialogBinding.editPassword.text.toString()
                prefs.autoConnect = dialogBinding.checkAutoConnect.isChecked
                binding.textUrl.text = prefs.url
                if (controller.isRecording) stopRecording()
                controller.disconnect()
                connect()
            }
            .setNegativeButton(R.string.cancel, null)
            .show()
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
        binding.textureView.setAspectRatio(width, height)
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
