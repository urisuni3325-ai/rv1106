package com.rv1106.camview.ui

import android.graphics.Matrix

/**
 * 화면 확대·이동 상태.
 *
 * 두피처럼 가까이 들여다봐야 하는 대상은 화면에서 한 번 더 키워 보는 일이 잦다.
 * 변환 행렬 계산과 경계 제한만 떼어 두어 화면 없이도 검증할 수 있게 했다.
 */
class ZoomState {

    var scale = 1f
        private set
    var panX = 0f
        private set
    var panY = 0f
        private set

    val isZoomed: Boolean get() = scale > 1.001f

    fun reset() {
        scale = 1f
        panX = 0f
        panY = 0f
    }

    /**
     * [focusX], [focusY] 를 중심으로 [factor] 배 확대한다.
     * 확대 중심이 화면에 그대로 머물도록 이동량을 함께 조정한다.
     */
    fun zoomBy(factor: Float, focusX: Float, focusY: Float, viewWidth: Int, viewHeight: Int) {
        val newScale = (scale * factor).coerceIn(MIN_SCALE, MAX_SCALE)
        val applied = newScale / scale
        // 확대 중심에서 본 상대 위치가 유지되도록 이동량을 보정한다.
        panX = (panX - focusX + viewWidth / 2f) * applied + focusX - viewWidth / 2f
        panY = (panY - focusY + viewHeight / 2f) * applied + focusY - viewHeight / 2f
        scale = newScale
        clamp(viewWidth, viewHeight)
    }

    fun panBy(dx: Float, dy: Float, viewWidth: Int, viewHeight: Int) {
        panX += dx
        panY += dy
        clamp(viewWidth, viewHeight)
    }

    /** 확대해도 화면 밖의 빈 공간이 보이지 않도록 이동량을 제한한다. */
    private fun clamp(viewWidth: Int, viewHeight: Int) {
        val maxX = viewWidth * (scale - 1f) / 2f
        val maxY = viewHeight * (scale - 1f) / 2f
        panX = panX.coerceIn(-maxX, maxX)
        panY = panY.coerceIn(-maxY, maxY)
    }

    /** TextureView.setTransform 에 넘길 행렬. */
    fun toMatrix(viewWidth: Int, viewHeight: Int): Matrix = Matrix().apply {
        setScale(scale, scale, viewWidth / 2f, viewHeight / 2f)
        postTranslate(panX, panY)
    }

    /** 사용자에게 보여줄 배율 표기. 예: `2.4x` */
    fun label(): String = String.format(java.util.Locale.US, "%.1fx", scale)

    companion object {
        const val MIN_SCALE = 1f
        const val MAX_SCALE = 8f
    }
}
