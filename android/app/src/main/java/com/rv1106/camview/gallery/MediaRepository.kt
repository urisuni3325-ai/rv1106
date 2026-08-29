package com.rv1106.camview.gallery

import android.content.ContentValues
import android.content.Context
import android.os.Build
import android.os.Environment
import android.provider.MediaStore
import java.io.File
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale

/** 앱 전용 저장소(권한 불필요)에 녹화 영상과 캡처 사진을 관리한다. */
object MediaRepository {

    data class MediaItem(val file: File, val isVideo: Boolean) {
        val dateMs: Long get() = file.lastModified()
        val sizeBytes: Long get() = file.length()
        val name: String get() = file.name
    }

    fun videoDir(context: Context): File =
        File(context.getExternalFilesDir(Environment.DIRECTORY_MOVIES), "recordings").apply { mkdirs() }

    fun photoDir(context: Context): File =
        File(context.getExternalFilesDir(Environment.DIRECTORY_PICTURES), "snapshots").apply { mkdirs() }

    fun newVideoFile(context: Context): File =
        File(videoDir(context), "REC_${timestamp()}.mp4")

    fun newPhotoFile(context: Context): File =
        File(photoDir(context), "IMG_${timestamp()}.jpg")

    fun listAll(context: Context): List<MediaItem> {
        val videos = videoDir(context).listFiles { f -> f.isFile && f.name.endsWith(".mp4") }
            ?.map { MediaItem(it, true) } ?: emptyList()
        val photos = photoDir(context).listFiles { f -> f.isFile && f.name.endsWith(".jpg") }
            ?.map { MediaItem(it, false) } ?: emptyList()
        return (videos + photos).sortedByDescending { it.dateMs }
    }

    fun delete(item: MediaItem): Boolean = item.file.delete()

    /** 시스템 갤러리(사진첩)로 복사해 다른 앱에서도 볼 수 있게 한다. */
    fun exportToGallery(context: Context, item: MediaItem): Boolean {
        return try {
            val resolver = context.contentResolver
            val collection = if (item.isVideo) {
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                    MediaStore.Video.Media.getContentUri(MediaStore.VOLUME_EXTERNAL_PRIMARY)
                } else {
                    MediaStore.Video.Media.EXTERNAL_CONTENT_URI
                }
            } else {
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                    MediaStore.Images.Media.getContentUri(MediaStore.VOLUME_EXTERNAL_PRIMARY)
                } else {
                    MediaStore.Images.Media.EXTERNAL_CONTENT_URI
                }
            }
            val values = ContentValues().apply {
                put(MediaStore.MediaColumns.DISPLAY_NAME, item.name)
                put(MediaStore.MediaColumns.MIME_TYPE, if (item.isVideo) "video/mp4" else "image/jpeg")
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                    val dir = if (item.isVideo) Environment.DIRECTORY_MOVIES else Environment.DIRECTORY_PICTURES
                    put(MediaStore.MediaColumns.RELATIVE_PATH, "$dir/RV1106Cam")
                    put(MediaStore.MediaColumns.IS_PENDING, 1)
                }
            }
            val uri = resolver.insert(collection, values) ?: return false
            resolver.openOutputStream(uri)?.use { out ->
                item.file.inputStream().use { it.copyTo(out) }
            } ?: return false
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                values.clear()
                values.put(MediaStore.MediaColumns.IS_PENDING, 0)
                resolver.update(uri, values, null, null)
            }
            true
        } catch (e: Exception) {
            false
        }
    }

    fun formatSize(bytes: Long): String = when {
        bytes >= 1024L * 1024 * 1024 -> String.format(Locale.US, "%.1f GB", bytes / 1024.0 / 1024 / 1024)
        bytes >= 1024L * 1024 -> String.format(Locale.US, "%.1f MB", bytes / 1024.0 / 1024)
        bytes >= 1024L -> String.format(Locale.US, "%.0f KB", bytes / 1024.0)
        else -> "$bytes B"
    }

    fun formatDate(ms: Long): String =
        SimpleDateFormat("yyyy-MM-dd HH:mm", Locale.getDefault()).format(Date(ms))

    private fun timestamp(): String =
        SimpleDateFormat("yyyyMMdd_HHmmss", Locale.US).format(Date())
}
