package com.rv1106.camview.capture

/**
 * AI 분석용 캡처 규격.
 *
 * 분석 쪽이 요구하는 형태는 **1:1 / 1000x1000 픽셀 / 72dpi** 다.
 * 스트림은 16:9(2304x1296)라서 그대로 저장하면 비율이 맞지 않으므로
 * 가운데를 정사각형으로 잘라낸 뒤 1000x1000 으로 줄인다.
 *
 * 가운데를 쓰는 이유: 초점(AF ROI)과 링 조명의 중심이 모두 화면 가운데라
 * 가장자리는 어둡고 흐리다. 잘라내도 잃는 정보가 거의 없다.
 *
 * 안드로이드 API 를 쓰지 않는 순수 계산만 두어서 JVM 단위 테스트가 가능하다.
 */
object CaptureFormat {

    /** 저장할 한 변의 픽셀 수. */
    const val SIZE = 1000

    /** JPEG 에 적어 넣을 해상도(dpi). */
    const val DPI = 72

    /** 잘라낼 영역. */
    data class Crop(val x: Int, val y: Int, val size: Int)

    /**
     * [width]x[height] 프레임의 가운데 정사각형.
     * 짧은 변을 한 변으로 삼는다. 잘못된 크기면 null.
     */
    fun centerSquare(width: Int, height: Int): Crop? {
        if (width <= 0 || height <= 0) return null
        val size = minOf(width, height)
        return Crop((width - size) / 2, (height - size) / 2, size)
    }
}
