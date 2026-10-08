// 웹 뷰어(main/web/index.html) 브라우저 테스트 — 보드 없이
//   node web_e2e.mjs ../main/web/index.html build/fixed_1280x720.jpg
// 보드 대역: 두 번째 탭이 canvas 영상을 sendonly offer 로 내보내고(보드처럼 후보를 SDP 에 다 넣어서),
// mock HTTP 서버가 /api/* 를 흉내낸다. 확인하는 것:
//   offer 받기 → answer 보내기 → 영상 재생, 스냅샷 2종 + 저장, 끊기(hangup) → 재연결
import http from "node:http";
import fs from "node:fs";
import os from "node:os";
import path from "node:path";
import { createRequire } from "node:module";

// 전역 설치된 playwright 도 찾도록 (npm i -g playwright 또는 npx playwright)
const require = createRequire(import.meta.url);
const { chromium } = require(process.env.PLAYWRIGHT_MODULE || "playwright");
const OUT = fs.mkdtempSync(path.join(os.tmpdir(), "p4cam-e2e-"));

const html = fs.readFileSync(process.argv[2]);
const jpeg = fs.readFileSync(process.argv[3]);
let boardPage, hangups = 0, answers = 0, session = 0, staleRejected = 0;

const server = http.createServer(async (req, res) => {
  const body = await new Promise((r) => { let b = ""; req.on("data", (c) => (b += c)); req.on("end", () => r(b)); });
  if (req.url === "/") { res.setHeader("Content-Type", "text/html; charset=utf-8"); return res.end(html); }
  const sid = Number(new URL(req.url, "http://x").searchParams.get("session") || 0);
  if (req.url === "/api/offer") {
    const sdp = await boardPage.evaluate(() => window.makeOffer());
    res.setHeader("Content-Type", "application/sdp");
    res.setHeader("X-Session", String(++session));
    return res.end(sdp);
  }
  if (req.url.startsWith("/api/answer")) {
    if (sid !== session) { staleRejected++; res.statusCode = 409; return res.end("stale"); }
    answers++;
    await boardPage.evaluate((s) => window.setAnswer(s), body);
    return res.end("OK");
  }
  if (req.url.startsWith("/api/hangup")) {
    if (sid === session) { hangups++; await boardPage.evaluate(() => window.pc && window.pc.close()); }
    return res.end("OK");
  }
  if (req.url.startsWith("/api/snapshot")) {
    res.setHeader("Content-Type", "image/jpeg");
    res.setHeader("X-Resolution", req.url.includes("hires") ? "4656x3496" : "1280x720");
    res.setHeader("X-Fallback", "0");
    return res.end(jpeg);
  }
  if (req.url === "/api/status") {
    res.setHeader("Content-Type", "application/json");
    return res.end(JSON.stringify({ camera: { connected: true, width: 1280, height: 720, fps: 30, max_width: 4656, max_height: 3496, dropped: 0 },
      encoder: {}, webrtc: await boardPage.evaluate(() => (window.pc && window.pc.connectionState === "connected" ? "connected" : "idle")), ip: "127.0.0.1", heap_internal: 1, heap_psram: 20e6, uptime_s: 1 }));
  }
  res.statusCode = 404; res.end();
});
await new Promise((r) => server.listen(18080, r));

const browser = await chromium.launch({ executablePath: process.env.CHROMIUM_PATH || undefined, args: ["--use-fake-ui-for-media-stream", "--disable-features=WebRtcHideLocalIpsWithMdns"] });
const ctx = await browser.newContext({ acceptDownloads: true });
boardPage = await ctx.newPage();
await boardPage.goto("about:blank");
await boardPage.evaluate(() => {
  const c = document.createElement("canvas"); c.width = 640; c.height = 480;
  const g = c.getContext("2d"); let n = 0;
  setInterval(() => { g.fillStyle = `hsl(${n++ % 360},80%,50%)`; g.fillRect(0, 0, 640, 480); }, 33);
  const track = c.captureStream(30).getVideoTracks()[0];
  window.makeOffer = async () => {
    if (window.pc) window.pc.close();
    const pc = (window.pc = new RTCPeerConnection());
    pc.addTransceiver(track, { direction: "sendonly" });
    await pc.setLocalDescription(await pc.createOffer());
    await new Promise((r) => { setTimeout(r, 1500); if (pc.iceGatheringState === "complete") r(); pc.onicegatheringstatechange = () => pc.iceGatheringState === "complete" && r(); });
    return pc.localDescription.sdp;   // 보드처럼 후보를 SDP 안에 다 넣어서
  };
  window.setAnswer = (sdp) => window.pc.setRemoteDescription({ type: "answer", sdp });
});

const page = await ctx.newPage();
const errors = [];
page.on("pageerror", (e) => errors.push(e.message));
await page.goto("http://127.0.0.1:18080/");
await page.waitForFunction(() => document.getElementById("video").videoWidth > 0, null, { timeout: 15000 });
const st = await page.textContent("#state");
const vw = await page.evaluate(() => document.getElementById("video").videoWidth);
console.log("state:", st, "videoWidth:", vw);
await page.waitForTimeout(2500);
const rx = await page.textContent("#iRx");
const overlay = await page.evaluate(() => getComputedStyle(document.getElementById("overlay")).display);
console.log("rx:", rx, "overlay:", overlay);


await page.click("#btnSnap");
await page.waitForSelector(".shot img");
await page.click("#btnSnapHi");
await page.waitForFunction(() => document.querySelectorAll(".shot").length === 2);
const metas = await page.$$eval(".shot .meta span", (e) => e.map((x) => x.textContent));
const names = await page.$$eval(".shot a", (e) => e.map((x) => x.download));
console.log("shots:", metas, names);
const dl = page.waitForEvent("download");
await page.click(".shot a");
const d = await dl;
const saved = OUT + "/" + d.suggestedFilename();
await d.saveAs(saved);
console.log("download size:", fs.statSync(saved).size, "src:", jpeg.length);
await page.screenshot({ path: OUT + "/viewer.png" });

await page.click("#btnConnect");   // 끊기
await page.waitForTimeout(300);
console.log("hangups:", hangups, "answers:", answers, "pageerrors:", errors);
await page.click("#btnConnect");   // 다시 연결
await page.waitForFunction(() => document.getElementById("state").textContent.includes("실시간"), null, { timeout: 15000 });
console.log("reconnected, answers:", answers);

// 두 번째 기기가 접속하면 첫 기기는 밀려나고, 다시 뺏어 오지 않아야 한다
const page2 = await ctx.newPage();
page2.on("pageerror", (e) => errors.push(e.message));
await page2.goto("http://127.0.0.1:18080/");
await page2.waitForFunction(() => document.getElementById("video").videoWidth > 0, null, { timeout: 15000 });
await page.waitForFunction(() => document.getElementById("state").textContent.includes("다른 기기"), null, { timeout: 40000 });
const answersAfterKick = answers;
await page.waitForTimeout(5000);
const still2 = await page2.evaluate(() => document.getElementById("state").textContent);
console.log("page1:", await page.textContent("#state"), "| page2:", still2, "| answers:", answersAfterKick, "->", answers);
if (answers !== answersAfterKick || !still2.includes("실시간")) { console.log("FAIL: 탭끼리 연결을 뺏음"); process.exit(1); }
await browser.close();
server.close();
if (errors.length || vw === 0 || answers < 2 || hangups < 1 || overlay !== "none" || rx === "-") process.exit(1);
console.log("E2E OK");
