package com.rv1106.camview.codec

/**
 * RBSP 비트 단위 읽기. H.264 와 H.265 파라미터셋 파싱이 함께 쓴다.
 *
 * 버퍼 끝을 넘어가면 [IndexOutOfBoundsException] 을 던진다. 파서 쪽에서 잡아
 * "해상도를 알 수 없음"으로 처리한다.
 */
internal class BitReader(private val data: ByteArray) {

    private var bitPos = 0

    fun u(bits: Int): Int {
        var value = 0
        for (i in 0 until bits) {
            val byteIndex = bitPos ushr 3
            if (byteIndex >= data.size) throw IndexOutOfBoundsException()
            val bit = (data[byteIndex].toInt() ushr (7 - (bitPos and 7))) and 1
            value = (value shl 1) or bit
            bitPos++
        }
        return value
    }

    /** 값을 쓰지 않는 구간을 건너뛴다(32비트를 넘는 예약 필드용). */
    fun skip(bits: Int) {
        if ((bitPos + bits + 7) / 8 > data.size) throw IndexOutOfBoundsException()
        bitPos += bits
    }

    fun ue(): Int {
        var leadingZeros = 0
        while (u(1) == 0) {
            leadingZeros++
            if (leadingZeros > 31) throw IndexOutOfBoundsException()
        }
        if (leadingZeros == 0) return 0
        return (1 shl leadingZeros) - 1 + u(leadingZeros)
    }

    fun se(): Int {
        val k = ue()
        val sign = if (k and 1 == 1) 1 else -1
        return sign * ((k + 1) / 2)
    }
}

/**
 * emulation prevention byte(0x03)를 제거해 RBSP 로 만든다.
 * [from] 은 NAL 헤더 다음 위치.
 */
internal fun unescapeRbsp(data: ByteArray, from: Int): ByteArray {
    val out = ByteArray(data.size - from)
    var len = 0
    var zeros = 0
    var i = from
    while (i < data.size) {
        val b = data[i]
        if (zeros == 2 && b == 3.toByte()) {
            zeros = 0
        } else {
            out[len++] = b
            zeros = if (b == 0.toByte()) zeros + 1 else 0
        }
        i++
    }
    return out.copyOf(len)
}

/** Annex-B start code(00 00 01 / 00 00 00 01)를 건너뛴 위치를 돌려준다. */
internal fun skipStartCode(nal: ByteArray): Int {
    var offset = 0
    while (offset + 3 < nal.size && nal[offset] == 0.toByte() && nal[offset + 1] == 0.toByte()) {
        offset += if (nal[offset + 2] == 1.toByte()) 3 else 1
    }
    return offset
}
