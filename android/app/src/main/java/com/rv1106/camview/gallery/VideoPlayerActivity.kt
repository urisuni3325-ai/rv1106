package com.rv1106.camview.gallery

import android.net.Uri
import android.os.Bundle
import android.widget.MediaController
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import com.rv1106.camview.R
import com.rv1106.camview.databinding.ActivityVideoPlayerBinding
import java.io.File

/** 저장된 MP4 재생. */
class VideoPlayerActivity : AppCompatActivity() {

    private lateinit var binding: ActivityVideoPlayerBinding
    private var position = 0

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityVideoPlayerBinding.inflate(layoutInflater)
        setContentView(binding.root)

        setSupportActionBar(binding.toolbar)
        binding.toolbar.setNavigationOnClickListener { finish() }

        val path = intent.getStringExtra(GalleryActivity.EXTRA_PATH)
        val file = path?.let { File(it) }
        if (file == null || !file.exists()) {
            Toast.makeText(this, R.string.msg_file_missing, Toast.LENGTH_SHORT).show()
            finish()
            return
        }
        title = file.name

        val controls = MediaController(this).apply { setAnchorView(binding.videoView) }
        binding.videoView.setMediaController(controls)
        binding.videoView.setOnPreparedListener { it.isLooping = false }
        binding.videoView.setOnErrorListener { _, what, extra ->
            Toast.makeText(this, getString(R.string.msg_play_failed, what, extra), Toast.LENGTH_LONG).show()
            true
        }
        binding.videoView.setVideoURI(Uri.fromFile(file))
        binding.videoView.start()
    }

    override fun onPause() {
        super.onPause()
        position = binding.videoView.currentPosition
        binding.videoView.pause()
    }

    override fun onResume() {
        super.onResume()
        if (position > 0) binding.videoView.seekTo(position)
    }
}
