using System;
using System.Drawing;
using System.IO;
using System.Reflection;
using System.Windows.Forms;

namespace HmiSplash
{
    /// <summary>
    /// 전체 화면 폼. 처음에는 로딩 화면을 보여주고, 3초 뒤 모드 선택 화면으로 바꾼다.
    /// 이미지는 실행 파일에 포함(Embedded Resource)되어 있으므로 따로 복사할 필요가 없다.
    /// </summary>
    public class MainForm : Form
    {
        // 첫 화면 → 두 번째 화면 전환까지 걸리는 시간(ms)
        private const int SplashDurationMs = 3000;

        // true  : 화면 전체에 늘려서 채움 (이미지 비율이 화면과 다르면 약간 찌그러짐)
        // false : 비율 유지, 남는 부분은 검은 여백
        private const bool StretchToFill = true;

        private Bitmap loadingScreen;     // 1번 화면 (Now Loading)
        private Bitmap modeSelectScreen;  // 2번 화면 (MODE SELECT)
        private Bitmap current;           // 지금 그리고 있는 화면
        private Timer splashTimer;

        public MainForm()
        {
            // 테두리/제목줄 없이 화면 전체 사용
            this.Text = "HmiSplash";
            this.FormBorderStyle = FormBorderStyle.None;
            this.ControlBox = false;
            this.MinimizeBox = false;
            this.MaximizeBox = false;
            this.WindowState = FormWindowState.Maximized;
            this.Bounds = Screen.PrimaryScreen.Bounds;
            this.BackColor = Color.Black;

            Size screen = Screen.PrimaryScreen.Bounds.Size;

            // 화면 해상도에 맞게 한 번만 미리 축소/확대해 둔다 → 그릴 때 빠르고 깜빡임 없음
            loadingScreen = LoadScaled("loading.jpg", screen);
            modeSelectScreen = LoadScaled("mode_select.jpg", screen);
            current = loadingScreen;

            splashTimer = new Timer();
            splashTimer.Interval = SplashDurationMs;
            splashTimer.Tick += new EventHandler(OnSplashTimerTick);
        }

        protected override void OnLoad(EventArgs e)
        {
            base.OnLoad(e);
            splashTimer.Enabled = true;
        }

        private void OnSplashTimerTick(object sender, EventArgs e)
        {
            splashTimer.Enabled = false;   // 한 번만 실행
            current = modeSelectScreen;
            this.Invalidate();
        }

        // 배경을 따로 지우지 않는다(깜빡임 방지). 전체를 OnPaint 에서 덮어쓴다.
        protected override void OnPaintBackground(PaintEventArgs e)
        {
        }

        protected override void OnPaint(PaintEventArgs e)
        {
            if (current != null)
            {
                e.Graphics.DrawImage(current, 0, 0);
            }
        }

        /// <summary>
        /// 포함된 이미지 리소스를 읽어서 화면 크기(target)의 비트맵으로 만들어 돌려준다.
        /// </summary>
        private static Bitmap LoadScaled(string fileName, Size target)
        {
            // 리소스 이름 = 기본 네임스페이스 + 폴더 + 파일명
            string resourceName = "HmiSplash.Images." + fileName;
            Assembly asm = Assembly.GetExecutingAssembly();

            using (Stream stream = asm.GetManifestResourceStream(resourceName))
            {
                if (stream == null)
                {
                    throw new FileNotFoundException("리소스를 찾을 수 없습니다: " + resourceName);
                }

                using (Bitmap original = new Bitmap(stream))
                {
                    Bitmap scaled = new Bitmap(target.Width, target.Height);
                    using (Graphics g = Graphics.FromImage(scaled))
                    {
                        g.Clear(Color.Black);

                        Rectangle src = new Rectangle(0, 0, original.Width, original.Height);
                        Rectangle dest = StretchToFill
                            ? new Rectangle(0, 0, target.Width, target.Height)
                            : FitRect(original.Size, target);

                        g.DrawImage(original, dest, src, GraphicsUnit.Pixel);
                    }
                    return scaled;
                }
            }
        }

        // 비율을 유지하면서 target 안에 가운데 정렬로 들어가는 사각형
        private static Rectangle FitRect(Size image, Size target)
        {
            float scale = Math.Min((float)target.Width / image.Width,
                                   (float)target.Height / image.Height);
            int w = (int)(image.Width * scale);
            int h = (int)(image.Height * scale);
            return new Rectangle((target.Width - w) / 2, (target.Height - h) / 2, w, h);
        }

        protected override void Dispose(bool disposing)
        {
            if (disposing)
            {
                if (splashTimer != null) splashTimer.Dispose();
                if (loadingScreen != null) loadingScreen.Dispose();
                if (modeSelectScreen != null) modeSelectScreen.Dispose();
            }
            base.Dispose(disposing);
        }
    }
}
