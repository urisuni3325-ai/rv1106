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

    companion object {
        private const val KEY_URL = "url"
        private const val KEY_USER = "user"
        private const val KEY_PASS = "pass"
        private const val KEY_AUTO = "auto_connect"

        /** Luckfox 공식 이미지의 rkipc 기본 스트림 주소 형식. */
        const val DEFAULT_URL = "rtsp://192.168.0.100:554/live/0"
    }
}
