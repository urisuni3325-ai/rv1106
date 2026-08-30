# 처음부터 끝까지 따라 하기

RV1106 보드의 카메라 영상을 핸드폰으로 보기까지, 순서대로 하나씩 해봅니다.
중간에 막히면 그 단계에서 멈추고 [문제 해결](troubleshooting.md)을 보세요.
**앞 단계가 안 되면 다음 단계는 절대 안 되니** 순서를 건너뛰지 마세요.

전체 흐름은 이렇습니다.

```
0. 준비물 확인
1. PC 에 프로그램 설치        (한 번만)
2. 이 저장소 내려받기          (한 번만)
3. 보드에 접속하기
4. 스크립트를 보드로 복사
5. 보드를 WiFi 에 연결        ← 여기가 핵심
6. PC 에서 영상이 나오는지 확인 ← 여기까지 되면 거의 다 온 것
7. 화질/지연 조정             (선택)
8. 핸드폰 앱 만들어 설치
9. 앱에서 보기
```

---

## 0단계. 준비물 확인

| 준비물 | 설명 |
|---|---|
| RV1106 보드 + SC3336 카메라 | WiFi 가 달린 모델이어야 합니다 (예: Luckfox Pico Ultra **W**) |
| USB 케이블 | 지금 PC 에 연결해서 쓰던 그 케이블 |
| 공유기(WiFi) | 보드와 핸드폰이 **같은** 공유기에 붙어야 합니다 |
| 안드로이드 폰 | 안드로이드 7.0 이상 |
| Windows / Mac PC | 앱을 만들고 보드에 명령을 내리는 용도 |

> **중요**: 핸드폰이 LTE/5G 로 인터넷을 쓰고 있으면 안 됩니다. 반드시 집 WiFi 에
> 연결된 상태여야 보드가 보입니다.

---

## 명령을 어디서 실행하나

### 먼저 — PC 인가 보드인가

이 문서의 명령은 **PC(Windows)에서 치는 것**과 **보드(리눅스) 안에서 치는 것**이
섞여 있습니다. 이걸 구분하는 게 가장 중요합니다.

| | PC | 보드 |
|---|---|---|
| 프롬프트 | `C:\device\...\android>` | `[root@luckfox scripts]#` |
| 경로 모양 | `C:\` 로 시작 | `/` 로 시작 (`/root/scripts`, `/etc/…`, `/dev/video0`) |
| 대표 명령 | `adb`, `gradlew.bat`, `git` | `./wifi-setup.sh`, `ifconfig`, `wpa_cli` |

보드 안으로 들어가려면 보드를 USB 로 연결한 뒤:

```
C:\platform-tools\adb.exe devices     (기기가 보이는지 확인)
C:\platform-tools\adb.exe shell
```

프롬프트가 `[root@luckfox ~]#` 로 바뀌면 보드 안입니다. 빠져나올 때는 `exit`
또는 Ctrl+D 를 누르면 `C:\…>` 로 돌아옵니다.

> 폰도 함께 꽂혀 있으면 `adb devices` 에 기기가 둘 나오고 `adb shell` 이
> `more than one device` 오류를 냅니다. 폰 USB 를 잠깐 빼거나,
> `adb.exe -s <보드시리얼> shell` 로 지정하세요.

### PC 안에서는 — 현재 폴더가 중요한가

현재 폴더가 중요한 명령과 아무 데서나 되는 명령이 섞여 있습니다.

