package com.rv1106.camview

import com.rv1106.camview.rtsp.DigestAuth
import java.security.MessageDigest
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNotNull
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Test

class DigestAuthTest {

    @Test
    fun `RFC 2617 예제와 같은 response 를 만든다`() {
        val challenge = "Digest realm=\"testrealm@host.com\", " +
            "nonce=\"dcd98b7102dd2f0e8b11d0f600bfb0c093\", " +
            "opaque=\"5ccc069c403ebaf9f0171e9517f40e41\""

        val header = DigestAuth.buildAuthorization(
            challenge, "GET", "/dir/index.html", "Mufasa", "Circle Of Life",
        )

        assertNotNull(header)
        assertEquals("670fd8c2df070c60b045671b8b24ff02", header!!.param("response"))
        assertEquals("Mufasa", header.param("username"))
        assertEquals("testrealm@host.com", header.param("realm"))
        assertEquals("/dir/index.html", header.param("uri"))
        assertEquals("5ccc069c403ebaf9f0171e9517f40e41", header.param("opaque"))
    }

    @Test
    fun `qop=auth 인 경우 cnonce 와 nc 를 포함해 계산한다`() {
        val challenge = "Digest realm=\"testrealm@host.com\", " +
            "qop=\"auth,auth-int\", nonce=\"dcd98b7102dd2f0e8b11d0f600bfb0c093\""

        val header = DigestAuth.buildAuthorization(
            challenge, "GET", "/dir/index.html", "Mufasa", "Circle Of Life",
        )
        assertNotNull(header)
        val cnonce = header!!.param("cnonce")!!
        assertEquals("00000001", header.param("nc"))
        assertTrue(header.contains("qop=auth,"))

        // cnonce 는 매번 달라지므로, 그 값을 그대로 넣어 기대값을 다시 계산한다.
        val ha1 = md5("Mufasa:testrealm@host.com:Circle Of Life")
        val ha2 = md5("GET:/dir/index.html")
        val expected = md5(
            "$ha1:dcd98b7102dd2f0e8b11d0f600bfb0c093:00000001:$cnonce:auth:$ha2",
        )
        assertEquals(expected, header.param("response"))
    }

    @Test
    fun `RTSP 요청마다 uri 가 바뀌면 response 도 바뀐다`() {
        val challenge = "Digest realm=\"IP Camera\", nonce=\"abc123\""
        val a = DigestAuth.buildAuthorization(challenge, "DESCRIBE", "rtsp://cam/live/0", "admin", "pw")
        val b = DigestAuth.buildAuthorization(challenge, "SETUP", "rtsp://cam/live/0/trackID=0", "admin", "pw")
        assertNotNull(a)
        assertNotNull(b)
        assertTrue(a!!.param("response") != b!!.param("response"))
    }

    @Test
    fun `따옴표 없는 파라미터도 파싱한다`() {
        val challenge = "Digest realm=IPCam, nonce=xyz, stale=FALSE"
        val header = DigestAuth.buildAuthorization(challenge, "OPTIONS", "rtsp://cam/", "u", "p")
        assertNotNull(header)
        assertEquals("IPCam", header!!.param("realm"))
        assertEquals(md5("${md5("u:IPCam:p")}:xyz:${md5("OPTIONS:rtsp://cam/")}"), header.param("response"))
    }

    @Test
    fun `모르는 인증 방식이면 null 을 돌려준다`() {
        assertNull(DigestAuth.buildAuthorization("Bearer realm=\"x\"", "OPTIONS", "rtsp://cam/", "u", "p"))
    }

    @Test
    fun `realm 이나 nonce 가 없으면 null 을 돌려준다`() {
        assertNull(DigestAuth.buildAuthorization("Digest realm=\"x\"", "OPTIONS", "rtsp://cam/", "u", "p"))
    }

    private fun String.param(name: String): String? {
        val quoted = Regex("$name=\"([^\"]*)\"").find(this)
        if (quoted != null) return quoted.groupValues[1]
        return Regex("$name=([^,\\s]+)").find(this)?.groupValues?.get(1)
    }

    private fun md5(text: String): String =
        MessageDigest.getInstance("MD5").digest(text.toByteArray()).joinToString("") {
            "%02x".format(it)
        }
}
