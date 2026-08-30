package com.rv1106.camview

import com.rv1106.camview.rtsp.RtspClient
import com.rv1106.camview.rtsp.SdpInfo
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNotNull
import org.junit.Assert.assertNull
import org.junit.Test

class SdpInfoTest {

    /** rkipc(Luckfox 기본 이미지)가 내려주는 SDP 와 같은 형태. */
    private val rkipcSdp = """
        v=0
        o=- 91234 1 IN IP4 192.168.0.100
        s=Session streamed by rkipc
        t=0 0
        a=control:*
        m=video 0 RTP/AVP 96
        c=IN IP4 0.0.0.0
        b=AS:4000
        a=rtpmap:96 H264/90000
        a=fmtp:96 packetization-mode=1
        a=control:track0
    """.trimIndent()

    @Test
    fun `비디오 트랙의 control 과 payload type 을 읽는다`() {
        val info = SdpInfo.parse(rkipcSdp)
        assertNotNull(info)
        assertEquals("track0", info!!.control)
        assertEquals(96, info.payloadType)
    }

    @Test
    fun `비디오에 control 이 없으면 세션 control 을 쓴다`() {
        val sdp = """
            v=0
            a=control:rtsp://192.168.0.100/live/0
            m=video 0 RTP/AVP 96
            a=rtpmap:96 H264/90000
        """.trimIndent()
        assertEquals("rtsp://192.168.0.100/live/0", SdpInfo.parse(sdp)?.control)
    }

    @Test
    fun `오디오 트랙의 속성에 영향을 받지 않는다`() {
        val sdp = """
            v=0
            m=audio 0 RTP/AVP 8
            a=control:track1
            m=video 0 RTP/AVP 97
            a=rtpmap:97 H264/90000
            a=control:track0
        """.trimIndent()
        val info = SdpInfo.parse(sdp)
        assertEquals("track0", info?.control)
        assertEquals(97, info?.payloadType)
    }

    @Test
    fun `rtpmap 에서 코덱 이름을 읽는다`() {
        assertEquals("H264", SdpInfo.parse(rkipcSdp)?.encoding)
    }

    @Test
    fun `H265 스트림도 코덱 이름을 그대로 돌려준다`() {
        val sdp = """
            v=0
            m=video 0 RTP/AVP 96
            a=rtpmap:96 H265/90000
            a=control:track0
        """.trimIndent()
        assertEquals("H265", SdpInfo.parse(sdp)?.encoding)
    }

    @Test
    fun `오디오 트랙의 rtpmap 은 비디오 코덱으로 잡히지 않는다`() {
        val sdp = """
            v=0
            m=audio 0 RTP/AVP 8
            a=rtpmap:8 PCMA/8000
            a=control:track1
            m=video 0 RTP/AVP 96
            a=rtpmap:96 H264/90000
            a=control:track0
        """.trimIndent()
        assertEquals("H264", SdpInfo.parse(sdp)?.encoding)
    }

    @Test
    fun `rtpmap 이 없으면 코덱은 null 이다`() {
        val sdp = "v=0\nm=video 0 RTP/AVP 96\na=control:track0"
        assertNull(SdpInfo.parse(sdp)?.encoding)
    }

    @Test
    fun `비디오 트랙이 없으면 null 을 돌려준다`() {
        val sdp = "v=0\nm=audio 0 RTP/AVP 8\na=control:track1"
        assertNull(SdpInfo.parse(sdp))
    }
}

class TransportHeaderTest {

    @Test
    fun `서버가 배정한 인터리브 채널을 읽는다`() {
        assertEquals(
            0,
            RtspClient.parseInterleavedChannel("RTP/AVP/TCP;unicast;interleaved=0-1"),
        )
        assertEquals(
            2,
            RtspClient.parseInterleavedChannel("RTP/AVP/TCP;unicast;interleaved=2-3"),
        )
    }

    @Test
    fun `순서가 달라도 읽는다`() {
        assertEquals(
            4,
            RtspClient.parseInterleavedChannel(
                "RTP/AVP/TCP;interleaved=4-5;unicast;ssrc=1234ABCD",
            ),
        )
    }

    @Test
    fun `interleaved 가 없거나 헤더가 없으면 null 을 돌려준다`() {
        assertNull(RtspClient.parseInterleavedChannel("RTP/AVP;unicast;client_port=8000-8001"))
        assertNull(RtspClient.parseInterleavedChannel(null))
    }
}
