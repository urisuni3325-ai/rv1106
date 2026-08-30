package com.rv1106.camview.rtsp

import android.util.Base64
import android.util.Log
import com.rv1106.camview.codec.H264SpsParser
import com.rv1106.camview.codec.H265SpsParser
import com.rv1106.camview.codec.StreamFormat
import java.io.BufferedInputStream
import java.io.BufferedOutputStream
import java.io.EOFException
import java.io.IOException
import java.io.InputStream
import java.io.OutputStream
import java.net.InetSocketAddress
import java.net.Socket
import java.net.URI
import java.util.Locale
import java.util.concurrent.atomic.AtomicBoolean

/**
 * RTSP(TCP interleaved) 클라이언트.
 *
 * 외부 라이브러리 없이 OPTIONS/DESCRIBE/SETUP/PLAY/TEARDOWN 을 직접 처리하고,
 * RTP 패킷을 H.264 Annex-B 액세스 유닛으로 재조립해서 리스너에 전달한다.
 * UDP 를 쓰지 않고 TCP interleaved 만 쓰기 때문에 공유기/NAT 환경에서 잘 붙는다.
 */
class RtspClient(
    private val url: String,
    private val username: String?,
    private val password: String?,
    private val listener: Listener,
) {

    interface Listener {
        /** 코덱 정보를 확보했을 때(최초 1회, 또는 파라미터가 바뀌었을 때) 호출된다. */
        fun onFormat(format: StreamFormat)

        /** 완성된 액세스 유닛 하나. Annex-B(00 00 00 01 포함) 바이트 배열. */
        fun onAccessUnit(au: ByteArray, isKeyFrame: Boolean, ptsUs: Long)

        fun onState(state: State, message: String?)
    }

    enum class State { CONNECTING, PLAYING, RECONNECTING, STOPPED, ERROR }

    private val running = AtomicBoolean(false)
    private var thread: Thread? = null

    @Volatile private var socket: Socket? = null
    private var input: InputStream? = null
    private var output: OutputStream? = null

    private var cseq = 1
    private var sessionId: String? = null
    private var sessionTimeoutSec = 60
    private var authChallenge: String? = null
    private var contentBase: String? = null

    private var isH265 = false
    private var vps: ByteArray? = null
    private var sps: ByteArray? = null
    private var pps: ByteArray? = null
    private var currentFormat: StreamFormat? = null

    /** 서버가 SETUP 응답에서 지정한 RTP 인터리브 채널. 요청값과 다를 수 있다. */
    private var rtpChannel = 0
    private var rtpPacketCount = 0L
    private var accessUnitCount = 0L
    private var codecSwitched = false
    private var firstPacketAtMs = 0L
    private var loggedUnexpectedChannel = -1

    private val depacketizerCallback = object : RtpDepacketizer.Callback {
        override fun onNalUnits(au: ByteArray, isKeyFrame: Boolean, rtpTimestamp: Long) {
            handleAccessUnit(au, isKeyFrame, rtpTimestamp)
        }
    }

    /** 코덱을 SDP 에서 확인한 뒤 세션마다 새로 만든다. */
    private var depacketizer: RtpDepacketizer? = null

    // RTP 타임스탬프(90kHz, 32bit) 랩어라운드 보정용
    private var firstRtpTs = -1L
    private var lastRtpTs = 0L
    private var tsWrapOffset = 0L

    fun start() {
        if (running.getAndSet(true)) return
        thread = Thread({ runLoop() }, "rtsp-client").apply {
            isDaemon = true
            start()
        }
    }

    fun stop() {
        if (!running.getAndSet(false)) return
        closeSocket()
        thread?.interrupt()
        thread = null
    }

    val isRunning: Boolean get() = running.get()

    private fun runLoop() {
        var backoffMs = 1000L
        while (running.get()) {
            try {
                listener.onState(State.CONNECTING, null)
                session()
                backoffMs = 1000L
            } catch (e: Exception) {
                if (!running.get()) break
                Log.w(TAG, "session ended: ${e.message}")
                listener.onState(State.RECONNECTING, e.message ?: e.javaClass.simpleName)
                try {
                    Thread.sleep(backoffMs)
                } catch (ie: InterruptedException) {
                    break
                }
                backoffMs = (backoffMs * 2).coerceAtMost(8000L)
            } finally {
                closeSocket()
            }
        }
        listener.onState(State.STOPPED, null)
    }

    // ---------------------------------------------------------------- session

    private fun session() {
        resetSessionState()

        val uri = URI(url)
        val host = uri.host ?: throw IOException("주소에서 호스트를 찾을 수 없습니다: $url")
        val port = if (uri.port > 0) uri.port else DEFAULT_PORT

        val sock = Socket()
        sock.tcpNoDelay = true
        sock.connect(InetSocketAddress(host, port), CONNECT_TIMEOUT_MS)
        sock.soTimeout = READ_TIMEOUT_MS
        socket = sock
        input = BufferedInputStream(sock.getInputStream(), 64 * 1024)
        output = BufferedOutputStream(sock.getOutputStream(), 8 * 1024)

        request("OPTIONS", url)

        val describe = request("DESCRIBE", url, listOf("Accept: application/sdp"))
        if (describe.status != 200) throw IOException("DESCRIBE 실패 (${describe.status})")
        contentBase = describe.headers["content-base"] ?: describe.headers["content-location"] ?: url

        Log.i(TAG, "SDP 원문:\n${describe.body.take(800)}")
        val sdp = SdpInfo.parse(describe.body)
            ?: throw IOException("SDP 에서 비디오 트랙을 찾지 못했습니다")
        Log.i(
            TAG,
            "SDP: control=${sdp.control} payloadType=${sdp.payloadType} " +
                "encoding=${sdp.encoding} vps=${sdp.vps?.size ?: 0}바이트 " +
                "sps=${sdp.sps?.size ?: 0}바이트 pps=${sdp.pps?.size ?: 0}바이트",
        )

        // 코덱에 맞는 재조립기를 고른다. 모르는 코덱이면 화면이 검은 채로 멈추므로
        // 여기서 이유를 남기고 끊는다.
        val encoding = sdp.encoding?.uppercase(Locale.US)
        isH265 = when (encoding) {
            "H264", "AVC" -> false
            "H265", "HEVC" -> true
            null -> sdp.vps != null // rtpmap 이 없으면 sprop-vps 유무로 짐작한다
            else -> throw IOException("지원하지 않는 코덱입니다: $encoding (H.264 와 H.265 만 처리합니다)")
        }
        if (encoding == null) {
            Log.w(TAG, "SDP 에 rtpmap 이 없습니다. ${if (isH265) "H.265" else "H.264"} 로 시작합니다")
        }
        createDepacketizer()

        vps = sdp.vps
        sps = sdp.sps
        pps = sdp.pps
        buildFormatIfReady()

        val controlUrl = resolveControl(sdp.control, contentBase ?: url)
        val setup = request(
            "SETUP", controlUrl,
            listOf("Transport: RTP/AVP/TCP;unicast;interleaved=0-1"),
        )
        if (setup.status != 200) throw IOException("SETUP 실패 (${setup.status})")
        parseSessionHeader(setup.headers["session"])
        rtpChannel = parseInterleavedChannel(setup.headers["transport"]) ?: 0
        Log.i(TAG, "SETUP 완료: session=$sessionId rtpChannel=$rtpChannel transport=${setup.headers["transport"]}")

        val play = request("PLAY", contentBase ?: url, listOf("Range: npt=0.000-"))
        if (play.status != 200) throw IOException("PLAY 실패 (${play.status})")

        listener.onState(State.PLAYING, null)
        readInterleaved()
    }

    private fun createDepacketizer() {
        depacketizer = if (isH265) {
            RtpH265Depacketizer(depacketizerCallback)
        } else {
            RtpH264Depacketizer(depacketizerCallback)
        }
        Log.i(TAG, "코덱: ${if (isH265) "H.265" else "H.264"}")
    }

    /**
     * 패킷은 들어오는데 액세스 유닛이 하나도 안 나오면 코덱 판단이 틀린 것이다.
     * SDP 가 코덱을 알려주지 않는 서버가 있어서, 한 번은 반대쪽으로 바꿔 본다.
     */
    /**
     * 코덱 판단이 틀렸는지 확인한다. 해석 못 한 페이로드가 쌓이거나, 한참 받고도
     * 액세스 유닛이 하나도 안 나오면 반대쪽 코덱으로 한 번 바꿔 본다.
     *
     * GOP 가 길면 첫 키프레임까지 몇 초가 걸리므로 시간 조건은 넉넉히 잡는다.
     */
    private fun maybeSwitchCodec() {
        if (codecSwitched) return
        val malformed = depacketizer?.malformedCount ?: 0
        val elapsed = if (firstPacketAtMs == 0L) 0L else System.currentTimeMillis() - firstPacketAtMs
        if (shouldSwitchCodec(malformed, elapsed, accessUnitCount)) {
            Log.w(TAG, "코덱 판단 재검토: 해석 실패 $malformed 회, ${elapsed}ms 동안 액세스 유닛 없음")
            switchCodec()
        }
    }

    private fun switchCodec() {
        codecSwitched = true
        isH265 = !isH265
        Log.w(TAG, "데이터가 풀리지 않아 코덱을 ${if (isH265) "H.265" else "H.264"} 로 바꿉니다")
        createDepacketizer()
        // 앞 코덱 기준으로 모아둔 파라미터셋은 버리고 스트림에서 다시 받는다.
        vps = null
        sps = null
        pps = null
        currentFormat = null
        rtpPacketCount = 0
        firstPacketAtMs = System.currentTimeMillis()
    }

    private fun resetSessionState() {
        cseq = 1
        sessionId = null
        contentBase = null
        isH265 = false
        vps = null
        sps = null
        pps = null
        currentFormat = null
        depacketizer = null
        codecSwitched = false
        firstPacketAtMs = 0L
        rtpChannel = 0
        rtpPacketCount = 0
        accessUnitCount = 0
        loggedUnexpectedChannel = -1
        firstRtpTs = -1L
        lastRtpTs = 0L
        tsWrapOffset = 0L
    }

    private fun parseSessionHeader(value: String?) {
        val v = value ?: return
        val parts = v.split(';')
        sessionId = parts[0].trim()
        for (p in parts.drop(1)) {
            val t = p.trim()
            if (t.startsWith("timeout=", ignoreCase = true)) {
                sessionTimeoutSec = t.substring(8).trim().toIntOrNull() ?: 60
            }
        }
    }

    private fun resolveControl(control: String?, base: String): String {
        if (control.isNullOrEmpty() || control == "*") return base
        if (control.startsWith("rtsp://", ignoreCase = true)) return control
        return if (base.endsWith("/")) base + control else "$base/$control"
    }

    // ---------------------------------------------------------------- 요청/응답

    private class Response(
        val status: Int,
        val headers: Map<String, String>,
        val body: String,
    )

    private fun request(
        method: String,
        uri: String,
        extraHeaders: List<String> = emptyList(),
    ): Response {
        var response = sendRequest(method, uri, extraHeaders)
        if (response.status == 401 && username != null) {
            val challenge = response.headers["www-authenticate"]
            if (challenge != null) {
                authChallenge = challenge
                response = sendRequest(method, uri, extraHeaders)
            }
        }
        if (response.status == 401) throw IOException("인증 실패 — 아이디/비밀번호를 확인하세요")
        return response
    }

    private fun sendRequest(
        method: String,
        uri: String,
        extraHeaders: List<String>,
    ): Response {
        val out = output ?: throw IOException("연결이 끊어졌습니다")
        val sb = StringBuilder()
        sb.append(method).append(' ').append(uri).append(" RTSP/1.0\r\n")
        sb.append("CSeq: ").append(cseq++).append("\r\n")
        sb.append("User-Agent: ").append(USER_AGENT).append("\r\n")
        sessionId?.let { sb.append("Session: ").append(it).append("\r\n") }
        // Authorization 은 요청 URI 마다 다시 계산해야 한다(digest 의 uri 필드가 바뀌므로).
        authorizationFor(method, uri)?.let {
            sb.append("Authorization: ").append(it).append("\r\n")
        }
        for (h in extraHeaders) sb.append(h).append("\r\n")
        sb.append("\r\n")
        out.write(sb.toString().toByteArray(Charsets.UTF_8))
        out.flush()
        return readResponse(firstLine = null)
    }

    private fun authorizationFor(method: String, uri: String): String? {
        val challenge = authChallenge ?: return null
        val user = username ?: return null
        return DigestAuth.buildAuthorization(challenge, method, uri, user, password ?: "")
    }

    /** RTSP 응답 파싱. [firstLine] 이 주어지면 이미 읽어둔 상태줄로 취급한다. */
    private fun readResponse(firstLine: String?): Response {
        val ins = input ?: throw IOException("연결이 끊어졌습니다")
        val status = firstLine ?: readLine(ins)
        val code = status.split(' ').getOrNull(1)?.toIntOrNull() ?: 0
        val headers = HashMap<String, String>()
        while (true) {
            val line = readLine(ins)
            if (line.isEmpty()) break
            val idx = line.indexOf(':')
            if (idx > 0) {
                headers[line.substring(0, idx).trim().lowercase(Locale.US)] =
                    line.substring(idx + 1).trim()
            }
        }
        val length = headers["content-length"]?.toIntOrNull() ?: 0
        var body = ""
        if (length > 0) {
            val buf = ByteArray(length)
            readFully(ins, buf, 0, length)
            body = String(buf, Charsets.UTF_8)
        }
        return Response(code, headers, body)
    }

    // ---------------------------------------------------------------- 데이터 수신

    private fun readInterleaved() {
        val ins = input ?: throw IOException("연결이 끊어졌습니다")
        var lastKeepalive = System.currentTimeMillis()
        val keepaliveIntervalMs = (sessionTimeoutSec.coerceAtLeast(20) / 2) * 1000L
        val header = ByteArray(3)
        var packet = ByteArray(4096)

        while (running.get()) {
            val first = ins.read()
            if (first == -1) throw EOFException("스트림이 종료되었습니다")

            if (first == DOLLAR) {
                readFully(ins, header, 0, 3)
                val channel = header[0].toInt() and 0xFF
                val len = ((header[1].toInt() and 0xFF) shl 8) or (header[2].toInt() and 0xFF)
                if (len > packet.size) packet = ByteArray(len)
                readFully(ins, packet, 0, len)
                if (channel == rtpChannel) {
                    if (rtpPacketCount == 0L) Log.i(TAG, "첫 RTP 패킷 수신 (채널 $channel, $len 바이트)")
                    if (firstPacketAtMs == 0L) firstPacketAtMs = System.currentTimeMillis()
                    rtpPacketCount++
                    depacketizer?.process(packet, len)
                    maybeSwitchCodec()
                } else if (channel != rtpChannel + 1 && channel != loggedUnexpectedChannel) {
                    // RTCP(rtpChannel+1)는 정상이므로 조용히 버린다.
                    loggedUnexpectedChannel = channel
                    Log.w(TAG, "예상치 못한 인터리브 채널 $channel (RTP 채널은 $rtpChannel)")
                }
            } else if (first == 'R'.code) {
                // 데이터 사이에 섞여 들어온 RTSP 응답(주로 keepalive 응답)
                readResponse("R" + readLine(ins))
            }
            // 그 밖의 바이트는 동기화가 깨진 것이므로 '$' 가 나올 때까지 흘려보낸다.

            val now = System.currentTimeMillis()
            if (now - lastKeepalive > keepaliveIntervalMs) {
                lastKeepalive = now
                sendKeepalive()
            }
        }
    }

    private fun sendKeepalive() {
        val out = output ?: return
        val sb = StringBuilder()
        sb.append("OPTIONS ").append(url).append(" RTSP/1.0\r\n")
        sb.append("CSeq: ").append(cseq++).append("\r\n")
        sb.append("User-Agent: ").append(USER_AGENT).append("\r\n")
        sessionId?.let { sb.append("Session: ").append(it).append("\r\n") }
        authorizationFor("OPTIONS", url)?.let { sb.append("Authorization: ").append(it).append("\r\n") }
        sb.append("\r\n")
        out.write(sb.toString().toByteArray(Charsets.UTF_8))
        out.flush()
    }

    private fun handleAccessUnit(au: ByteArray, isKeyFrame: Boolean, rtpTimestamp: Long) {
        // 스트림 안에 SPS/PPS 가 들어오면(SDP 에 없던 경우 포함) 갱신한다.
        extractParameterSets(au)

        if (firstRtpTs < 0) {
            firstRtpTs = rtpTimestamp
            lastRtpTs = rtpTimestamp
        }
        if (rtpTimestamp < lastRtpTs && lastRtpTs - rtpTimestamp > 0x80000000L) {
            tsWrapOffset += 0x100000000L
        }
        lastRtpTs = rtpTimestamp
        val ticks = rtpTimestamp + tsWrapOffset - firstRtpTs
        val ptsUs = ticks * 1_000_000L / CLOCK_RATE
        accessUnitCount++
        if (accessUnitCount == 1L || accessUnitCount % 300L == 0L) {
            Log.i(TAG, "액세스 유닛 $accessUnitCount 개 (${au.size}바이트, 키프레임=$isKeyFrame, RTP 패킷 $rtpPacketCount)")
        }
        listener.onAccessUnit(au, isKeyFrame, ptsUs)
    }

    /** 스트림 안에 들어오는 파라미터셋을 코덱에 맞게 뽑아 둔다. */
    private fun extractParameterSets(au: ByteArray) {
        var changed = false
        forEachNal(au) { start, end ->
            val nal = { au.copyOfRange(start, end) }
            if (isH265) {
                when ((au[start].toInt() ushr 1) and 0x3F) {
                    32 -> if (!nal().contentEquals(vps)) { vps = nal(); changed = true }
                    33 -> if (!nal().contentEquals(sps)) { sps = nal(); changed = true }
                    34 -> if (!nal().contentEquals(pps)) { pps = nal(); changed = true }
                }
            } else {
                when (au[start].toInt() and 0x1F) {
                    7 -> if (!nal().contentEquals(sps)) { sps = nal(); changed = true }
                    8 -> if (!nal().contentEquals(pps)) { pps = nal(); changed = true }
                }
            }
        }
        if (changed) buildFormatIfReady()
    }

    /** 필요한 파라미터셋이 모두 모였으면 StreamFormat 을 만들어 알린다. */
    private fun buildFormatIfReady() {
        val s = sps ?: return
        val p = pps ?: return
        val format = if (isH265) {
            val v = vps ?: return
            val size = H265SpsParser.parseSize(s)
            StreamFormat.h265(v, s, p, size?.first ?: 0, size?.second ?: 0)
        } else {
            val size = H264SpsParser.parseSize(s)
            StreamFormat.h264(s, p, size?.first ?: 0, size?.second ?: 0)
        }
        if (format.sameAs(currentFormat)) return
        currentFormat = format
        Log.i(TAG, "코덱 정보 확보: ${format.codecName} ${format.width}x${format.height}")
        listener.onFormat(format)
    }

    private fun closeSocket() {
        try {
            output?.flush()
        } catch (_: IOException) {
        }
        try {
            socket?.close()
        } catch (_: IOException) {
        }
        socket = null
        input = null
        output = null
    }

    companion object {
        private const val TAG = "RtspClient"
        private const val DEFAULT_PORT = 554
        private const val CONNECT_TIMEOUT_MS = 5000
        private const val READ_TIMEOUT_MS = 15000
        private const val USER_AGENT = "RV1106CamView"
        private const val DOLLAR = '$'.code
        private const val CLOCK_RATE = 90_000L

        /** 해석 못 한 페이로드가 이만큼 쌓이면 코덱 판단이 틀린 것으로 본다. */
        private const val MALFORMED_TO_SWITCH = 2

        /** 해석 실패가 없더라도 이 시간 동안 화면 한 장 못 만들면 코덱을 바꿔 본다. */
        private const val NO_FRAME_TIMEOUT_MS = 12_000L

        /**
         * 코덱 판단을 뒤집어야 하는지. 시간 조건이 잘못 계산되면 정상 스트림도
         * 첫 패킷에서 바로 뒤집히므로 판단만 떼어 내 테스트한다.
         */
        fun shouldSwitchCodec(malformedCount: Int, elapsedMs: Long, accessUnitCount: Long): Boolean {
            if (accessUnitCount > 0L) return false
            return malformedCount >= MALFORMED_TO_SWITCH || elapsedMs > NO_FRAME_TIMEOUT_MS
        }

        fun readLine(ins: InputStream): String {
            val sb = StringBuilder()
            while (true) {
                val b = ins.read()
                if (b == -1) throw EOFException("스트림이 종료되었습니다")
                if (b == '\n'.code) break
                if (b != '\r'.code) sb.append(b.toChar())
            }
            return sb.toString()
        }

        fun readFully(ins: InputStream, buf: ByteArray, offset: Int, length: Int) {
            var read = 0
            while (read < length) {
                val n = ins.read(buf, offset + read, length - read)
                if (n == -1) throw EOFException("스트림이 종료되었습니다")
                read += n
            }
        }

        /** Annex-B 버퍼를 훑으면서 각 NAL 의 [payload 시작, 끝) 을 콜백한다. */
        inline fun forEachNal(data: ByteArray, action: (start: Int, end: Int) -> Unit) {
            var i = 0
            var nalStart = -1
            while (i + 3 < data.size + 1) {
                if (i + 2 < data.size && data[i] == 0.toByte() && data[i + 1] == 0.toByte() &&
                    data[i + 2] == 1.toByte()
                ) {
                    if (nalStart >= 0) {
                        var end = i
                        if (end > nalStart && data[end - 1] == 0.toByte()) end--
                        action(nalStart, end)
                    }
                    nalStart = i + 3
                    i += 3
                } else {
                    i++
                }
            }
            if (nalStart in 0 until data.size) action(nalStart, data.size)
        }

        fun base64Decode(s: String): ByteArray = Base64.decode(s, Base64.DEFAULT)

        /**
         * `Transport: RTP/AVP/TCP;unicast;interleaved=2-3` 에서 RTP 채널(앞 숫자)을 뽑는다.
         * 서버가 요청과 다른 채널을 배정할 수 있으므로 응답을 반드시 확인해야 한다.
         */
        fun parseInterleavedChannel(transport: String?): Int? {
            val t = transport ?: return null
            val idx = t.indexOf("interleaved=", ignoreCase = true)
            if (idx < 0) return null
            val digits = t.substring(idx + "interleaved=".length).takeWhile { it.isDigit() }
            return digits.toIntOrNull()
        }
    }
}
