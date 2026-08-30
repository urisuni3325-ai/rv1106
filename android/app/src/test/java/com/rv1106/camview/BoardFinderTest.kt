package com.rv1106.camview

import com.rv1106.camview.net.BoardFinder
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Test

class BoardFinderTest {

    @Test
    fun `주소에서 대역 앞부분을 뽑는다`() {
        assertEquals("172.30.1.", BoardFinder.subnetPrefix("172.30.1.39"))
        assertEquals("192.168.0.", BoardFinder.subnetPrefix("192.168.0.100"))
        assertEquals("10.0.0.", BoardFinder.subnetPrefix("10.0.0.2"))
    }

    @Test
    fun `자리 수가 안 맞는 주소는 null 을 돌려준다`() {
        assertNull(BoardFinder.subnetPrefix("172.30.1"))
        assertNull(BoardFinder.subnetPrefix("172.30.1.39.5"))
    }

    @Test
    fun `숫자가 아닌 주소는 null 을 돌려준다`() {
        assertNull(BoardFinder.subnetPrefix("fe80::1"))
        assertNull(BoardFinder.subnetPrefix("board.local"))
        assertNull(BoardFinder.subnetPrefix(null))
    }
}