- 명령 안에 **`C:\` 로 시작하는 전체 경로**만 들어 있으면 → **아무 폴더에서나**
  됩니다. `%USERPROFILE%` 도 Windows 가 `C:\Users\사용자이름` 으로 바꿔주므로
  전체 경로와 같습니다
- **폴더 이름만 짧게 적힌 것**이 있으면(`app\build\…`, `device\scripts` 등) →
  그 폴더가 있는 위치에서 실행해야 합니다

| 명령 | 실행 위치 |
|---|---|
| `type "%USERPROFILE%\.gradle\gradle.properties"` | 아무 데나 |
| `dir "C:\Program Files\Android\…"` | 아무 데나 |
| `C:\platform-tools\adb.exe devices` | 아무 데나 |
| `adb push device\scripts /root/scripts` | 저장소(`rv1106`) 폴더 |
| `gradlew.bat …` | `android` 폴더 |
| `adb install -r app\build\outputs\…` | `android` 폴더 |

현재 폴더는 프롬프트의 `>` **왼쪽**에 표시됩니다. 옮길 때는 `cd /d 경로` 를
쓰세요. `/d` 는 드라이브까지 바꿔주는 옵션이라 붙여두면 안전합니다.

```
C:\Users\user>cd /d C:\dev\rv1106\android
C:\dev\rv1106\android>
```

Mac 은 `cd 경로`, 현재 위치 확인은 `pwd` 입니다.

## 1단계. PC 에 프로그램 설치 (한 번만)

### (1) 안드로이드 스튜디오

앱을 만드는 프로그램입니다. <https://developer.android.com/studio> 에서 받아
설치하세요. 설치 중 물어보는 건 전부 기본값(Next)으로 두면 됩니다.
용량이 크고 시간이 좀 걸립니다(10~20분).

이걸 설치하면 뒤에서 쓸 `adb` 도 같이 깔립니다.

### (2) adb 명령을 어디서든 쓸 수 있게 하기

`adb` 는 PC 에서 보드나 폰에 명령을 내리는 도구입니다. 설치된 위치는 보통:

- **Windows**: `C:\Users\<사용자이름>\AppData\Local\Android\Sdk\platform-tools`
- **Mac**: `~/Library/Android/sdk/platform-tools`

설치가 끝나기 전이라도, adb 만 따로 받으면 3~7단계(보드 작업)를 먼저 진행할 수
있습니다. 아래 접힌 항목을 보세요.

터미널(Windows 는 PowerShell, Mac 은 터미널)을 열고 아래를 쳐서 확인합니다.

```sh
adb version
```

`Android Debug Bridge version ...` 이 나오면 성공입니다.

<details>
<summary><b>"'adb'은(는) 내부 또는 외부 명령... 이 아닙니다" 가 나오면 (Windows)</b></summary>

adb 가 아직 없거나, 있어도 Windows 가 위치를 모르는 상태입니다.

**① 있는지부터 확인** — 명령 프롬프트에 그대로 붙여넣으세요.

```
dir "%LOCALAPPDATA%\Android\Sdk\platform-tools\adb.exe"
```

- `파일을 찾을 수 없습니다` → 아직 없습니다. ②로
- 목록에 `adb.exe` 가 보임 → 있습니다. ③으로

**② 없다면 작은 것만 따로 받기**

안드로이드 스튜디오는 용량이 커서 오래 걸리는데, 보드 작업(3~7단계)에는 adb 만
있으면 됩니다. 먼저 이것만 받아서 진행하세요.

1. <https://dl.google.com/android/repository/platform-tools-latest-windows.zip> 다운로드 (약 10MB)
2. zip 우클릭 → **압축 풀기**
3. 안에 있는 `platform-tools` 폴더를 통째로 `C:\` 로 옮깁니다
4. **`C:\platform-tools\adb.exe`** 가 있으면 성공

> 안드로이드 스튜디오는 8단계에서 필요합니다. 지금 설치를 걸어두면 시간을 아낄 수 있습니다.

**③ 폴더로 이동해서 쓰기**

```
cd C:\platform-tools
adb version
```

안드로이드 스튜디오가 이미 있었다면:

```
cd %LOCALAPPDATA%\Android\Sdk\platform-tools
adb version
```

여기서 버전이 나오면 이대로도 다음 단계를 진행할 수 있습니다. 다만 명령 프롬프트를
새로 열 때마다 `cd` 를 다시 쳐야 합니다.

**④ 매번 cd 하기 싫다면 — PATH 에 등록**

1. 윈도우 키 → **환경 변수** 검색 → **시스템 환경 변수 편집**
2. 창 아래쪽 **환경 변수(N)...** 버튼
3. **위쪽** "사용자 변수" 목록에서 **Path** 선택 → **편집**
4. **새로 만들기** → `C:\platform-tools` 입력 (또는 ③에서 쓴 경로)
5. **확인** 을 세 번 눌러 모든 창을 닫습니다
6. **명령 프롬프트를 완전히 닫고 새로 엽니다** ← 이걸 안 하면 그대로 안 됩니다
7. `adb version`

</details>

<details>
<summary><b>Mac 에서 adb 가 없다고 나오면</b></summary>

```sh
cd ~/Library/Android/sdk/platform-tools
./adb version
```

매번 쓰려면 `~/.zshrc` 에 아래 줄을 추가하고 터미널을 새로 여세요.

```sh
export PATH="$PATH:$HOME/Library/Android/sdk/platform-tools"
```

</details>

### (3) VLC (영상 확인용, 강력 추천)

<https://www.videolan.org/vlc/> 에서 받아 설치하세요.
6단계에서 "보드가 영상을 잘 보내고 있는지"를 확인하는 데 씁니다.
이 확인 단계가 있으면 문제가 보드 쪽인지 앱 쪽인지 바로 갈립니다.

---

## 2단계. 이 저장소 내려받기 (한 번만)

### 어느 위치에 둘 것인가

어디에 둬도 동작하지만, Windows 에서 안드로이드 빌드를 하려면 피해야 할 위치가
있습니다. **`C:\dev\rv1106`** 을 권합니다.

| 피할 위치 | 이유 |
|---|---|
| 바탕 화면, 내 문서 | OneDrive 가 동기화하는 폴더라 빌드 중 파일이 잠기거나 충돌합니다 |
| 경로에 **한글**이 있는 곳 | 안드로이드 빌드 도구가 한글 경로에서 실패하는 경우가 있습니다 |
| 경로에 **띄어쓰기**가 있는 곳 | 명령어에서 따옴표를 빠뜨리면 바로 오류가 납니다 |
| 아주 깊은 폴더 | 빌드 중 경로가 길어져 Windows 260자 제한에 걸릴 수 있습니다 |

Mac 이라면 `~/dev/rv1106` 처럼 홈 폴더 아래 아무 데나 두면 됩니다.

### 내려받기

`git clone` 은 **지금 있는 폴더 안에** `rv1106` 폴더를 새로 만듭니다.
그래서 `C:\dev` 에서 실행하면 결과가 `C:\dev\rv1106` 이 됩니다.

```
mkdir C:\dev
cd C:\dev
git clone https://github.com/urisuni3325-ai/rv1106.git
cd rv1106
git checkout claude/rv1106-sc3336-wifi-streaming-p74k9v
```

`git` 이 없다면 <https://git-scm.com/downloads> 에서 설치하거나,
GitHub 페이지에서 **Code → Download ZIP** 으로 받아 압축을 풀어도 됩니다.
ZIP 으로 받으면 폴더 이름이 `rv1106-claude-rv1106-sc3336...` 처럼 길게 나올 수
있습니다. `rv1106` 으로 이름을 바꿔 `C:\dev\` 에 두면 되고, 이 경우
`git checkout` 은 필요 없습니다.

### 확인

폴더 안에서 `dir` (Mac 은 `ls`) 을 쳤을 때 `android`, `device`, `docs`
세 폴더가 보이면 제대로 된 것입니다.

이 폴더는 앞으로 두 번 더 씁니다.

- **4단계** — `adb push` 명령을 반드시 이 폴더 안에서 실행합니다
- **8단계** — 안드로이드 스튜디오에서 `C:\dev\rv1106\android` 를 엽니다
  (`rv1106` 이 아니라 그 안의 `android`)

앞 단계에서 만든 `C:\platform-tools` 와는 별개의 폴더입니다. 서로 섞지 마세요.

---

## 3단계. 보드에 접속하기

보드를 USB 로 PC 에 연결하고 전원을 켠 뒤, 터미널에서:

```sh
adb devices
```

이렇게 나오면 성공입니다.

```
List of devices attached
1234567890abcdef        device
```

이제 보드 안으로 들어가 봅니다.

```sh
adb shell
```

프롬프트가 `#` 로 바뀌면 **보드 안**입니다. 확인해 보세요.

