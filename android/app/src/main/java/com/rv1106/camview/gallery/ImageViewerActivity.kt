package com.rv1106.camview.gallery

import android.graphics.BitmapFactory
import android.os.Bundle
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import com.rv1106.camview.R
import com.rv1106.camview.databinding.ActivityImageViewerBinding
import java.io.File

/** 저장된 캡처 사진 보기. */
class ImageViewerActivity : AppCompatActivity() {

    private lateinit var binding: ActivityImageViewerBinding

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityImageViewerBinding.inflate(layoutInflater)
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
        val bitmap = BitmapFactory.decodeFile(file.absolutePath)
        if (bitmap == null) {
            Toast.makeText(this, R.string.msg_file_missing, Toast.LENGTH_SHORT).show()
            finish()
            return
        }
        binding.imageView.setImageBitmap(bitmap)
    }
}
