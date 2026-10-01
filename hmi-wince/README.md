# HmiSplash — Windows CE HMI 시작 화면

Autobase Touch Smart(Windows CE)용 .NET Compact Framework 3.5 C# 프로그램.
실행하면 **Now Loading** 화면을 3초 보여준 뒤 **MODE SELECT** 화면으로 바뀝니다.

## 열고 실행하기 (Visual Studio 2008)

1. `HmiSplash.sln` 을 엽니다.
2. Project → HmiSplash Properties → Devices → Target device 를 **Windows CE Device** 로 선택.
3. Debug → Start Debugging (F5). 빌드 후 장치에 자동 배포·실행됩니다.

## 파일

| 파일 | 내용 |
|---|---|
| `HmiSplash/MainForm.cs` | 전체 화면 폼, 3초 타이머, 화면 전환 |
| `HmiSplash/Program.cs` | 진입점 |
| `HmiSplash/Images/*.jpg` | 화면 이미지 (exe 안에 Embedded Resource 로 포함) |

이미지는 실행 시 장치 화면 크기(`Screen.PrimaryScreen.Bounds`)에 맞게 한 번 변환해
둡니다. 원본이 화면 해상도와 같으면 변환 없이 그대로 나옵니다.
늘리기/비율 유지는 `MainForm.cs` 의 `StretchToFill` 로 바꿉니다.

## .sln 이 안 열릴 때 (안내서 방식으로 새로 만들기)

1. File → New → Project → Visual C# → Smart Device → Smart Device Project,
   이름 `HmiSplash`, Target platform **Windows CE**, Device Application.
2. 자동 생성된 `Form1.cs` 와 `Program.cs` 를 삭제하고, 이 폴더의
   `MainForm.cs`, `Program.cs` 를 Add → Existing Item 으로 추가.
3. 프로젝트에 `Images` 폴더를 만들고 jpg 두 개를 추가한 뒤, 각각
   Properties 창에서 **Build Action = Embedded Resource** 로 설정.
4. 기본 네임스페이스(Default namespace)가 `HmiSplash` 인지 확인
   (리소스 이름이 `HmiSplash.Images.loading.jpg` 여야 함).

## 종료

화면에 종료 버튼이 없습니다. 개발 중에는 VS 에서 Debug → Stop Debugging 으로 끕니다.
