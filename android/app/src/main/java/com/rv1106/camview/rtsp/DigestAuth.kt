package com.rv1106.camview.rtsp

import android.util.Base64
import java.security.MessageDigest
import java.util.Locale

/** RTSP 401 응답의 WWW-Authenticate 를 보고 Basic / Digest(MD5) 헤더를 만든다. */
object DigestAuth {

    fun buildAuthorization(
        challenge: String,
        method: String,
        uri: String,
        username: String,
        password: String,
    ): String? {
        val scheme = challenge.trim().substringBefore(' ').lowercase(Locale.US)
        return when (scheme) {
            "basic" -> basic(username, password)
            "digest" -> digest(challenge, method, uri, username, password)
            else -> null
        }
    }

    private fun basic(username: String, password: String): String {
        val token = Base64.encodeToString(
            "$username:$password".toByteArray(Charsets.UTF_8), Base64.NO_WRAP,
        )
        return "Basic $token"
    }

    private fun digest(
        challenge: String,
        method: String,
        uri: String,
        username: String,
        password: String,
    ): String? {
        val params = parseParams(challenge)
        val realm = params["realm"] ?: return null
        val nonce = params["nonce"] ?: return null
        val qop = params["qop"]?.split(',')?.map { it.trim() }?.firstOrNull { it == "auth" }
        val opaque = params["opaque"]

        val ha1 = md5("$username:$realm:$password")
        val ha2 = md5("$method:$uri")

        val sb = StringBuilder("Digest ")
        sb.append("username=\"").append(username).append("\", ")
        sb.append("realm=\"").append(realm).append("\", ")
        sb.append("nonce=\"").append(nonce).append("\", ")
        sb.append("uri=\"").append(uri).append("\", ")

        val response: String
        if (qop != null) {
            val nc = "00000001"
            val cnonce = md5(System.nanoTime().toString()).substring(0, 16)
            response = md5("$ha1:$nonce:$nc:$cnonce:$qop:$ha2")
            sb.append("qop=").append(qop).append(", ")
            sb.append("nc=").append(nc).append(", ")
            sb.append("cnonce=\"").append(cnonce).append("\", ")
        } else {
            response = md5("$ha1:$nonce:$ha2")
        }
        sb.append("response=\"").append(response).append("\"")
        opaque?.let { sb.append(", opaque=\"").append(it).append("\"") }
        return sb.toString()
    }

    /** `key="value"` / `key=value` 를 콤마 단위로 파싱한다(따옴표 안의 콤마는 무시). */
    private fun parseParams(challenge: String): Map<String, String> {
        val body = challenge.trim().substringAfter(' ', "")
        val result = HashMap<String, String>()
        var i = 0
        val n = body.length
        while (i < n) {
            while (i < n && (body[i] == ',' || body[i] == ' ')) i++
            val eq = body.indexOf('=', i)
            if (eq < 0) break
            val key = body.substring(i, eq).trim().lowercase(Locale.US)
            var p = eq + 1
            val value: String
            if (p < n && body[p] == '"') {
                p++
                val close = body.indexOf('"', p)
                if (close < 0) break
                value = body.substring(p, close)
                p = close + 1
            } else {
                var end = body.indexOf(',', p)
                if (end < 0) end = n
                value = body.substring(p, end).trim()
                p = end
            }
            result[key] = value
            i = p
        }
        return result
    }

    private fun md5(text: String): String {
        val digest = MessageDigest.getInstance("MD5").digest(text.toByteArray(Charsets.UTF_8))
        val sb = StringBuilder(digest.size * 2)
        for (b in digest) sb.append(String.format("%02x", b))
        return sb.toString()
    }
}
