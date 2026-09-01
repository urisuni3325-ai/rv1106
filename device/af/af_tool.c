/* RV1106 두피 스코프용 오토포커스 도구.
 *
 * 하드웨어 I2C 컨트롤러(i2c-3/i2c-4)가 핀헤더로 나와 있지 않아서
 * GPIO 두 개를 비트뱅해 DW9714 VCM 드라이버를 제어한다.
 *
 * 기본 배선:  SDA = GPIO1_A0 (32),  SCL = GPIO1_A1 (33)
 */
#include "af_search.h"
#include "dw9714.h"
#include "focus_score.h"
#include "gpio_i2c.h"
#include "v4l2_cap.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define FOCUS_STATE_PATH "/userdata/af_focus.conf"

typedef struct {
    gpio_i2c_t *bus;
    v4l2_cap_t *cap;
    int roi_percent;
    int settle_ms;
    int backlash;
    int verbose;
} af_context_t;

static void msleep(int ms)
{
    struct timespec ts = { ms / 1000, (long)(ms % 1000) * 1000000L };
    nanosleep(&ts, NULL);
}

/* af_search 가 부르는 측정 콜백: 렌즈를 옮기고 한 프레임 재서 점수를 낸다. */
static double measure_at(int position, void *ctx)
{
    af_context_t *af = (af_context_t *)ctx;

    if (dw9714_set_from_below(af->bus, position, af->backlash, af->settle_ms) < 0) {
        fprintf(stderr, "VCM 이동 실패: %s\n", gpio_i2c_strerror(af->bus));
        return -1.0;
    }

    /* 이동 직후 프레임은 아직 이전 위치일 수 있으므로 한 장 버린다. */
    if (!v4l2_cap_grab(af->cap, NULL, NULL, NULL))
        return -1.0;

    int width = 0, height = 0, stride = 0;
    const uint8_t *luma = v4l2_cap_grab(af->cap, &width, &height, &stride);
    if (!luma)
        return -1.0;

    int rx, ry, rw, rh;
    focus_score_center_roi(width, height, af->roi_percent / 100.0, &rx, &ry, &rw, &rh);
    focus_score_t score = focus_score_tenengrad(luma, stride, width, height, rx, ry, rw, rh);

    if (af->verbose)
        printf("  위치 %4d  선명도 %10.2f  밝기 %6.1f\n",
               position, score.normalized, score.mean_luma);

    return score.normalized;
}

static int save_focus(int position)
{
    FILE *f = fopen(FOCUS_STATE_PATH, "w");
    if (!f) {
        fprintf(stderr, "초점값 저장 실패: %s\n", FOCUS_STATE_PATH);
        return -1;
    }
    fprintf(f, "%d\n", position);
    fclose(f);
    printf("초점값 %d 을 %s 에 저장했습니다\n", position, FOCUS_STATE_PATH);
    return 0;
}

static int load_focus(void)
{
    FILE *f = fopen(FOCUS_STATE_PATH, "r");
    if (!f)
        return -1;
    int position = -1;
    if (fscanf(f, "%d", &position) != 1)
        position = -1;
    fclose(f);
    return position;
}

/* 한 줄 입력에서 앞뒤 공백과 개행을 떼어낸다. */
static void trim(char *s)
{
    size_t len = strlen(s);
    while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r' ||
                       s[len - 1] == ' '  || s[len - 1] == '\t'))
        s[--len] = 0;
    size_t start = 0;
    while (s[start] == ' ' || s[start] == '\t')
        start++;
    if (start)
        memmove(s, s + start, len - start + 1);
}

static void focus_help(int step)
{
    printf("  +  /  -    %d 스텝씩 이동 (+++ 처럼 붙이면 그만큼 크게)\n", step);
    printf("  <숫자>     그 위치로 바로 이동 (0~1023)\n");
    printf("  step <n>   한 번에 움직일 양을 바꾼다\n");
    printf("  s          지금 위치를 초점값으로 저장\n");
    printf("  q          끝내기 (엔터만 치면 직전 명령을 한 번 더)\n");
}

/* 수동으로 초점을 맞춘다.
 *
 * 카메라를 열지 않기 때문에 rkipc 를 죽일 필요가 없다. 스트리밍은 그대로 두고
 * 폰 화면을 보면서 렌즈를 조금씩 옮기다가, 제일 선명한 자리에서 s 를 누르면 된다.
 * 오토포커스가 안 될 때(무늬가 없는 두피, VCM 이 약할 때) 쓰는 방법이다.
 */
