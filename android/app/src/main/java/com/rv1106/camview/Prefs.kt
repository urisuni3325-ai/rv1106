package com.rv1106.camview

import android.content.Context

/** 접속 정보 저장. */
class Prefs(context: Context) {

    private val sp = context.getSharedPreferences("camview", Context.MODE_PRIVATE)

    var url: String
        get() = sp.getString(KEY_URL, DEFAULT_URL) ?: DEFAULT_URL
        set(value) = sp.edit().putString(KEY_URL, value.trim()).apply()

    var username: String
        get() = sp.getString(KEY_USER, "") ?: ""
        set(value) = sp.edit().putString(KEY_USER, value.trim()).apply()

    var password: String
        get() = sp.getString(KEY_PASS, "") ?: ""
        set(value) = sp.edit().putString(KEY_PASS, value).apply()

    var autoConnect: Boolean
        get() = sp.getBoolean(KEY_AUTO, true)
        set(value) = sp.edit().putBoolean(KEY_AUTO, value).apply()

    /** 캡처를 AI 분석 규격(1:1 / 1000x1000 / 72dpi)으로 저장한다. */
    var captureForAi: Boolean
        get() = sp.getBoolean(KEY_CAPTURE_AI, true)
        set(value) = sp.edit().putBoolean(KEY_CAPTURE_AI, value).apply()

    /** AI 규격과 별개로 스트림 원본 해상도 사진도 함께 남긴다. */
    var captureOriginalToo: Boolean
        get() = sp.getBoolean(KEY_CAPTURE_ORIGINAL, false)
        set(value) = sp.edit().putBoolean(KEY_CAPTURE_ORIGINAL, value).apply()

    companion object {
        private const val KEY_URL = "url"
        private const val KEY_USER = "user"
        private const val KEY_PASS = "pass"
        private const val KEY_AUTO = "auto_connect"
        private const val KEY_CAPTURE_AI = "capture_ai"
        private const val KEY_CAPTURE_ORIGINAL = "capture_original"

        /** Luckfox 공식 이미지의 rkipc 기본 스트림 주소 형식. */
        const val DEFAULT_URL = "rtsp://192.168.0.100:554/live/0"
    }
}
