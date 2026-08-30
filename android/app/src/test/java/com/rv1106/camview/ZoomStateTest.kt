package com.rv1106.camview

import com.rv1106.camview.ui.ZoomState
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

class ZoomStateTest {

    private val w = 1000
    private val h = 600

    @Test
    fun `기본은 확대되지 않은 상태다`() {
        val z = ZoomState()
        assertEquals(1f, z.scale, 0.001f)
        assertFalse(z.isZoomed)
    }

    @Test
    fun `확대 배율이 곱해진다`() {
        val z = ZoomState()
        z.zoomBy(2f, w / 2f, h / 2f, w, h)
        assertEquals(2f, z.scale, 0.001f)
        assertTrue(z.isZoomed)
    }

    @Test
    fun `최대 배율을 넘지 않는다`() {
        val z = ZoomState()
        repeat(10) { z.zoomBy(2f, w / 2f, h / 2f, w, h) }
        assertEquals(ZoomState.MAX_SCALE, z.scale, 0.001f)
    }

    @Test
    fun `축소해도 1배 아래로 내려가지 않는다`() {
        val z = ZoomState()
        z.zoomBy(0.1f, w / 2f, h / 2f, w, h)
        assertEquals(1f, z.scale, 0.001f)
    }

    @Test
    fun `확대하지 않았으면 이동해도 제자리다`() {
        // 1배에서는 여백이 없으므로 끌어도 움직이면 안 된다.
        val z = ZoomState()
        z.panBy(500f, 500f, w, h)
        assertEquals(0f, z.panX, 0.001f)
        assertEquals(0f, z.panY, 0.001f)
    }

    @Test
    fun `이동은 여백 범위 안으로 제한된다`() {
        val z = ZoomState()
        z.zoomBy(2f, w / 2f, h / 2f, w, h)
        z.panBy(9999f, 9999f, w, h)
        // 2배에서 최대 이동량은 화면의 절반
        assertEquals(w * 0.5f, z.panX, 0.001f)
        assertEquals(h * 0.5f, z.panY, 0.001f)
    }

    @Test
    fun `확대 중심이 화면에 머문다`() {
        // 왼쪽 위 모서리를 잡고 확대하면 그쪽으로 따라가야 한다.
        val z = ZoomState()
        z.zoomBy(2f, 0f, 0f, w, h)
        assertTrue("panX=${z.panX}", z.panX > 0f)
        assertTrue("panY=${z.panY}", z.panY > 0f)
    }

    @Test
    fun `reset 하면 처음 상태로 돌아간다`() {
        val z = ZoomState()
        z.zoomBy(3f, 0f, 0f, w, h)
        z.panBy(50f, 50f, w, h)
        z.reset()
        assertEquals(1f, z.scale, 0.001f)
        assertEquals(0f, z.panX, 0.001f)
        assertEquals(0f, z.panY, 0.001f)
    }

    @Test
    fun `배율 표기가 읽기 쉽다`() {
        val z = ZoomState()
        assertEquals("1.0x", z.label())
        z.zoomBy(2.44f, w / 2f, h / 2f, w, h)
        assertEquals("2.4x", z.label())
    }
}
