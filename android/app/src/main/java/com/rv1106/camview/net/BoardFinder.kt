package com.rv1106.camview.net

import android.util.Log
import java.net.InetSocketAddress
import java.net.Inet4Address
import java.net.NetworkInterface
import java.net.Socket
import java.util.concurrent.Executors
import java.util.concurrent.TimeUnit
import java.util.concurrent.atomic.AtomicInteger

/**
 * 같은 공유기 안에서 RTSP 서버(554 포트)가 열려 있는 기기를 찾는다.
 *
 * 보드가 DHCP 로 주소를 받으면 재부팅할 때마다 IP 가 바뀌는데, 그때마다 보드를
 * PC 에 연결해 확인하는 건 번거롭다. 폰이 붙어 있는 대역을 훑어서 후보를
 * 찾아주면 그 과정을 건너뛸 수 있다.
 */
object BoardFinder {

    private const val TAG = "BoardFinder"
    private const val THREADS = 32
    private const val CONNECT_TIMEOUT_MS = 400
    private const val SCAN_TIMEOUT_SEC = 25L

    /** 폰이 붙어 있는 IPv4 사설 주소. 못 찾으면 null. */
    fun localAddress(): String? {
        try {
            for (nif in NetworkInterface.getNetworkInterfaces()) {
                if (!nif.isUp || nif.isLoopback) continue
                for (addr in nif.inetAddresses) {
                    if (addr !is Inet4Address || addr.isLoopbackAddress) continue
                    val ip = addr.hostAddress ?: continue
                    if (isPrivate(ip)) return ip
                }
            }
        } catch (e: Exception) {
            Log.w(TAG, "주소를 읽지 못했습니다: ${e.message}")
        }
        return null
    }

    /** `172.30.1.39` → `172.30.1.` 처럼 마지막 자리를 뺀 앞부분. */
    fun subnetPrefix(ip: String?): String? {
        if (ip == null) return null
        val parts = ip.split('.')
        if (parts.size != 4) return null
        if (parts.any { it.toIntOrNull() == null }) return null
        return "${parts[0]}.${parts[1]}.${parts[2]}."
    }

    private fun isPrivate(ip: String): Boolean {
        val p = ip.split('.').mapNotNull { it.toIntOrNull() }
        if (p.size != 4) return false
        return when {
            p[0] == 10 -> true
            p[0] == 192 && p[1] == 168 -> true
            p[0] == 172 && p[1] in 16..31 -> true
            else -> false
        }
    }

    /**
     * 현재 대역의 1~254 를 훑어 [port] 가 열린 주소를 모은다.
     *
     * 호출한 스레드를 막으므로 백그라운드에서 부를 것. 진행 상황은 [onProgress]
     * 로 0~100 을 흘려준다.
     */
    fun scan(port: Int = 554, onProgress: (Int) -> Unit = {}): List<String> {
        val prefix = subnetPrefix(localAddress()) ?: run {
            Log.w(TAG, "WiFi 주소를 확인하지 못했습니다")
            return emptyList()
        }
        val self = localAddress()
        val found = java.util.Collections.synchronizedList(ArrayList<String>())
        val done = AtomicInteger(0)
        val pool = Executors.newFixedThreadPool(THREADS)

        for (i in 1..254) {
            val ip = prefix + i
            if (ip == self) {
                onProgress(done.incrementAndGet() * 100 / 254)
                continue
            }
            pool.execute {
                if (isOpen(ip, port)) {
                    Log.i(TAG, "발견: $ip:$port")
                    found.add(ip)
                }
                onProgress(done.incrementAndGet() * 100 / 254)
            }
        }
        pool.shutdown()
        try {
            pool.awaitTermination(SCAN_TIMEOUT_SEC, TimeUnit.SECONDS)
        } catch (e: InterruptedException) {
            pool.shutdownNow()
        }
        return found.sortedBy { ip -> ip.substringAfterLast('.').toIntOrNull() ?: 0 }
    }

    private fun isOpen(ip: String, port: Int): Boolean = try {
        Socket().use { socket ->
            socket.connect(InetSocketAddress(ip, port), CONNECT_TIMEOUT_MS)
            true
        }
    } catch (e: Exception) {
        false
    }
}