static int focus_interactive(gpio_i2c_t *bus, int start, int step,
                             int min_pos, int max_pos, int backlash, int settle_ms)
{
    int position = start;
    if (position < min_pos) position = min_pos;
    if (position > max_pos) position = max_pos;
    if (step < 1) step = 1;

    printf("수동 초점 맞추기 — 폰 화면을 보면서 조절하세요.\n");
    focus_help(step);
    printf("\n");

    if (dw9714_set_from_below(bus, position, backlash, settle_ms) < 0) {
        fprintf(stderr, "이동 실패: %s\n", gpio_i2c_strerror(bus));
        return -1;
    }
    printf("위치 %d\n", position);
    fflush(stdout);

    char line[64] = {0};
    char last[64] = {0};

    while (fgets(line, sizeof(line), stdin)) {
        trim(line);
        if (line[0] == 0)
            snprintf(line, sizeof(line), "%s", last);   /* 엔터만 = 직전 명령 반복 */
        else
            snprintf(last, sizeof(last), "%s", line);

        if (line[0] == 0)
            continue;
        if (!strcmp(line, "q") || !strcmp(line, "quit") || !strcmp(line, "exit"))
            break;
        if (!strcmp(line, "h") || !strcmp(line, "?")) {
            focus_help(step);
            continue;
        }
        if (!strcmp(line, "s") || !strcmp(line, "save")) {
            save_focus(position);
            continue;
        }
        if (!strncmp(line, "step", 4)) {
            int value = atoi(line + 4);
            if (value < 1 || value > 512) {
                printf("  1~512 사이로 정하세요\n");
            } else {
                step = value;
                printf("  이제 한 번에 %d 씩 움직입니다\n", step);
            }
            continue;
        }

        int target = position;
        if (line[0] == '+' || line[0] == '-') {
            char sign = line[0];
            int count = 0;
            while (line[count] == sign)
                count++;
            if (line[count] != 0) {
                printf("  모르는 입력입니다. h 를 치면 도움말이 나옵니다\n");
                continue;
            }
            target += (sign == '+' ? 1 : -1) * step * count;
        } else if (line[0] >= '0' && line[0] <= '9') {
            target = atoi(line);
        } else {
            printf("  모르는 입력입니다. h 를 치면 도움말이 나옵니다\n");
            continue;
        }

        if (target < min_pos) target = min_pos;
        if (target > max_pos) target = max_pos;
        if (target == position) {
            printf("위치 %d (끝)\n", position);
            fflush(stdout);
            continue;
        }

        if (dw9714_set_from_below(bus, target, backlash, settle_ms) < 0) {
            fprintf(stderr, "이동 실패: %s\n", gpio_i2c_strerror(bus));
            return -1;
        }
        position = target;
        printf("위치 %d\n", position);
        fflush(stdout);
    }

    printf("끝냈습니다. 마지막 위치 %d\n", position);
    return 0;
}

