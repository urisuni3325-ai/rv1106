package com.rv1106.camview.gallery

import android.content.Intent
import android.os.Bundle
import android.view.View
import androidx.appcompat.app.AlertDialog
import androidx.appcompat.app.AppCompatActivity
import androidx.core.content.FileProvider
import androidx.recyclerview.widget.GridLayoutManager
import com.rv1106.camview.R
import com.rv1106.camview.databinding.ActivityGalleryBinding
import android.widget.Toast
import java.util.concurrent.Executors

/** 저장된 녹화 영상과 캡처 사진 목록. */
class GalleryActivity : AppCompatActivity() {

    private lateinit var binding: ActivityGalleryBinding
    private lateinit var adapter: MediaAdapter
    private val io = Executors.newSingleThreadExecutor()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityGalleryBinding.inflate(layoutInflater)
        setContentView(binding.root)

        setSupportActionBar(binding.toolbar)
        binding.toolbar.setNavigationOnClickListener { finish() }

        adapter = MediaAdapter(
            onClick = { item -> open(item) },
            onLongClick = { item -> showItemMenu(item) },
        )
        binding.recyclerView.layoutManager = GridLayoutManager(this, 2)
        binding.recyclerView.adapter = adapter
    }

    override fun onResume() {
        super.onResume()
        reload()
    }

    override fun onDestroy() {
        super.onDestroy()
        io.shutdown()
    }

    private fun reload() {
        val items = MediaRepository.listAll(this)
        adapter.submit(items)
        binding.emptyView.visibility = if (items.isEmpty()) View.VISIBLE else View.GONE
    }

    private fun open(item: MediaRepository.MediaItem) {
        val intent = if (item.isVideo) {
            Intent(this, VideoPlayerActivity::class.java)
        } else {
            Intent(this, ImageViewerActivity::class.java)
        }
        intent.putExtra(EXTRA_PATH, item.file.absolutePath)
        startActivity(intent)
    }

    private fun showItemMenu(item: MediaRepository.MediaItem) {
        val actions = arrayOf(
            getString(R.string.action_export),
            getString(R.string.action_share),
            getString(R.string.action_delete),
        )
        AlertDialog.Builder(this)
            .setTitle(item.name)
            .setItems(actions) { _, which ->
                when (which) {
                    0 -> export(item)
                    1 -> share(item)
                    2 -> confirmDelete(item)
                }
            }
            .show()
    }

    private fun export(item: MediaRepository.MediaItem) {
        io.execute {
            val ok = MediaRepository.exportToGallery(this, item)
            runOnUiThread {
                val msg = if (ok) R.string.msg_export_done else R.string.msg_export_failed
                Toast.makeText(this, msg, Toast.LENGTH_SHORT).show()
            }
        }
    }

    private fun share(item: MediaRepository.MediaItem) {
        val uri = FileProvider.getUriForFile(this, "$packageName.fileprovider", item.file)
        val intent = Intent(Intent.ACTION_SEND).apply {
            type = if (item.isVideo) "video/mp4" else "image/jpeg"
            putExtra(Intent.EXTRA_STREAM, uri)
            addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)
        }
        startActivity(Intent.createChooser(intent, getString(R.string.action_share)))
    }

    private fun confirmDelete(item: MediaRepository.MediaItem) {
        AlertDialog.Builder(this)
            .setMessage(getString(R.string.confirm_delete, item.name))
            .setPositiveButton(R.string.action_delete) { _, _ ->
                MediaRepository.delete(item)
                reload()
            }
            .setNegativeButton(R.string.cancel, null)
            .show()
    }

    companion object {
        const val EXTRA_PATH = "path"
    }
}
