package com.rv1106.camview.gallery

import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.media.MediaMetadataRetriever
import android.os.Build
import android.os.Handler
import android.os.Looper
import android.util.LruCache
import android.util.Size
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.recyclerview.widget.RecyclerView
import com.rv1106.camview.R
import com.rv1106.camview.databinding.ItemMediaBinding
import java.io.File
import java.util.concurrent.Executors

class MediaAdapter(
    private val onClick: (MediaRepository.MediaItem) -> Unit,
    private val onLongClick: (MediaRepository.MediaItem) -> Unit,
) : RecyclerView.Adapter<MediaAdapter.ViewHolder>() {

    private var items: List<MediaRepository.MediaItem> = emptyList()
    private val io = Executors.newFixedThreadPool(2)
    private val main = Handler(Looper.getMainLooper())
    private val cache = LruCache<String, Bitmap>(64)

    fun submit(newItems: List<MediaRepository.MediaItem>) {
        items = newItems
        notifyDataSetChanged()
    }

    class ViewHolder(val binding: ItemMediaBinding) : RecyclerView.ViewHolder(binding.root)

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): ViewHolder {
        val binding = ItemMediaBinding.inflate(LayoutInflater.from(parent.context), parent, false)
        return ViewHolder(binding)
    }

    override fun getItemCount(): Int = items.size

    override fun onBindViewHolder(holder: ViewHolder, position: Int) {
        val item = items[position]
        val b = holder.binding
        b.textName.text = item.name
        b.textInfo.text = "${MediaRepository.formatDate(item.dateMs)} · ${MediaRepository.formatSize(item.sizeBytes)}"
        b.videoBadge.visibility = if (item.isVideo) View.VISIBLE else View.GONE
        b.root.setOnClickListener { onClick(item) }
        b.root.setOnLongClickListener {
            onLongClick(item)
            true
        }

        val key = item.file.absolutePath + ":" + item.dateMs
        val cached = cache.get(key)
        if (cached != null) {
            b.thumbnail.setImageBitmap(cached)
            return
        }
        b.thumbnail.setImageResource(R.drawable.thumb_placeholder)
        b.thumbnail.tag = key
        io.execute {
            val bitmap = loadThumbnail(item)
            if (bitmap != null) {
                cache.put(key, bitmap)
                main.post {
                    if (b.thumbnail.tag == key) b.thumbnail.setImageBitmap(bitmap)
                }
            }
        }
    }

    private fun loadThumbnail(item: MediaRepository.MediaItem): Bitmap? = try {
        if (item.isVideo) videoThumbnail(item.file) else imageThumbnail(item.file)
    } catch (e: Exception) {
        null
    }

    private fun videoThumbnail(file: File): Bitmap? {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
            return android.media.ThumbnailUtils.createVideoThumbnail(file, Size(480, 270), null)
        }
        val retriever = MediaMetadataRetriever()
        return try {
            retriever.setDataSource(file.absolutePath)
            retriever.frameAtTime
        } finally {
            retriever.release()
        }
    }

    private fun imageThumbnail(file: File): Bitmap? {
        val bounds = BitmapFactory.Options().apply { inJustDecodeBounds = true }
        BitmapFactory.decodeFile(file.absolutePath, bounds)
        var sample = 1
        while (bounds.outWidth / sample > 640) sample *= 2
        val options = BitmapFactory.Options().apply { inSampleSize = sample }
        return BitmapFactory.decodeFile(file.absolutePath, options)
    }
}