static void usage(void)
{
    printf(
"사용법: af_tool <명령> [옵션]\n"
"\n"
"명령\n"
"  scan                 비트뱅 I2C 버스를 훑어 응답하는 주소를 찾는다\n"
"  ping                 DW9714(0x0C) 가 응답하는지만 본다\n"
"  set <위치>           렌즈를 그 위치로 옮긴다 (0~1023)\n"
"  save <위치>          그 위치로 옮기고 초점값으로 저장한다 (수동 보정용)\n"
"  focus                수동 초점 맞추기 — +/- 로 옮기고 s 로 저장 (카메라 불필요)\n"
"  sweep                전 구간을 훑는다 — 렌즈가 실제로 움직이는지 눈으로 확인\n"
"  score                현재 위치의 선명도를 한 번 잰다 (카메라 필요)\n"
"  af                   오토포커스를 돌리고 결과 위치로 이동한다 (카메라 필요)\n"
"  restore              저장해 둔 초점값으로 즉시 복귀한다\n"
"  list-video           캡처 가능한 /dev/video* 노드를 나열한다\n"
"\n"
"옵션\n"
"  --sda <핀>           기본 32  (GPIO1_A0). chardev 백엔드면 \"1:0\" 형식\n"
"  --scl <핀>           기본 33  (GPIO1_A1)\n"
"  --backend <이름>     sysfs (기본) 또는 chardev\n"
"  --video <경로>       기본 /dev/video0\n"
"  --roi <퍼센트>       중앙 ROI 크기, 기본 25\n"
"  --from/--to/--step   sweep·af 의 탐색 구간, focus 의 이동 간격\n"
"  --settle <ms>        VCM 정착 대기, 기본 25\n"
"  --backlash <스텝>    히스테리시스 보정용 되돌림, 기본 40\n"
"  --save               af 결과를 저장한다\n"
"  -v                   측정값을 모두 출력한다\n"
"\n"
"예시\n"
"  af_tool scan                       # 배선 확인 — 0x0C 가 보이면 DW9714 정상\n"
"  af_tool sweep --step 128           # 렌즈가 움직이는지 눈으로 확인\n"
"  af_tool af --video /dev/video0 -v --save\n"
"  af_tool restore                    # 부팅 후 저장된 초점으로 바로 복귀\n"
"\n"
"수동 보정 — 카메라를 열지 않으므로 스트리밍을 멈출 필요가 없다.\n"
"오토포커스가 안 되거나(무늬 없는 두피 등) 부품이 아직 없을 때 쓰는 방법이다.\n"
"  af_tool focus       # 폰 화면을 보면서 +/- 로 옮기고, 제일 선명할 때 s\n"
"  af_tool restore     # 다음부터는 이 한 줄\n"
"\n"
"한 번에 한 위치만 시도하려면\n"
"  af_tool set 400     # 그 위치로만 옮긴다\n"
"  af_tool save 420    # 옮기고 초점값으로 저장한다\n");
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        usage();
        return 1;
    }

    const char *command = argv[1];
    const char *sda_spec = "32";
    const char *scl_spec = "33";
    const char *video = "/dev/video0";
    gpio_backend_t backend = GPIO_BACKEND_SYSFS;
    af_config_t cfg = af_config_default();
    int roi_percent = 25;
    int settle_ms = 25;
    int backlash = 40;
    int verbose = 0;
    int do_save = 0;
    int sweep_step = 64;
    int have_step = 0;

    for (int i = 2; i < argc; i++) {
        const char *arg = argv[i];
        const char *next = (i + 1 < argc) ? argv[i + 1] : NULL;

        if (!strcmp(arg, "-v")) { verbose = 1; }
        else if (!strcmp(arg, "--save")) { do_save = 1; }
        else if (!strcmp(arg, "-h") || !strcmp(arg, "--help")) { usage(); return 0; }
        else if (!next) { fprintf(stderr, "%s 에 값이 필요합니다\n", arg); return 1; }
        else if (!strcmp(arg, "--sda")) { sda_spec = next; i++; }
        else if (!strcmp(arg, "--scl")) { scl_spec = next; i++; }
        else if (!strcmp(arg, "--video")) { video = next; i++; }
        else if (!strcmp(arg, "--roi")) { roi_percent = atoi(next); i++; }
        else if (!strcmp(arg, "--settle")) { settle_ms = atoi(next); i++; }
        else if (!strcmp(arg, "--backlash")) { backlash = atoi(next); i++; }
        else if (!strcmp(arg, "--from")) { cfg.min_pos = atoi(next); i++; }
        else if (!strcmp(arg, "--to")) { cfg.max_pos = atoi(next); i++; }
        else if (!strcmp(arg, "--step")) { sweep_step = atoi(next); cfg.coarse_step = sweep_step; have_step = 1; i++; }
        else if (!strcmp(arg, "--backend")) {
            if (!strcmp(next, "chardev")) backend = GPIO_BACKEND_CHARDEV;
            else if (!strcmp(next, "sysfs")) backend = GPIO_BACKEND_SYSFS;
            else { fprintf(stderr, "알 수 없는 백엔드: %s\n", next); return 1; }
            i++;
        }
        else { fprintf(stderr, "알 수 없는 옵션: %s\n", arg); return 1; }
    }

    if (backend == GPIO_BACKEND_CHARDEV && !strcmp(sda_spec, "32")) {
        sda_spec = "1:0";
        scl_spec = "1:1";
    }

    if (!strcmp(command, "-h") || !strcmp(command, "--help") || !strcmp(command, "help")) {
        usage();
        return 0;
    }

    if (!strcmp(command, "list-video")) {
        v4l2_cap_list();
        return 0;
    }

    /* I2C 를 열기 전에 명령 이름부터 확인한다. 오타 때문에 GPIO 를 잡을 이유가 없다. */
    static const char *const commands[] = {
        "scan", "ping", "set", "sweep", "score", "af", "restore", "save", "focus", NULL
    };
    int known = 0;
    for (int i = 0; commands[i]; i++) {
        if (!strcmp(command, commands[i])) { known = 1; break; }
    }
    if (!known) {
        fprintf(stderr, "알 수 없는 명령: %s\n\n", command);
        usage();
        return 1;
    }

    /* 여기서부터는 I2C 가 필요하다. */
    int sda = gpio_i2c_parse_pin(sda_spec, backend);
    int scl = gpio_i2c_parse_pin(scl_spec, backend);
    if (sda < 0 || scl < 0) {
        fprintf(stderr, "핀 지정이 잘못됐습니다 (sysfs 는 \"32\", chardev 는 \"1:0\" 형식)\n");
        return 1;
    }

    gpio_i2c_t *bus = gpio_i2c_open(backend, sda, scl, 0);
    if (!gpio_i2c_ok(bus)) {
        fprintf(stderr, "I2C 초기화 실패: %s\n", gpio_i2c_strerror(bus));
        gpio_i2c_close(bus);
        return 1;
    }

    int status = 0;

    if (!strcmp(command, "scan")) {
        printf("비트뱅 I2C 스캔 (SDA=%s SCL=%s)\n", sda_spec, scl_spec);
        int found = 0;
        for (int addr = 0x08; addr <= 0x77; addr++) {
            if (gpio_i2c_ping(bus, (uint8_t)addr) == 1) {
                const char *hint = "";
                if (addr == DW9714_ADDR) hint = "  <- DW9714 VCM 드라이버";
                else if (addr == 0x3C)   hint = "  <- OV5640 센서";
                printf("  0x%02x 응답%s\n", addr, hint);
                found++;
            }
        }
        if (!found) {
            printf("  응답 없음.\n"
                   "  확인: 4.7k 풀업이 3.3V 에 붙어 있는지, GND 공통인지,\n"
                   "        모듈 전원(AF-VCC)이 들어갔는지, SDA/SCL 이 바뀌지 않았는지\n");
            status = 1;
        } else if (found > 8) {
            /* 실제 버스에 장치가 이렇게 많을 리 없다. SDA 가 LOW 에 붙어 있으면
             * 모든 주소가 ACK 로 읽힌다. */
            printf("\n  ** %d 개는 실제 장치가 아닙니다. SDA 가 LOW 에 붙잡힌 상태입니다. **\n"
                   "  모듈이 선을 물고 있습니다. 확인할 것:\n"
                   "    - 센서의 RESET / PWDN 이 떠 있지 않은지 (RESET=HIGH, PWDN=GND)\n"
                   "    - 센서가 XCLK 없이 동작하지 못하는 것은 아닌지\n"
                   "    - VCM 만 쓸 거라면 센서 전원과 제어선을 아예 빼는 편이 낫습니다\n",
                   found);
            status = 1;
        }
    }
    else if (!strcmp(command, "ping")) {
        int ack = gpio_i2c_ping(bus, DW9714_ADDR);
        if (ack == 1) {
            printf("DW9714(0x%02x) 응답 있음\n", DW9714_ADDR);
        } else {
            printf("DW9714(0x%02x) 응답 없음 — 코일 직결 모듈이거나 배선 문제입니다\n",
                   DW9714_ADDR);
            status = 1;
        }
    }
    else if (!strcmp(command, "set") || !strcmp(command, "save")) {
        int position = -1;
        for (int i = 2; i < argc; i++) {
            if (argv[i][0] != '-') { position = atoi(argv[i]); break; }
        }
        if (position < 0) {
            fprintf(stderr, "위치를 지정하세요 (0~1023)\n");
            status = 1;
        } else if (dw9714_set_from_below(bus, position, backlash, settle_ms) < 0) {
            fprintf(stderr, "이동 실패: %s\n", gpio_i2c_strerror(bus));
            status = 1;
        } else {
            printf("위치 %d 로 이동했습니다\n", position);
            /* 콘으로 거리가 고정된 장비에서는 이 수동 보정 한 번이면 충분하다.
             * 카메라를 열지 않으므로 스트리밍을 멈출 필요가 없다. */
            if (!strcmp(command, "save") && save_focus(position) < 0)
                status = 1;
        }
    }
    else if (!strcmp(command, "sweep")) {
        printf("%d 에서 %d 까지 %d 간격으로 훑습니다. 렌즈가 움직이는지 보세요.\n",
               cfg.min_pos, cfg.max_pos, sweep_step);
        for (int position = cfg.min_pos; position <= cfg.max_pos; position += sweep_step) {
            if (dw9714_set(bus, position) < 0) {
                fprintf(stderr, "이동 실패: %s\n", gpio_i2c_strerror(bus));
                status = 1;
                break;
            }
            printf("  위치 %d\n", position);
            fflush(stdout);
            msleep(300);
        }
        if (!status)
            dw9714_set_from_below(bus, cfg.min_pos, backlash, settle_ms);
    }
    else if (!strcmp(command, "focus")) {
        int start = load_focus();
        if (start < 0)
            start = (cfg.min_pos + cfg.max_pos) / 2;
        /* --step 을 안 주면 눈으로 차이가 보일 만큼만 움직인다. */
        int focus_step = have_step ? sweep_step : 20;
        if (focus_interactive(bus, start, focus_step, cfg.min_pos, cfg.max_pos,
                              backlash, settle_ms) < 0)
            status = 1;
    }
    else if (!strcmp(command, "restore")) {
        int position = load_focus();
        if (position < 0) {
            fprintf(stderr, "저장된 초점값이 없습니다 (먼저 af --save 를 돌리세요)\n");
            status = 1;
        } else if (dw9714_set_from_below(bus, position, backlash, settle_ms) < 0) {
            fprintf(stderr, "이동 실패: %s\n", gpio_i2c_strerror(bus));
            status = 1;
        } else {
            printf("저장된 초점 %d 로 복귀했습니다\n", position);
        }
    }
    else if (!strcmp(command, "score") || !strcmp(command, "af")) {
        char err[256] = {0};
        v4l2_cap_t *cap = v4l2_cap_open(video, 0, 0, err, sizeof(err));
        if (!cap) {
            fprintf(stderr, "카메라 열기 실패: %s\n", err);
            gpio_i2c_close(bus);
            return 1;
        }
        v4l2_cap_warmup(cap, 5);

        af_context_t af = { bus, cap, roi_percent, settle_ms, backlash, verbose };

        if (!strcmp(command, "score")) {
            int width = 0, height = 0, stride = 0;
            const uint8_t *luma = v4l2_cap_grab(cap, &width, &height, &stride);
            if (!luma) {
                fprintf(stderr, "프레임을 받지 못했습니다\n");
                status = 1;
            } else {
                int rx, ry, rw, rh;
                focus_score_center_roi(width, height, roi_percent / 100.0, &rx, &ry, &rw, &rh);
                focus_score_t s = focus_score_tenengrad(luma, stride, width, height,
                                                        rx, ry, rw, rh);
                printf("프레임 %dx%d (stride %d), ROI %dx%d @ (%d,%d)\n",
                       width, height, stride, rw, rh, rx, ry);
                printf("  Tenengrad  %.2f\n", s.tenengrad);
                printf("  평균 밝기  %.1f\n", s.mean_luma);
                printf("  정규화     %.4f\n", s.normalized);
            }
        } else {
            printf("오토포커스: %d~%d, 거친 %d / 미세 %d\n",
                   cfg.min_pos, cfg.max_pos, cfg.coarse_step, cfg.fine_step);
            af_result_t result = af_search(&cfg, measure_at, &af);
            printf("\n결과: %s\n", af_status_str(result.status));
            printf("  최적 위치  %d\n", result.best_pos);
            printf("  선명도     %.4f (최저 %.4f, 비 %.2f배)\n",
                   result.best_score, result.worst_score,
                   result.worst_score > 0 ? result.best_score / result.worst_score : 0.0);
            printf("  측정 횟수  %d\n", result.measurements);

            if (result.status == AF_OK || result.status == AF_WARN_AT_EDGE) {
                dw9714_set_from_below(bus, result.best_pos, backlash, settle_ms);
                if (do_save)
                    save_focus(result.best_pos);
            } else {
                status = 1;
            }
        }
        v4l2_cap_close(cap);
    }

    gpio_i2c_close(bus);
    return status;
}
