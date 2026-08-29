package com.rv1106.camview.ui

import android.content.Context
import android.util.AttributeSet
import android.view.TextureView

/** 영상 비율을 유지하면서 부모 안에 꽉 차게(letterbox) 배치되는 TextureView. */
class AspectRatioTextureView @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null,
    defStyleAttr: Int = 0,
) : TextureView(context, attrs, defStyleAttr) {

    private var aspectWidth = 16
    private var aspectHeight = 9

    fun setAspectRatio(width: Int, height: Int) {
        if (width <= 0 || height <= 0) return
        if (aspectWidth == width && aspectHeight == height) return
        aspectWidth = width
        aspectHeight = height
        requestLayout()
    }

    override fun onMeasure(widthMeasureSpec: Int, heightMeasureSpec: Int) {
        super.onMeasure(widthMeasureSpec, heightMeasureSpec)
        val width = MeasureSpec.getSize(widthMeasureSpec)
        val height = MeasureSpec.getSize(heightMeasureSpec)
        if (width == 0 || height == 0) {
            setMeasuredDimension(width, height)
            return
        }
        if (width * aspectHeight > height * aspectWidth) {
            setMeasuredDimension(height * aspectWidth / aspectHeight, height)
        } else {
            setMeasuredDimension(width, width * aspectHeight / aspectWidth)
        }
    }
}