```sh
ls /dev/video*
```

`/dev/video0` 같은 게 보이면 카메라가 정상입니다.
빠져나오려면 `exit` 를 치거나 Ctrl+D 를 누릅니다.

<details>
<summary><b>adb devices 에 아무것도 안 뜬다면</b></summary>

- USB 케이블을 뽑았다 다시 꽂고, 보드 전원이 켜져 있는지 확인
- Windows 라면 장치 관리자에서 드라이버가 잡혔는지 확인
- 그래도 안 되면 USB 시리얼로 접속합니다. Windows 는
  [PuTTY](https://www.putty.org/), Mac 은 터미널에서:
  ```sh
  # Mac
  ls /dev/tty.usb*            # 포트 이름 확인
  screen /dev/tty.usbserial-XXXX 115200
  ```
  로그인은 보통 `root` / `luckfox` 입니다.

</details>

---

## 4단계. 스크립트를 보드로 복사

**PC 터미널**에서(보드 안이 아니라), 2단계에서 받은 `rv1106` 폴더 안에서 실행합니다.

먼저 지금 위치가 맞는지 확인합니다. `dir` (Mac 은 `ls`) 을 쳤을 때
`android`, `device`, `docs` 세 폴더가 보여야 합니다. 안쪽에 폴더 하나만
덩그러니 있다면 한 단계 더 들어가세요(ZIP 을 풀면 폴더가 두 겹으로 생기곤 합니다).

```sh
adb push device/scripts /root/scripts
adb shell chmod +x /root/scripts/*.sh
```

`4 files pushed` 같은 메시지가 나오면 성공입니다.

> **`adb` 를 찾을 수 없다고 나오면** — PATH 등록을 아직 안 한 것입니다.
> 등록하지 않고 바로 쓰려면 `adb` 자리에 전체 경로를 적으면 됩니다.
> 폴더 위치와는 무관하게 동작합니다.
>
> ```
> C:\platform-tools\adb.exe push device\scripts /root/scripts
> C:\platform-tools\adb.exe shell chmod +x /root/scripts/*.sh
> ```
>
> 5단계부터는 `adb shell` 을 자주 쓰니, 1단계의 PATH 등록을 지금 해두는 편이
> 편합니다. 등록 후에는 **명령 프롬프트를 닫고 새로 열어야** 적용됩니다.

---

## 5단계. 보드를 WiFi 에 연결 ← 핵심

보드 안으로 들어가서 실행합니다.

```sh
adb shell
cd /root/scripts
./wifi-setup.sh -s "우리집공유기이름" -p "와이파이비밀번호"
```

> 따옴표를 꼭 넣으세요. 이름이나 비밀번호에 띄어쓰기·특수문자가 있으면
> 따옴표 없이는 실패합니다.

<details>
<summary><b>5GHz 를 써도 되나요?</b></summary>

됩니다. 스크립트는 밴드를 구분하지 않으므로 **옵션을 추가할 필요 없이 5GHz 쪽
SSID 를 그대로 넣으면** 됩니다.

```sh
./wifi-setup.sh -s "우리집공유기_5G" -p "와이파이비밀번호"
```

관건은 보드의 WiFi 모듈이 5GHz 를 지원하느냐입니다. RV1106 보드에 쓰이는 모듈은
모델에 따라 2.4GHz 전용인 것과 듀얼밴드인 것이 섞여 있어서, **5GHz SSID 로 한 번
돌려보는 게 가장 빠른 확인 방법**입니다.

- 연결 완료 + IP 가 나오면 → 지원됩니다
- 실패하면 → 2.4GHz SSID 로 다시 돌려 진행하고, 아래로 확인하세요

2.4GHz 로 붙은 상태에서(wpa_supplicant 가 떠 있어야 동작합니다):

```sh
wpa_cli -i wlan0 scan
wpa_cli -i wlan0 scan_results
```

`frequency` 열에 5180·5745 같은 **5000번대 숫자가 하나도 없으면** 2.4GHz 전용
칩입니다. 이 경우 2.4GHz 로 쓰시면 됩니다.

**어느 쪽이 나은가**

| | 5GHz | 2.4GHz |
|---|---|---|
| 속도·간섭 | 넉넉함. 1080p 에 여유 | 전자레인지·블루투스와 간섭 |
| 도달 거리 | 짧음. 벽에 약함 | 김. 벽 두세 개도 통과 |
| 추천 | 보드가 공유기와 같은 방 | 보드가 벽 너머 먼 곳 |

영상 전송이라 가능하면 5GHz 가 낫지만, 전파가 약한 자리라면 2.4GHz 가 훨씬
안정적입니다. 보드를 놓을 위치를 기준으로 고르세요.

**알아둘 점**

- 폰과 보드가 서로 다른 밴드여도 됩니다. 보드가 5GHz, 폰이 2.4GHz 여도 같은
  공유기라면 통신됩니다. 밴드가 아니라 *같은 공유기*인지가 조건입니다.
- 게스트 네트워크는 피하세요. 게스트 WiFi 나 "AP 격리(AP Isolation)" 가 켜져
  있으면 기기끼리 서로 보지 못합니다.
- 5GHz 가 유독 안 붙으면 공유기의 5GHz 채널을 **36~48** 중 하나로 고정해 보세요.
  스크립트는 국가 코드를 `KR` 로 넣어 한국 채널이 열려 있습니다. 해외라면
  `-c US` 처럼 바꾸면 됩니다.

</details>

이런 식으로 진행됩니다.

```
[1/5] 인터페이스: wlan0
[2/5] /etc/wpa_supplicant.conf 작성
[3/5] 기존 연결 정리
[4/5] 우리집공유기이름 에 접속
[5/5] DHCP 주소 요청

연결 완료. 보드 IP: 192.168.0.53
핸드폰 앱 설정에 넣을 주소:  rtsp://192.168.0.53:554/live/0
```

**마지막 줄의 주소를 메모해 두세요.** 8단계에서 앱에 넣을 값입니다.

이어서, 보드를 껐다 켜도 자동으로 WiFi 에 붙게 만듭니다.

```sh
./wifi-autostart.sh
```

마지막으로 전체 상태를 점검합니다.

```sh
./check-stream.sh
```

아래 네 가지가 모두 확인되면 통과입니다.

- `wlan0` 에 IP 가 붙어 있음
- `rkipc: 실행 중`
- `/dev/video0` 존재
- `554 포트 열림`

<details>
<summary><b>"무선 인터페이스(wlan*)를 찾지 못했습니다" 라고 나오면</b></summary>

보드에 WiFi 하드웨어가 없거나 드라이버가 안 올라온 상태입니다.

```sh
dmesg | grep -i -E 'aic|wifi|wlan'
ls /sys/class/net/
```

`wlan0` 이 목록에 없다면 WiFi 없는 보드 모델이거나 이미지에 드라이버가
빠진 것입니다. 이 경우 WiFi 전송은 불가능하니 보드 모델을 먼저 확인하세요.

</details>

<details>
<summary><b>"DHCP 실패" 가 나오면</b></summary>

비밀번호가 틀렸을 가능성이 가장 큽니다. 다시 실행해 보고, 그래도 안 되면
보드에서 아래로 상태를 확인하세요.

```sh
wpa_cli -i wlan0 status
```

`wpa_state=COMPLETED` 가 아니면 SSID/비밀번호 문제입니다.

</details>

---

## 6단계. PC 에서 영상이 나오는지 확인 ← 여기까지 되면 거의 다 왔습니다

**PC 를 보드와 같은 WiFi 에 연결한 상태**에서 VLC 를 엽니다.

1. VLC 실행
2. 메뉴에서 **미디어 → 네트워크 스트림 열기** (Mac: 파일 → 네트워크 열기)
3. 5단계에서 메모한 주소 입력: `rtsp://192.168.0.53:554/live/0`
4. **재생**

카메라 영상이 나오면 **보드 쪽은 완전히 끝난 것**입니다.
(VLC 는 기본 설정 때문에 1초 정도 느리게 보입니다. 정상입니다.
앱은 이보다 훨씬 빠릅니다.)

> 여기서 안 나오면 앱을 만들어도 안 나옵니다. 5단계로 돌아가
> `./check-stream.sh` 결과를 다시 확인하세요.

---

## 7단계. 화질과 지연 조정 (선택, 나중에 해도 됨)

일단 영상이 나온 뒤에 손보면 되는 부분입니다. 보드 안에서:

```sh
cd /root/scripts
./stream-tune.sh --preset wifi
```

- `--preset wifi` : 1920x1080 / 25fps / 3Mbps — 보통 이걸로 충분합니다
- `--preset lowlatency` : 1280x720 / 30fps / 2Mbps — 지연을 최대한 줄이고 싶을 때
- `--preset quality` : 2304x1296 / 6Mbps — 화질 우선(지연은 늘어남)

**코덱은 반드시 H.264 여야 합니다.** 이 앱은 H.264 만 처리합니다. 보드가
H.265 로 설정돼 있으면 VLC 로는 잘 보이는데 앱만 검은 화면이 됩니다.

```sh
./stream-tune.sh --show          # output_data_type 확인
./stream-tune.sh --codec h264    # H.265 로 나오면 바꾸기
```

위 `--preset` 들은 코덱도 함께 H.264 로 맞춰줍니다.

원본 설정은 자동으로 백업되고, 적용 후 rkipc 가 재시작됩니다.
자세한 내용은 [지연 시간 줄이기](latency-tuning.md)를 보세요.

---

## 8단계. 핸드폰 앱 만들어 설치

### (1) 폰에서 USB 디버깅 켜기

1. **설정 → 휴대전화 정보** 로 들어갑니다
2. **빌드번호** 를 **7번 연속** 탭합니다 → "개발자가 되었습니다" 메시지
3. **설정 → 개발자 옵션** 에서 **USB 디버깅** 을 켭니다
4. 폰을 USB 로 PC 에 연결하고, 폰 화면에 뜨는 **"USB 디버깅을 허용하시겠습니까?"**
   에서 **허용** 을 누릅니다

> 보드도 USB 로 연결돼 있다면, 이 단계에서는 보드를 잠깐 빼두는 게 헷갈리지
> 않습니다.

### (2) 안드로이드 스튜디오에서 열기

1. 안드로이드 스튜디오 실행
2. **Open** (또는 File → Open) 클릭
3. 2단계에서 받은 폴더 안의 **`android` 폴더**를 선택 (`rv1106` 폴더가 아니라
   그 안의 `android` 입니다)
4. 오른쪽 아래에 진행 표시줄이 돌아갑니다. 필요한 파일을 인터넷에서 받아오는
   과정이라 **처음 한 번은 5~15분** 걸립니다. 끝날 때까지 기다리세요.
   (`Gradle sync finished` 가 뜨면 완료)

제대로 열렸다면 왼쪽 프로젝트 패널에 `app`, `gradle`, `build.gradle.kts`,
`settings.gradle.kts` 가 보입니다. `scripts` 와 `README.md` 만 보인다면
`device` 폴더를 연 것이니 다시 여세요.

<details>
<summary><b>"Incompatible Gradle JVM version" 오류가 나오면</b></summary>

```
The project's Gradle version 8.9 is incompatible with the Gradle JVM
version 25 currently selected to run Gradle build.
```

Gradle 8.9 가 아직 지원하지 않는 최신 JDK 가 선택돼 있어서 납니다. 프로젝트가
아니라 IDE 설정 문제라 클릭 몇 번이면 됩니다.

**가장 빠른 방법** — 오류 아래 파란 링크
**Apply compatible Gradle JDK configuration and sync** 를 누르면 안드로이드
스튜디오가 알아서 호환되는 JDK 로 바꾸고 다시 동기화합니다.

**직접 지정하려면**

1. **File → Settings** (Mac 은 Android Studio → Settings)
2. **Build, Execution, Deployment → Build Tools → Gradle**
3. **Gradle JDK** 를 **`jbr-21`** 또는 **`Embedded JDK`** 로 변경.
   목록에 21 이 없으면 17 도 됩니다. 안드로이드 스튜디오에 딸려오는 JDK 라
   호환이 보장됩니다
4. **OK** → **File → Sync Project with Gradle Files**

JDK 를 지우거나 다시 설치할 필요는 없습니다. 이 프로젝트에서 쓸 JDK 만
지정하는 것이라 다른 작업에는 영향이 없습니다.

**설정을 바꿨는데도 같은 오류가 반복되면**

원인이 둘 중 하나입니다. 순서대로 확인하세요.

**① 사용자 폴더의 Gradle 설정이 JDK 를 강제하는 경우**

```
type "%USERPROFILE%\.gradle\gradle.properties"
```

`org.gradle.java.home=...` 줄이 있으면 그게 원인입니다. IDE 에서 무엇을 고르든
이 값이 이깁니다. `notepad "%USERPROFILE%\.gradle\gradle.properties"` 로 열어
**그 줄만** 지우고 저장한 뒤, 안드로이드 스튜디오를 완전히 종료했다가 다시 열어
Sync 하세요. (`지정된 파일을 찾을 수 없습니다` 가 나오면 이 경우가 아닙니다)

**② 안드로이드 스튜디오에 딸려온 JDK 가 Gradle 보다 최신인 경우**

실제로 어떤 JDK 가 쓰이는지는 명령줄에서 확인하는 게 확실합니다.

```
cd /d C:\dev\rv1106\android
gradlew.bat --version
```

`Launcher JVM: 25.x` 처럼 나오면 이 경우입니다. 최근 안드로이드 스튜디오는
JBR 25 를 함께 설치하는데, Gradle 8.9 는 아직 Java 25 를 지원하지 않습니다.
설정 창에 `jbr-21` 이라고 보여도 폴더 내용이 업데이트되어 라벨만 남은 것일 수
있으니, 라벨이 아니라 위 출력을 믿으세요.

Java 21 을 하나 마련하면 됩니다. 안드로이드 스튜디오가 대신 받아줍니다.

1. **File → Settings → Build, Execution, Deployment → Build Tools → Gradle**
2. **Gradle JDK** 드롭다운 맨 아래 **`Download JDK...`**
3. **Version** `21`, **Vendor** `Eclipse Temurin`, Location 은 기본값
4. **Download** → 끝나면 자동 선택됨 → **OK** → **Sync Project with Gradle Files**

명령줄로 빌드할 때는 받은 JDK 를 직접 지정합니다. 폴더 이름은
`dir "%USERPROFILE%\.jdks"` 로 확인하세요.

```
set JAVA_HOME=C:\Users\사용자이름\.jdks\temurin-21.0.5
gradlew.bat --version
```

`Launcher JVM: 21.x` 로 바뀌면 됩니다. 직접 설치하고 싶다면
<https://adoptium.net/temurin/releases/?version=21> 에서 Windows x64 `.msi` 를
받아 기본값으로 설치해도 결과는 같습니다.

</details>

<details>
<summary><b>안드로이드 스튜디오 없이 명령줄로 APK 만들기</b></summary>

IDE 설정과 씨름하기 싫을 때 쓰는 방법입니다. JDK 를 명령줄에서 직접 지정하므로
`gradle.properties` 나 IDE 설정에 영향을 받지 않습니다.

먼저 안드로이드 스튜디오에 딸려온 JDK 경로를 확인합니다.

```
dir "C:\Program Files\Android\Android Studio\jbr\bin\java.exe"
```

`java.exe` 가 보이면 빌드합니다.

```
cd /d C:\dev\rv1106\android
gradlew.bat "-Dorg.gradle.java.home=C:\Program Files\Android\Android Studio\jbr" assembleDebug
```

따옴표는 `-D` 부터 경로 끝까지 통째로 감쌉니다. 경로에 띄어쓰기가 있어서
그렇습니다. Mac 이라면:

```sh
cd ~/dev/rv1106/android
./gradlew assembleDebug
```

`BUILD SUCCESSFUL` 이 나오면 폰에 설치합니다.

```
C:\platform-tools\adb.exe install -r app\build\outputs\apk\debug\app-debug.apk
```

`Success` 가 뜨면 폰에 "RV1106 카메라" 앱이 생깁니다. 이 방법으로 설치했다면
안드로이드 스튜디오는 더 쓰지 않아도 됩니다. 앱을 고쳐서 다시 설치할 때도 위
두 명령만 반복하면 됩니다.

</details>

### (3) 실행

1. 화면 위쪽 기기 선택 칸에 **연결한 폰 이름**이 보이는지 확인
2. 초록색 **▶ Run** 버튼 클릭
3. 앱이 자동으로 설치되고 폰에서 실행됩니다

<details>
<summary><b>기기 목록에 폰이 안 보이면</b></summary>

```sh
adb devices
```

폰이 `unauthorized` 로 나오면 폰 화면의 허용 팝업을 놓친 것입니다.
케이블을 뽑았다 꽂고 다시 **허용** 을 누르세요.

</details>

---

## 9단계. 앱에서 보기

1. 앱이 열리면 아래쪽 **설정** 버튼을 누릅니다
2. **RTSP 주소** 칸에 5단계에서 메모한 주소를 넣습니다
   → `rtsp://192.168.0.53:554/live/0`
3. 아이디/비밀번호는 보드에 인증을 걸어두지 않았다면 **비워둡니다**
4. **저장** 을 누릅니다

잠시 뒤 화면에 카메라 영상이 나옵니다. 왼쪽 위에 **"스트리밍 중"** 이라고
표시되면 정상입니다.

### 버튼 사용법

| 버튼 | 하는 일 |
|---|---|
| **연결 / 끊기** | 스트림에 붙거나 끊습니다. 끊겨도 자동으로 다시 붙습니다 |
| **● 녹화** | 녹화 시작. 다시 누르면 정지하고 저장됩니다 |
| **캡처** | 지금 화면을 사진으로 저장합니다 |
| **갤러리** | 저장한 영상과 사진 목록. 눌러서 재생, **길게 눌러** 내보내기·공유·삭제 |
| **설정** | 주소·아이디·비밀번호 변경 |

> **녹화 버튼을 눌렀는데 "준비 중…" 이 잠깐 뜨는 건 정상입니다.**
> 영상이 깨지지 않게 키프레임이 오는 시점부터 저장을 시작하기 때문이고,
> 보통 1~2초입니다.

### 저장한 영상을 폰의 기본 갤러리 앱에서도 보고 싶다면

이 앱은 저장소 권한을 요구하지 않으려고 앱 전용 폴더에 저장합니다.
다른 앱에서도 보려면 갤러리 화면에서 항목을 **길게 눌러 → 갤러리로 내보내기**
를 선택하세요.

---

## 자주 겪는 상황

| 증상 | 확인할 것 |
|---|---|
| 앱이 계속 "재연결 중" | 폰이 WiFi 가 아니라 LTE 로 붙어 있지 않은지 |
| 어제는 됐는데 오늘 안 됨 | 보드 IP 가 바뀌었을 수 있음 → 보드에서 `./check-stream.sh` 로 새 IP 확인 후 앱 설정 수정 |
| 화면이 까맣기만 함 | 첫 키프레임 대기 중. `./stream-tune.sh --gop 25` 로 해결 |
| 영상이 뚝뚝 끊김 | 전파가 약한 것. 보드를 공유기 가까이 옮기거나 `--preset lowlatency` |
| 지연이 1초 이상 | [지연 시간 줄이기](latency-tuning.md) |

증상별 자세한 해결은 [문제 해결](troubleshooting.md)에 정리해 두었습니다.

## 매번 반복할 일 / 한 번만 할 일

한 번만 하면 되는 것: 1, 2, 4, 5(자동 접속까지), 8단계
보드를 껐다 켤 때: 아무것도 안 해도 됩니다(자동 접속됨).
공유기를 바꾸거나 비밀번호가 바뀌면: 5단계만 다시 하세요.
