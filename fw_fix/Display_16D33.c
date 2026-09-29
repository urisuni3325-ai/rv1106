

#include <string.h>

extern void HT16D33_WriteCmd(BYTE cmd);
extern void HT16D33_WriteRamAll(BYTE *pRam);
extern void HT16D33_WriteCmdParam(BYTE cmd, BYTE param) ;

void HT16D33_SetPixelPWM(BYTE com, BYTE row, BYTE level);
void HT16D33_Update(void);
void Set_LED_Brightness_Ratio(uint8_t ratio);
extern void HT16D33_WriteLedCtrlAll(BYTE val);
extern void HT16D33_WriteLedCtrlOne(void);

BYTE	P100    = 255;
BYTE	P20	   =  0  ;  // 5
BYTE	P20_B =  0  ;  //	15

#define DIGIT_LEVEL   P100    
/*
#define	P100	255
#define	P20		0 // 5
#define	P20_B	0 //	15

*/

//기본 RGB
BYTE WB_R =  255 ;//150;   // 시작값 = 기존 BASE_WHITE 비율
BYTE WB_G = 255;   // 가장 어두워 보이는 채널을 255로 고정
BYTE WB_B = 255 ; // 200;

#define BASE_WHITE_R      165 // 150
#define BASE_WHITE_G      255 // 255
#define BASE_WHITE_B      80 //150
// 화이트
//#define BASE_WHITE_R      150 // 150
//#define BASE_WHITE_G      255
//#define BASE_WHITE_B      200 // 120

// 연한 적색 100% Base
#define BASE_RED_50P_R         200//255
#define BASE_RED_50P_G         40
#define BASE_RED_50P_B         0

// 밝은 하늘색 100% Base
#define BASE_SKY_BLUE_R        20
#define BASE_SKY_BLUE_G        200
#define BASE_SKY_BLUE_B        200 //255


// 자홍색 (알칼리 강)
#define BASE_MAGENTA_R     165 // 255
#define BASE_MAGENTA_G     0
#define BASE_MAGENTA_B     80 //255

// 블루 (알칼리 중)
#define BASE_BLUE_R        			0
#define BASE_BLUE_G        			0
#define BASE_BLUE_B        		255

// 약 그린 (알칼리 약)
#define BASE_LIGHT_GREEN_R  30
#define BASE_LIGHT_GREEN_G  255
#define BASE_LIGHT_GREEN_B  20

	

// 주황  (온수 중)
#define BASE_ORANGE_R      255 // 165 //255
#define BASE_ORANGE_G      70 //130 //70
#define BASE_ORANGE_B      0

// 노랑  (온수 약)
#define BASE_YELLOW_R      255 //165 //255
#define BASE_YELLOW_G      200 //255 //200
#define BASE_YELLOW_B      0

//그린
#define BASE_GREEN_R   0
#define BASE_GREEN_G  255
#define BASE_GREEN_B   0

// 적색
#define BASE_RED_R         255
#define BASE_RED_G         0
#define BASE_RED_B         0




// [20% 그룹 변수]
BYTE COLOR_RED_20P_R, COLOR_RED_20P_G, COLOR_RED_20P_B;
BYTE COLOR_LIGHT_BLUE_20_R, COLOR_LIGHT_BLUE_20_G, COLOR_LIGHT_BLUE_20_B;

// [100% 그룹 변수]
BYTE COLOR_RED_50P_R, COLOR_RED_50P_G, COLOR_RED_50P_B;
BYTE COLOR_SKY_BLUE_R, COLOR_SKY_BLUE_G, COLOR_SKY_BLUE_B;

BYTE COLOR_MAGENTA_R, COLOR_MAGENTA_G, COLOR_MAGENTA_B;
BYTE COLOR_BLUE_R, COLOR_BLUE_G, COLOR_BLUE_B;
BYTE COLOR_LIGHT_GREEN_R, COLOR_LIGHT_GREEN_G, COLOR_LIGHT_GREEN_B;

BYTE COLOR_ORANGE_R, COLOR_ORANGE_G, COLOR_ORANGE_B;
BYTE COLOR_YELLOW_R, COLOR_YELLOW_G, COLOR_YELLOW_B;

BYTE COLOR_GREEN_R, COLOR_GREEN_G, COLOR_GREEN_B;
BYTE COLOR_RED_R, COLOR_RED_G, COLOR_RED_B;
BYTE COLOR_WHITE_R, COLOR_WHITE_G, COLOR_WHITE_B;

#define HT16D33_CMD_SOFT_RESET      0xCCu
#define HT16D33_CMD_SYSTEM_CTRL     0x35u
#define HT16D33_CMD_CONFIG_MODE     0x31u
#define HT16D33_CMD_CURRENT_RATIO   0x36u
#define HT16D33_CMD_GLOBAL_BRIGHT   0x37u
#define HT16D33_CMD_RAM_PAGE_ADDR   0xFDu
#define HT16D33_CMD_WRITE_RAM       0x80u
#define HT16D33_CMD_WRITE_LEDCTRL   0x84u

// 12x12 Gray 모드: 도트당 1바이트, addr=com*16+row, 최대 0xBB -> 188바이트
#define HT16D33_RAM_SIZE            188 

static BYTE g_ht16_ram[HT16D33_RAM_SIZE]; 
static BYTE g_prev[HT16D33_RAM_SIZE];
void DISP_SetDigitRaw(BYTE digit, BYTE seg_mask);
	 
void Display_Number(void);	 
void Display_clean(void);
void Display_Set(void);
void Display_Uv(void);
void Display_CorkLed(void);;
void Display_mL(void);
void Display_Ph_Temp_L(void);
void Display_Pure(void);
void Display_Cool(void);
void Display_Alkali(void);
void Display_AlkaliCool(void);
void Display_Hot(void);
void Display_FilterChange(void);
void Display_Flushing(void);
void Display_Run(void);
void Display_CleanBtn(void);
void Display_Ready(void);

extern BYTE con_brightness;
extern BYTE con_deactivate;

extern int32_t hi_temp,ho_temp;
extern int32_t avg_hi_temp ;

extern int32_t ho_temp;
extern int32_t cool_temp;
extern ULONG filter1_life;       		// 6000.0
extern ULONG filter2_life;
 
extern ULONG flow_liter;
extern BYTE 	m_state;
 
extern WORD s_mode;
extern BYTE 	ion_state,flicker_cnt;
extern BYTE volume_level,ph_disp_fg;
extern BYTE filter_change_fg,cleaning_fg,clean_speed_cnt,clean_cnt;
extern BYTE filter_change_en;

extern 	BYTE key_num ;
extern BYTE	orp_tbl[9],lang_level; 
extern BYTE auto_clean_value;
extern BYTE alkali_ph_step_disp[3];
extern BYTE alkali_ph_step[3];
extern const WORD	pi_ref_tbl[57] ;
extern ULONG disp_ion_i;
extern BYTE bSet_mL;
extern WORD bDisp_mL[3];

//온도
extern WORD wSetHotTemperDisp[3];	
extern BYTE bSetHotTemper;
extern BYTE bWaterMode;

extern BYTE bCoolTempStatus;
	
extern ULONG	flow_pulse_cnt;  

extern ULONG flow_out_liter;
extern ULONG  lTargetFlow;
extern ULONG lFlowSum;
extern long lPump_rpm;

extern ULONG flow_hot_liter;
extern int32_t wCoolTemper;
extern ULONG lFlowSum_Out;
extern uint32_t lError;

extern BYTE 	backlight_en_fg ;
extern WORD set_uv_stop_time,set_uv_run_time,set_uv_cork_sec_time;
extern BYTE  set_uv_cork_onoff;
extern uint32_t 	eep_data;

extern BYTE	flow_error;
extern ULONG ion_i;
extern ULONG lAdc_Ave[8],avg_current;

extern WORD 	pwm_value;
extern ULONG disp_flow_hot ;
extern ULONG disp_flow ;
extern BYTE bSet_mL_Old;
extern ULONG flow_temp ,saved_flow_pulse ;
extern WORD timer_sec1;
extern ULONG flow_out_temp ;
extern BYTE bUvcCoolFg;
extern ULONG disp_flow_out ;
extern BYTE bHotLockFg;
extern BYTE filter_change_en;
extern BYTE con_hot_over_temp;
extern BYTE con_hot_over_sec;
extern BYTE con_touch_sens;
extern BYTE con_deact_show;
extern BYTE bCoolReadyFg ;
extern BYTE bFlushingStep;
extern BYTE bManualCleanFg;
extern BYTE bSetIntroFg;
BYTE bSet_mL_FlickerOn=0;
BYTE bSet_mL_Flicker_n = 0;
BYTE bSet_mL_FlickerCnt=0;
BYTE bSet_mL_FlickerFg=0;
BYTE bSet_mL_FlickerNum=0;

WORD wBlink_timer = 0;  
uint8_t  bLast_s_mode = 0;   
 uint8_t bBlinkingFg=0;
uint8_t bFlikerFg = 0;
uint8_t bBlink_OnTimer = 0;

BYTE bActive_errors[18];
BYTE bErr_count = 0;
WORD wErr_rotate_timer = 0;
BYTE bErr_disp_idx = 0;
BYTE bFlushHotLedStep = 0;   // 온수 온도 LED 순차 점등 인덱스 (0=약 1=중 2=강)
WORD wFlushHotLedCnt  = 0;
BYTE bSetIntroStep = 0;      // 설정모드 진입  단계 (0~7 : 4항목 x 2회)
WORD wSetIntroCnt  = 0;
/* =========================================================
   7-seg mapping
   ROW0=A, ROW1=B, ROW2=C, ROW3=D, ROW4=E, ROW5=F, ROW6=G, ROW7=DP
   ========================================================= */
#define SEG_A   (1u << 0)
#define SEG_B   (1u << 1)
#define SEG_C   (1u << 2)
#define SEG_D   (1u << 3)
#define SEG_E   (1u << 4)
#define SEG_F   (1u << 5)
#define SEG_G   (1u << 6)
#define SEG_DP  (1u << 7)

#define FONT_MINUS  10
#define FONT_BLANK  11
#define FONT_E      12
#define FONT_L      13
#define FONT_MLC1     14
#define FONT_MLC2      15
#define FONT_MLC3    16
#define FONT_d    		17
#define FONT_o    18
#define FONT_n    19
#define FONT_F    20
#define FONT_U    21


BYTE ml_rotate_phase = 0;
BYTE ml_rotate_cnt   = 0;

typedef struct
{
    BYTE digit;
    BYTE seg;
} ML_ROT_STEP;

static const ML_ROT_STEP g_ml_ring_tbl[] =
{
    {3, SEG_A},
    {4, SEG_A},
    {5, SEG_A},
    {5, SEG_B},
    {5, SEG_C},
    {5, SEG_D},
    {4, SEG_D},
    {3, SEG_D},
    {3, SEG_E},
    {3, SEG_F}
};
#define ML_RING_STEP_MAX   (sizeof(g_ml_ring_tbl)/sizeof(g_ml_ring_tbl[0]))



typedef struct { BYTE com, row; } SegPos;

static const SegPos seg7_1[7] = {
    /* a  D16 */ { 1, 7 },
    /* b  D17 */ { 1, 8 },
    /* c  D38 */ { 4, 5 },
    /* d  D39 */ { 4, 6 },
    /* e  D36 */ { 3, 10 },
    /* f  D15  */ { 1, 6 },
    /* g  D37 */ { 3, 11 },
};
static const SegPos seg7_2[7] = {
    /* a  D19 */ { 1, 10 },
    /* b  D20 */ { 1, 11 },
    /* c  D42 */ { 4,  9 },
    /* d  D43 */ { 4, 10 },
    /* e  D40 */ { 4,  7 },
    /* f   D18 */ {  1, 9 },
    /* g  D41 */ { 4,  8 },
};
static const SegPos seg7_3[7] = {
    /* a  D22 */ { 2,  3 },
    /* b  D23 */ { 2,  4 },
    /* c  D46 */ { 5,  6 },
    /* d  D48 */ { 5,  8 },
    /* e  D44 */ { 4, 11 },
    /* f   D21 */ { 2,  2 },
    /* g  D45 */ { 5,  5},
};
static const SegPos seg7_4[7] = {
    /* a  D26 */ { 2,  7 },
    /* b  D27 */ { 2,  8 },
    /* c  D51 */ { 5, 11 },
    /* d  D52 */ { 6,  7 },
    /* e  D49 */ { 5,  9 },
    /* f   D25 */ { 2,  6 },
    /* g  D50 */ { 5, 10},
};
static const SegPos seg7_5[7] = {
    /* a  D29 */ { 2,  10 },
    /* b  D30 */ { 2,  11 },
    /* c  D55 */ { 6,  10 },
    /* d  D56 */ { 6,  11 },
    /* e  D53 */ { 6,  8 },
    /* f   D28 */ { 2,  9 },
    /* g  D54 */ { 6,  9},
};
static const SegPos seg7_6[7] = {
    /* a  D32 */ { 3,  6 },
    /* b  D33 */ { 3,  7 },
    /* c  D59 */ { 7,  9 },
    /* d  D60 */ { 7,  10 },
    /* e  D57 */ { 7,  7 },
    /* f   D31 */ { 3,  5 },
    /* g  D58 */ { 7,  8},
};
// 0~9 폰트: bit0=a, bit1=b, ... bit6=g 
static const BYTE font7[22] = {
    0x3F, // 0 : a b c d e f
    0x06, // 1 : b c
    0x5B, // 2 : a b d e g
    0x4F, // 3 : a b c d g
    0x66, // 4 : b c f g
    0x6D, // 5 : a c d f g
    0x7D, // 6 : a c d e f g
    0x07, // 7 : a b c
    0x7F, // 8 : a b c d e f g
    0x6F, // 9 : a b c d f g
	/* - */ 0x40,
    /* blank */ 0x00, //0x00
	/* E */ 0x79,
    /* L */ 0x38,
	/* ㄷ */ 0x39,
	/* ㅡ */ 0x09,
	/* -ㅣㅡ */ 0x0f,
	/*d*/ 0x5e,
	/*o*/ 0x5c,
	/*n*/ 0x54,
	/*F*/ 0x71,
	/*U*/ 0x3e
};
#define FONT_MAX   (BYTE)(sizeof(font7) / sizeof(font7[0]))
	
/* base(0~255) x wb(0~255) x pct(0~100) → 0~255, 포화 처리 */
static BYTE Mix(BYTE base, BYTE wb, uint16_t pct)
{
    uint32_t v = (uint32_t)base * (uint32_t)wb * (uint32_t)pct;  /* 최대 6,502,500 */
    v = (v + (255u * 100u) / 2u) / (255u * 100u);                /* 반올림 */
    if (v > 255u) v = 255u;
    return (BYTE)v;
}


// 숫자 value(0~9)를 밝기 level(0~255)로 표시.
void Disp_Digit_1(BYTE value, BYTE level)
{
    BYTE i, mask;
    if (value > 9) return;
    mask = font7[value];
    for (i = 0; i < 7; i++) {
        HT16D33_SetPixelPWM(seg7_1[i].com, seg7_1[i].row, (mask & (1u << i)) ? level : 0);                   
    }
}
void Disp_Digit_2(BYTE value, BYTE level)
{
    BYTE i, mask;
    if (value > 9) return;
    mask = font7[value];
    for (i = 0; i < 7; i++) {
        HT16D33_SetPixelPWM(seg7_2[i].com, seg7_2[i].row, (mask & (1u << i)) ? level : 0);                   
    }
}
void Disp_Digit_3(BYTE value, BYTE level)
{
    BYTE i, mask;
    if (value > 9) return;
    mask = font7[value];
    for (i = 0; i < 7; i++) {
        HT16D33_SetPixelPWM(seg7_3[i].com, seg7_3[i].row, (mask & (1u << i)) ? level : 0);                   
    }
}
void Disp_Digit_4(BYTE value, BYTE level)
{
    BYTE i, mask;
    if (value > 9) return;
    mask = font7[value];
    for (i = 0; i < 7; i++) {
        HT16D33_SetPixelPWM(seg7_4[i].com, seg7_4[i].row, (mask & (1u << i)) ? level : 0);                   
    }
}
void Disp_Digit_5(BYTE value, BYTE level)
{
    BYTE i, mask;
    if (value > 9) return;
    mask = font7[value];
    for (i = 0; i < 7; i++) {
        HT16D33_SetPixelPWM(seg7_5[i].com, seg7_5[i].row, (mask & (1u << i)) ? level : 0);                   
    }
}
void Disp_Digit_6(BYTE value, BYTE level)
{
    BYTE i, mask;
    if (value > 9) return;
    mask = font7[value];
    for (i = 0; i < 7; i++) {
        HT16D33_SetPixelPWM(seg7_6[i].com, seg7_6[i].row, (mask & (1u << i)) ? level : 0);                   
    }
}





static const SegPos * DISP_SegTable(BYTE digit)
{
    switch (digit) {
        case 0: return seg7_1;
        case 1: return seg7_2;
        case 2: return seg7_3;
        case 3: return seg7_4;
        case 4: return seg7_5;
        case 5: return seg7_6;
        default: return seg7_1;
    }
}

//#define DIGIT_LEVEL   0xFF       

// seg_mask: bit0=a … bit6=g (DP는 g_seg_font에 bit7로 들어옴) 
void DISP_SetDigitRaw(BYTE digit, BYTE seg_mask)
{
    const SegPos *seg = DISP_SegTable(digit);
    BYTE i;
    if (digit >= 6) return;

    for (i = 0; i < 7; i++) {       /* a~g */
        HT16D33_SetPixelPWM(seg[i].com, seg[i].row,(seg_mask & (1u << i)) ? DIGIT_LEVEL : 0);
                            
    }

}

void DISP_NumDot(BYTE on)
{
	BYTE level = on ? P100 : P20;
    HT16D33_SetPixelPWM(5, 7, level);   // com5-row7

}

void DISP_SetDigitNum(BYTE digit, BYTE value, BYTE dp_on)
{
    BYTE seg;
	
    if (digit >= 6) return;

   /* if(value <= 9)            seg = font7[value];
    else if(value == FONT_MINUS)   seg = font7[FONT_MINUS];
    else if(value == FONT_E)       	 seg = font7[FONT_E];
    else if(value == FONT_L)       	 seg = font7[FONT_L];
    else if(value == FONT_MLC1)     seg = font7[FONT_MLC1];
    else if(value == FONT_MLC2)     seg = font7[FONT_MLC2];
    else if(value == FONT_MLC3)     seg = font7[FONT_MLC3];
    else                            seg = font7[FONT_BLANK];
*/
	if (value < FONT_MAX)  seg = font7[value];
    else                   seg = font7[FONT_BLANK];
	
    if (dp_on) 	DISP_NumDot(1);
   // else       		seg &= (BYTE)(~SEG_DP);

    DISP_SetDigitRaw(digit, seg);
}



void DISP_ShowMlRingRotate(void)
{
    DISP_SetDigitRaw(3, 0x00);
    DISP_SetDigitRaw(4, 0x00);
    DISP_SetDigitRaw(5, 0x00);

    DISP_SetDigitRaw(g_ml_ring_tbl[ml_rotate_phase].digit,g_ml_ring_tbl[ml_rotate_phase].seg);
     
}

void DISP_UpdateMlRingRotate(void)
{
    if (++ml_rotate_cnt >= 13)
    {
        ml_rotate_cnt = 0;
        ml_rotate_phase++;
        if (ml_rotate_phase >= ML_RING_STEP_MAX)
            ml_rotate_phase = 0;
    }
}

/* =========================================================
   HT16D33 low-level  ―  12×12 GRAY(그레이스케일) mode
   ---------------------------------------------------------
   [설정] Configuration Mode(0x31) 파라미터 = 0x02
          BGS=0(Gray) , MT1:MT0=10 → Type2 12×12
   [DDRAM] 도트마다 1바이트(0~255 PWM). (데이터시트 p.24 Gray 12×12 표)
              addr = com*16 + row       (com 0~11, row 0~11)
              값   = 0(꺼짐) ~ 255(100%)
           최대 주소 0xBB → 188바이트 (반드시 188바이트 전송, limit 0xBB).
   [필수] Gray 모드는 'LED On/Off Control RAM'(0x84)이 유효하므로
          init 에서 반드시 전부 0xFF(마스크 해제) 로 채워야 표시됨.
   ========================================================= */

/* 도트별 임의 밝기: level 0(꺼짐) ~ 255(100%)  ← 그라데이션/연출용 */
void HT16D33_SetPixelPWM(BYTE com, BYTE row, BYTE level)
{
    if (com > 11 || row > 11) return;
    g_ht16_ram[com * 16 + row] = level;
}
BYTE HT16D33_GetPixelPWM(BYTE com, BYTE row)
{
    if (com > 11 || row > 11) return 0;
    return g_ht16_ram[com * 16 + row];
}

void HT16D33_SetBrightness(BYTE level_0_to_255)
{
    // HT16D33은 256단계(0x00~0xFF) PWM 밝기 조절
    HT16D33_WriteCmdParam(HT16D33_CMD_GLOBAL_BRIGHT, level_0_to_255);
}

// 0~100% 전체 LED 밝기를 조절하는 함수
void Set_LED_Brightness_Ratio(uint8_t ratio)
{
	BYTE level_0_to_255;
	
    if (ratio > 100) ratio = 100; // 최대치 제한
    
    // 0~100%를 0~255 레벨로 변환
    level_0_to_255 = (BYTE)((255 * ratio) / 100);
    
    // 이미 구현되어 있는 함수 호출
    HT16D33_SetBrightness(level_0_to_255);
}
void Update_Color(BYTE new_p100, BYTE new_p20)
{
	
	 uint16_t pct100, pct20;

    /* con_brightness / con_deactivate 는 0~100(%) 로 통일 */
    pct100 = (new_p100 > 100u) ? 100u : new_p100;
    pct20  = (new_p20  > 100u) ? 100u : new_p20;

    /* 모노(단색) LED 용 PWM 레벨 : 100% 일 때 255 가 되도록 환산 */
    P100  = (BYTE)((255u * pct100) / 100u);
    P20   = (BYTE)((255u * pct20 ) / 100u);
    P20_B = P20;

    /* ---- 활성(100%) 그룹 ---- */
    COLOR_WHITE_R       = Mix(BASE_WHITE_R,       WB_R, pct100);
    COLOR_WHITE_G       = Mix(BASE_WHITE_G,       WB_G, pct100);
    COLOR_WHITE_B       = Mix(BASE_WHITE_B,       WB_B, pct100);

    COLOR_RED_R         = Mix(BASE_RED_R,         WB_R, pct100);
    COLOR_RED_G         = Mix(BASE_RED_G,         WB_G, pct100);
    COLOR_RED_B         = Mix(BASE_RED_B,         WB_B, pct100);

    COLOR_GREEN_R       = Mix(BASE_GREEN_R,       WB_R, pct100);
    COLOR_GREEN_G       = Mix(BASE_GREEN_G,       WB_G, pct100);
    COLOR_GREEN_B       = Mix(BASE_GREEN_B,       WB_B, pct100);

    COLOR_BLUE_R        = Mix(BASE_BLUE_R,        WB_R, pct100);
    COLOR_BLUE_G        = Mix(BASE_BLUE_G,        WB_G, pct100);
    COLOR_BLUE_B        = Mix(BASE_BLUE_B,        WB_B, pct100);

    COLOR_MAGENTA_R     = Mix(BASE_MAGENTA_R,     WB_R, pct100);
    COLOR_MAGENTA_G     = Mix(BASE_MAGENTA_G,     WB_G, pct100);
    COLOR_MAGENTA_B     = Mix(BASE_MAGENTA_B,     WB_B, pct100);

    COLOR_LIGHT_GREEN_R = Mix(BASE_LIGHT_GREEN_R, WB_R, pct100);
    COLOR_LIGHT_GREEN_G = Mix(BASE_LIGHT_GREEN_G, WB_G, pct100);
    COLOR_LIGHT_GREEN_B = Mix(BASE_LIGHT_GREEN_B, WB_B, pct100);

    COLOR_ORANGE_R      = Mix(BASE_ORANGE_R,      WB_R, pct100);
    COLOR_ORANGE_G      = Mix(BASE_ORANGE_G,      WB_G, pct100);
    COLOR_ORANGE_B      = Mix(BASE_ORANGE_B,      WB_B, pct100);

    COLOR_YELLOW_R      = Mix(BASE_YELLOW_R,      WB_R, pct100);
    COLOR_YELLOW_G      = Mix(BASE_YELLOW_G,      WB_G, pct100);
    COLOR_YELLOW_B      = Mix(BASE_YELLOW_B,      WB_B, pct100);

    COLOR_RED_50P_R     = Mix(BASE_RED_50P_R,     WB_R, pct100);
    COLOR_RED_50P_G     = Mix(BASE_RED_50P_G,     WB_G, pct100);
    COLOR_RED_50P_B     = Mix(BASE_RED_50P_B,     WB_B, pct100);

    COLOR_SKY_BLUE_R    = Mix(BASE_SKY_BLUE_R,    WB_R, pct100);
    COLOR_SKY_BLUE_G    = Mix(BASE_SKY_BLUE_G,    WB_G, pct100);
    COLOR_SKY_BLUE_B    = Mix(BASE_SKY_BLUE_B,    WB_B, pct100);

    /* ---- 비활성(20%) 그룹 : 같은 색을 pct20 로 낮추기만 함 ----
       (기존 BASE_*_20P 상수는 적색이 4%, 청색이 8% 로 제각각이라 폐기 권장) */
    COLOR_RED_20P_R       = Mix(BASE_RED_50P_R,  WB_R, pct20);
    COLOR_RED_20P_G       = Mix(BASE_RED_50P_G,  WB_G, pct20);
    COLOR_RED_20P_B       = Mix(BASE_RED_50P_B,  WB_B, pct20);

    COLOR_LIGHT_BLUE_20_R = Mix(BASE_SKY_BLUE_R, WB_R, pct20);
    COLOR_LIGHT_BLUE_20_G = Mix(BASE_SKY_BLUE_G, WB_G, pct20);
    COLOR_LIGHT_BLUE_20_B = Mix(BASE_SKY_BLUE_B, WB_B, pct20);
	
    // 밝기 변수 갱신
   /* P100 = new_p100;
    P20  = new_p20;
	P20_B = new_p20;
	
    // --------------------------------------------------
    // [20% 그룹 색상 연산] - 수식: (BASE * P20) / 100
    // --------------------------------------------------
    COLOR_RED_20P_R         = (BASE_RED_20P_R * P20) / 100;
    COLOR_RED_20P_G         = (BASE_RED_20P_G * P20) / 100;
    COLOR_RED_20P_B         = (BASE_RED_20P_B * P20) / 100;

    COLOR_LIGHT_BLUE_20_R   = (BASE_LIGHT_BLUE_20_R * P20) / 100;
    COLOR_LIGHT_BLUE_20_G   = (BASE_LIGHT_BLUE_20_G * P20) / 100;
    COLOR_LIGHT_BLUE_20_B   = (BASE_LIGHT_BLUE_20_B * P20) / 100;


    // --------------------------------------------------
    // [100% 그룹 색상 연산] - 수식: (BASE * P100) / 100
    // --------------------------------------------------
    COLOR_RED_50P_R         = (BASE_RED_50P_R * P100) / 100;
    COLOR_RED_50P_G         = (BASE_RED_50P_G * P100) / 100;
    COLOR_RED_50P_B         = (BASE_RED_50P_B * P100) / 100;

    COLOR_SKY_BLUE_R        = (BASE_SKY_BLUE_R * P100) / 100;
    COLOR_SKY_BLUE_G        = (BASE_SKY_BLUE_G * P100) / 100;
    COLOR_SKY_BLUE_B        = (BASE_SKY_BLUE_B * P100) / 100;

    COLOR_MAGENTA_R         = (BASE_MAGENTA_R * P100) / 100;
    COLOR_MAGENTA_G         = (BASE_MAGENTA_G * P100) / 100;
    COLOR_MAGENTA_B         = (BASE_MAGENTA_B * P100) / 100;

    COLOR_BLUE_R            = (BASE_BLUE_R * P100) / 100;
    COLOR_BLUE_G            = (BASE_BLUE_G * P100) / 100;
    COLOR_BLUE_B            = (BASE_BLUE_B * P100) / 100;

    COLOR_LIGHT_GREEN_R     = (BASE_LIGHT_GREEN_R * P100) / 100;
    COLOR_LIGHT_GREEN_G     = (BASE_LIGHT_GREEN_G * P100) / 100;
    COLOR_LIGHT_GREEN_B     = (BASE_LIGHT_GREEN_B * P100) / 100;

    COLOR_ORANGE_R          = (BASE_ORANGE_R * P100) / 100;
    COLOR_ORANGE_G          = (BASE_ORANGE_G * P100) / 100;
    COLOR_ORANGE_B          = (BASE_ORANGE_B * P100) / 100;

    COLOR_YELLOW_R          = (BASE_YELLOW_R * P100) / 100;
    COLOR_YELLOW_G          = (BASE_YELLOW_G * P100) / 100;
    COLOR_YELLOW_B          = (BASE_YELLOW_B * P100) / 100;

    COLOR_GREEN_R           = (BASE_GREEN_R * P100) / 100;
    COLOR_GREEN_G           = (BASE_GREEN_G * P100) / 100;
    COLOR_GREEN_B           = (BASE_GREEN_B * P100) / 100;

    COLOR_RED_R             = (BASE_RED_R * P100) / 100;
    COLOR_RED_G             = (BASE_RED_G * P100) / 100;
    COLOR_RED_B             = (BASE_RED_B * P100) / 100;

    COLOR_WHITE_R           = (BASE_WHITE_R * P100) / 100;
    COLOR_WHITE_G           = (BASE_WHITE_G * P100) / 100;
    COLOR_WHITE_B           = (BASE_WHITE_B * P100) / 100;*/
}


void HT16D33_Init(void)
{
    delay_1ms(5);
    HT16D33_WriteCmd(HT16D33_CMD_SOFT_RESET);
    delay_1ms(5);

    // 1. Gray + 12x12 (BGS=0 Gray, MT=10 Type2)
    HT16D33_WriteCmdParam(HT16D33_CMD_CONFIG_MODE,   0x02);
    HT16D33_WriteCmdParam(HT16D33_CMD_CURRENT_RATIO, 0x08); // 0x0f->0x0a   , 0x08=27mA, 0x05=18mA)  전류비 0x0a =33mA — 0x0F=48mA) 에서 낮춤
    HT16D33_WriteCmdParam(HT16D33_CMD_GLOBAL_BRIGHT, 0xff); //con_brightness); // con_brightness);  ???
	HT16D33_WriteCmdParam(0x32, 0x0A);   // Fade Function Control: GMEN=0 → Linear
	 HT16D33_WriteCmdParam(0x34, 0x00);   //Cascade: 단일칩 (명시) 
    HT16D33_WriteCmdParam(0x39, 0x88);   // Blanking Voltage 활성 VBEN=1, VB=1111 
	
    // 2. Gray 모드 필수: LED On/Off Control RAM 을 전부 ON(0xFF) 으로 (18바이트 패킹)
    HT16D33_WriteLedCtrlAll(0xFF);

    // 3. 디스플레이 켜기 전에 gray RAM 클리어 (188바이트 0)
     memset(g_ht16_ram, 0xff, sizeof(g_ht16_ram));// memset(g_ht16_ram, 0x00, sizeof(g_ht16_ram));
    HT16D33_Update();

    // 4. 마지막에 OSC + Display ON
    HT16D33_WriteCmdParam(HT16D33_CMD_SYSTEM_CTRL, 0x03);
	
	delay_1ms(100);
	
	
	memset(g_ht16_ram, 0x00, sizeof(g_ht16_ram));
	
	
	HT16D33_WriteLedCtrlAll(0xff);
	//HT16D33_WriteLedCtrlOne();
}



void HT16D33_ClearBuffer(void)
{
    memset(g_ht16_ram, 0, sizeof(g_ht16_ram));
}

void HT16D33_FillBuffer(BYTE value)
{
    memset(g_ht16_ram, value, sizeof(g_ht16_ram));
}

void HT16D33_Update(void)
{
	static BYTE s_shadow[HT16D33_RAM_SIZE];
    static BYTE s_first = 1;
    static BYTE s_refresh_cnt = 0;

    /* 100 tick = 1초마다 한 번은 무조건 전송 (칩 글리치 자동 복구) */
    if (++s_refresh_cnt >= 100) {
        s_refresh_cnt = 0;
        s_first = 1;
    }

    if (!s_first && memcmp(s_shadow, g_ht16_ram, HT16D33_RAM_SIZE) == 0)
        return;

    memcpy(s_shadow, g_ht16_ram, HT16D33_RAM_SIZE);
    s_first = 0;

    HT16D33_WriteRamAll(g_ht16_ram);
}

void DISP_AllOff(void)
{
    HT16D33_ClearBuffer();
    HT16D33_Update();
}

// =========================================================

void fcLed_Clean(uint8_t on)
{
   BYTE level = on ? P100 : P20;
   HT16D33_SetPixelPWM(0, 2, level);   // com0-row2
   HT16D33_SetPixelPWM(0, 3, level);   // com0-row3

}

void fcLed_Uv(uint8_t on)
{
    BYTE level = on ? P100 : P20;
    HT16D33_SetPixelPWM(0, 4,  level);   // com0-row4
	HT16D33_SetPixelPWM(0, 5,  level);   // com0-row5

}

void fcLed_CorkUv(uint8_t on)
{
	
    BYTE level = on ? P100 : P20;

    if (set_uv_cork_onoff == 0) level = 0;            /* 설정 oFF → 완전 소등 */
    else                        level = on ? P100 : P20;
	
	
	//E9
    HT16D33_SetPixelPWM(9 , 0,  level);   // com9-row0
	HT16D33_SetPixelPWM(10, 0,  level);   // com10-row0
	HT16D33_SetPixelPWM(11, 0,  level);   // com11-row0
	
	//E10
    HT16D33_SetPixelPWM(9 , 1,  level);   // com9-1
	HT16D33_SetPixelPWM(10, 1,  level);   // com10-row1
	HT16D33_SetPixelPWM(11, 1,  level);   // com11-row1
	
	//E11
    HT16D33_SetPixelPWM(9 , 2,  level);   // com9-row2
	HT16D33_SetPixelPWM(10, 2,  level);   // com10-row2
	HT16D33_SetPixelPWM(11, 2,  level);   // com11-row2
	
	//E12
    HT16D33_SetPixelPWM(9 , 3,  level);   // com9-row3
	HT16D33_SetPixelPWM(10, 3,  level);   // com10-row0
	HT16D33_SetPixelPWM(11, 3,  level);   // com11-row3
	
	//E13
    HT16D33_SetPixelPWM(9 , 4,  level);   // com9-row4
	HT16D33_SetPixelPWM(10, 4,  level);   // com10-row4
	HT16D33_SetPixelPWM(11, 4,  level);   // com11-row4
	
	//E14
    HT16D33_SetPixelPWM(9 , 5,  level);   // com9-row5
	HT16D33_SetPixelPWM(10, 5,  level);   // com10-row5
	HT16D33_SetPixelPWM(11, 5,  level);   // com11-row5

}

void fcLed_FilterChange(uint8_t on)
{
    BYTE level = on ? P100 : P20;
    HT16D33_SetPixelPWM(0, 6,  level);   // com0-row6
    HT16D33_SetPixelPWM(0, 7,  level);   // com0-row7
}

void fcLed_Filter(uint8_t f)
{
	BYTE level = (BYTE)(((WORD)P100 * 128u) / 255u);   // 기존 128 = P100 의 50%

    if (f >= 1)   HT16D33_SetPixelPWM(0, 8,  level);   // com0-row8
    if (f >= 2)   HT16D33_SetPixelPWM(0, 9,  level);   // com0-row9
    if (f >= 3)   HT16D33_SetPixelPWM(0, 10, level);   // com0-row10
	/*
    if (f == 0) {
       //HT16D33_SetPixelPWM(0, 8, 128);
      // HT16D33_SetPixelPWM(0, 9, 128);
       //HT16D33_SetPixelPWM(0, 10, 128);
    }
    else if (f == 1) {
        HT16D33_SetPixelPWM(0, 8, 128);
        //HT16D33_SetPixelPWM(0, 9, 128);
       // HT16D33_SetPixelPWM(0, 10, 128);
    }
    else if (f == 2) {
        HT16D33_SetPixelPWM(0, 8, 128);
        HT16D33_SetPixelPWM(0, 9, 128);
        //HT16D33_SetPixelPWM(0, 10, 128);
    }
    else if (f == 3) {
        HT16D33_SetPixelPWM(0, 8, 128);
        HT16D33_SetPixelPWM(0, 9, 128);
        HT16D33_SetPixelPWM(0, 10, 128);
    }*/
}

void fcLed_Volume(uint8_t on)
{
	BYTE level = on ? P100 : P20;    
	HT16D33_SetPixelPWM(0, 11, level);  // com0-row11
	HT16D33_SetPixelPWM(1,  2, level);  // com1-row2
	
}

void fcLed_Lang(uint8_t on)
{
	BYTE level = on ? P100 : P20;
    HT16D33_SetPixelPWM(1,  3, level);  // com1-row3
}

void fcLed_Cl(uint8_t on)
{
	BYTE level = on ? P100 : P20;
    HT16D33_SetPixelPWM(1,  4, level);  // com1-row4
}

void fcLed_Ph(uint8_t on)
{
	BYTE level = on ? P100 : P20;
    HT16D33_SetPixelPWM(1,  5, level);  // com1-row5
}

void fcLed_ContinuousPut(uint8_t on)
{
     BYTE level = on ? P100 : P20;
     HT16D33_SetPixelPWM(3, 8, level);       // com3-row8
     HT16D33_SetPixelPWM(3, 9, level);       // com3-row9
    
}

void fcLed_mL(uint8_t on)
{
	BYTE level = on ? P100 : P20;
    HT16D33_SetPixelPWM(7, 11, level);   // com7-row11
}

void fcLed_mL_Btn(uint8_t on)
{
    BYTE level = on ? P100 : P20;
    HT16D33_SetPixelPWM(9,  7,  level);   // com9-row7
    HT16D33_SetPixelPWM(11, 11, level);   // com11-row11 
}

void fcLed_TempUnit(uint8_t on)
{
	BYTE level = on ? P100 : P20;
	HT16D33_SetPixelPWM(2, 5, level);  // com2-row5
}

void fcLed_Lpm(uint8_t on)
{
	BYTE level = on ? P100 : P20;
    HT16D33_SetPixelPWM(9, 10, level);  // com9-row10
}

void fcLed_AlkaliStep(uint8_t s)
{
    if (s == 3) {          // 강: 자홍색
        HT16D33_SetPixelPWM(3, 0, COLOR_MAGENTA_R);
        HT16D33_SetPixelPWM(4, 0, COLOR_MAGENTA_G);
        HT16D33_SetPixelPWM(5, 0, COLOR_MAGENTA_B);
    }
    else if (s == 2) {     // 중: 블루
        HT16D33_SetPixelPWM(3, 1, COLOR_BLUE_R);
        HT16D33_SetPixelPWM(4, 1, COLOR_BLUE_G);
        HT16D33_SetPixelPWM(5, 1, COLOR_BLUE_B);
    }
    else if (s == 1) {     // 약: 약 그린
        HT16D33_SetPixelPWM(3, 2, COLOR_LIGHT_GREEN_R); 
        HT16D33_SetPixelPWM(4, 2, COLOR_LIGHT_GREEN_G); 
        HT16D33_SetPixelPWM(5, 2, COLOR_LIGHT_GREEN_B); 
    }
    else if (s == 0) {
        // 꺼짐 처리 생략
    }
}

void fcLed_HotStep(uint8_t s)
{
    if (s == 2) {          // 강: 적색
        HT16D33_SetPixelPWM(3, 0, COLOR_RED_R);
        HT16D33_SetPixelPWM(4, 0, COLOR_RED_G);
        HT16D33_SetPixelPWM(5, 0, COLOR_RED_B);
    }
    else if (s == 1) {     // 중: 주황
        HT16D33_SetPixelPWM(3, 1, COLOR_ORANGE_R);
        HT16D33_SetPixelPWM(4, 1, COLOR_ORANGE_G);
        HT16D33_SetPixelPWM(5, 1, COLOR_ORANGE_B);   
    }
    else if (s == 0) {     // 약: 노랑
        HT16D33_SetPixelPWM(3, 2, COLOR_YELLOW_R); 
        HT16D33_SetPixelPWM(4, 2, COLOR_YELLOW_G); 
        HT16D33_SetPixelPWM(5, 2, COLOR_YELLOW_B); 
    }
}

void fcLed_AlkaliStep_Run(uint8_t s)
{
    if (s == 3) {          // 강: 자홍색
        HT16D33_SetPixelPWM(6, 3, COLOR_MAGENTA_R);
        HT16D33_SetPixelPWM(7, 3, COLOR_MAGENTA_G);
        HT16D33_SetPixelPWM(8, 3, COLOR_MAGENTA_B);
		
		HT16D33_SetPixelPWM(6, 4, COLOR_MAGENTA_R);
        HT16D33_SetPixelPWM(7, 4, COLOR_MAGENTA_G);
        HT16D33_SetPixelPWM(8, 4, COLOR_MAGENTA_B);
    }
    else if (s == 2) {     // 중: 블루
        HT16D33_SetPixelPWM(6, 3, COLOR_BLUE_R);
        HT16D33_SetPixelPWM(7, 3, COLOR_BLUE_G);
        HT16D33_SetPixelPWM(8, 3, COLOR_BLUE_B);
		
		HT16D33_SetPixelPWM(6, 4, COLOR_BLUE_R);
        HT16D33_SetPixelPWM(7, 4, COLOR_BLUE_G);
        HT16D33_SetPixelPWM(8, 4, COLOR_BLUE_B);
    }
    else if (s == 1) {     // 약: 약 그린
        HT16D33_SetPixelPWM(6, 3, COLOR_LIGHT_GREEN_R); 
        HT16D33_SetPixelPWM(7, 3, COLOR_LIGHT_GREEN_G); 
        HT16D33_SetPixelPWM(8, 3, COLOR_LIGHT_GREEN_B); 
		
		HT16D33_SetPixelPWM(6, 4, COLOR_LIGHT_GREEN_R); 
        HT16D33_SetPixelPWM(7, 4, COLOR_LIGHT_GREEN_G); 
        HT16D33_SetPixelPWM(8, 4, COLOR_LIGHT_GREEN_B); 
    }
    else if (s == 0) {
        // 꺼짐 처리 생략
    }
}

void fcLed_HotStep_Run(uint8_t s)
{
    if (s == 2) {          // 강: 적색
        HT16D33_SetPixelPWM(6, 3, COLOR_RED_R);
        HT16D33_SetPixelPWM(7, 3, COLOR_RED_G);
        HT16D33_SetPixelPWM(8, 3, COLOR_RED_B);
		
		HT16D33_SetPixelPWM(6, 4, COLOR_RED_R);
        HT16D33_SetPixelPWM(7, 4, COLOR_RED_G);
        HT16D33_SetPixelPWM(8, 4, COLOR_RED_B);
    }
    else if (s == 1) {     // 중: 주황
        HT16D33_SetPixelPWM(6, 3, COLOR_ORANGE_R);
        HT16D33_SetPixelPWM(7, 3, COLOR_ORANGE_G);
        HT16D33_SetPixelPWM(8, 3, COLOR_ORANGE_B);   
		
		HT16D33_SetPixelPWM(6, 4, COLOR_ORANGE_R);
        HT16D33_SetPixelPWM(7, 4, COLOR_ORANGE_G);
        HT16D33_SetPixelPWM(8, 4, COLOR_ORANGE_B);   
    }
    else if (s == 0) {     // 약: 노랑
        HT16D33_SetPixelPWM(6, 3, COLOR_YELLOW_R); 
        HT16D33_SetPixelPWM(7, 3, COLOR_YELLOW_G); 
        HT16D33_SetPixelPWM(8, 3, COLOR_YELLOW_B); 
		
		HT16D33_SetPixelPWM(6, 4, COLOR_YELLOW_R); 
        HT16D33_SetPixelPWM(7, 4, COLOR_YELLOW_G); 
        HT16D33_SetPixelPWM(8, 4, COLOR_YELLOW_B); 
    }
}

void fcLed_BtnClean(uint8_t on) //CHAR
{
	BYTE level = on ? P100 : P20;
    HT16D33_SetPixelPWM(9, 8, level);   // com9-row8
	
}

void fcLed_BtnCleanSel(uint8_t on)
{
	BYTE level = on ? P100 : P20;
    HT16D33_SetPixelPWM(8, 7, level);   // com8-row7 ICON
}

void fcLed_Hot(uint8_t on)
{
	BYTE level = on ? P100 : P20;
    HT16D33_SetPixelPWM(10, 6, level);  // com2-row10 (char)
	//HT16D33_SetPixelPWM(10, 7, level);      // com10-row7 (잠금3초 글자)
}

void fcLed_HotSel(uint8_t on)
{
	//BYTE level = on ? P100 : P20;
	
	// 연한  적색 
	if(on){ //100%
		HT16D33_SetPixelPWM(6, 0, COLOR_RED_50P_R);  // com6-row0  R
		HT16D33_SetPixelPWM(7, 0, COLOR_RED_50P_G);    // com7-row0 G
		HT16D33_SetPixelPWM(8, 0, COLOR_RED_50P_B);    // com8-row0 B
	}
	else{ //20%
		HT16D33_SetPixelPWM(6, 0, COLOR_RED_20P_R);  // com6-row0  R
		HT16D33_SetPixelPWM(7, 0, COLOR_RED_20P_G);    // com7-row0 G
		HT16D33_SetPixelPWM(8, 0, COLOR_RED_20P_B);    // com8-row0 B
	}
}

void fcLed_HotDot(uint8_t on)
{
	BYTE level = on ? P100 : P20;
    
	HT16D33_SetPixelPWM(8, 8, level);      // com10-row7 (온수 다트led)
	HT16D33_SetPixelPWM(10, 7, level);      // com10-row7 (잠금3초 글자)
}

void fcLed_Pure(uint8_t on)
{
	BYTE level = on ? P100 : P20;
    HT16D33_SetPixelPWM(10, 8, level);  // com10-row8  CHAR
}

void fcLed_PureSel(uint8_t on)
{
	BYTE level = on ? P100 : P20;
    HT16D33_SetPixelPWM( 8, 9, level);  // com8-row9 ICON
}

void fcLed_Cool(uint8_t on)
{
	BYTE level = on ? P100 : P20;
    HT16D33_SetPixelPWM(11, 6, level);   // com11-row6  CHAR
}

void fcLed_CoolSel(uint8_t on)
{
	//BYTE level = on ? P100 : P20;
	
	if(on){
		//연한 청색 10%
		HT16D33_SetPixelPWM(6, 1,COLOR_SKY_BLUE_R);     // com6-row1  ICON
		HT16D33_SetPixelPWM(7, 1,COLOR_SKY_BLUE_G);   // com7-row1  ICON
		HT16D33_SetPixelPWM(8, 1,COLOR_SKY_BLUE_B);   // com8-row1  ICON
	}
	else{ // 연한 청색  20%
		HT16D33_SetPixelPWM(6, 1,COLOR_LIGHT_BLUE_20_R);     // com6-row1  ICON
		HT16D33_SetPixelPWM(7, 1,COLOR_LIGHT_BLUE_20_G);   // com7-row1  ICON
		HT16D33_SetPixelPWM(8, 1,COLOR_LIGHT_BLUE_20_B);   // com8-row1  ICON
	}
}

void fcLed_CoolDot(uint8_t on)
{
	BYTE level = on ? P100 : P20_B;
    HT16D33_SetPixelPWM(8, 10, level);   // com8-row10
}

void fcLed_Alkali(uint8_t on) //CHAR
{
	BYTE level = on ? P100 : P20;

    HT16D33_SetPixelPWM(11, 7, level);       // com11-row7
    HT16D33_SetPixelPWM(11, 8, level);       // com11-row8
   
}

void fcLed_AlkaliSel(uint8_t on)
{
	BYTE level = on ? P100 : P20;
    HT16D33_SetPixelPWM(8, 11,level);  // com8-row11
}

void fcLed_AlkaliCool(uint8_t on)
{
	BYTE level = on ? P100 : P20;
 
    HT16D33_SetPixelPWM(9, 11,  level);      // com9-row11
    HT16D33_SetPixelPWM(10, 11,level);      // com9-row11
}

void fcLed_AlkaliCoolSel(uint8_t on)
{
	//BYTE level = on ? P100 : P20;
	
	if(on){
		//연한 청색 100%
		HT16D33_SetPixelPWM(6, 2,COLOR_SKY_BLUE_R);   // com6-row1  ICON
		HT16D33_SetPixelPWM(7, 2,COLOR_SKY_BLUE_G);   // com7-row1  ICON
		HT16D33_SetPixelPWM(8, 2,COLOR_SKY_BLUE_B);   // com8-row1  ICON
	}
	else{
		//연한 흐린 청색 20%
		HT16D33_SetPixelPWM(6, 2, COLOR_LIGHT_BLUE_20_R);  // com6-row0  R
		HT16D33_SetPixelPWM(7, 2, COLOR_LIGHT_BLUE_20_G);    // com7-row0 G
		HT16D33_SetPixelPWM(8, 2, COLOR_LIGHT_BLUE_20_B);    // com8-row0 B
	}
}

void fcLed_AlkaliCoolDot(uint8_t on)
{
	BYTE level = on ? P100 : P20_B;
    HT16D33_SetPixelPWM(9, 6,level);   // com9-row6
}

void fcLed_IconMl(uint8_t on)
{
	BYTE level = on ? P100 : P20;

    HT16D33_SetPixelPWM(9, 7,level);       // com9-row7
    HT16D33_SetPixelPWM(11, 11, level);       // com11-row11
    
}
void fcLed_Run_Ready(uint8_t on)
{
	//BYTE level = on ? P100 : P20;

	HT16D33_SetPixelPWM(6, 3, COLOR_WHITE_R);   // com6-row3  R
	HT16D33_SetPixelPWM(7, 3, COLOR_WHITE_G);   // com7-row3  G
	HT16D33_SetPixelPWM(8, 3, COLOR_WHITE_B);   // com8-row3  B 
	
	HT16D33_SetPixelPWM(6, 4, COLOR_WHITE_R);   // com6-row4  R
	HT16D33_SetPixelPWM(7, 4, COLOR_WHITE_G);   // com7-row4  G
	HT16D33_SetPixelPWM(8, 4, COLOR_WHITE_B);   // com8-row4  B
	
	
	
}
void fcLed_Run(uint8_t on)
{
	//BYTE level = on ? P100 : P20;
    if(on==0){ // 점멸시 색깔
		if((s_mode&0x7f0 ) == ALKA || (s_mode&0x7f0)==COOLALKALI ){
		
			fcLed_AlkaliStep_Run(ion_state);
		}
		else if((s_mode&0x7f0 ) == HOT){
			
			fcLed_HotStep_Run(bSetHotTemper);

		}
		else  if((s_mode&0x7f0 ) == CLEAN){
			HT16D33_SetPixelPWM(6, 3, COLOR_YELLOW_R);   // com6-row3  R
			HT16D33_SetPixelPWM(7, 3, COLOR_YELLOW_G);   // com7-row3  G
			HT16D33_SetPixelPWM(8, 3, COLOR_YELLOW_B);   // com8-row3  B 
			
			HT16D33_SetPixelPWM(6, 4, COLOR_YELLOW_R);   // com6-row4  R
			HT16D33_SetPixelPWM(7, 4, COLOR_YELLOW_G);   // com7-row4  G
			HT16D33_SetPixelPWM(8, 4, COLOR_YELLOW_B);   // com8-row4  B
		}
		else  if((s_mode&0x7f0 ) == PURE){
			HT16D33_SetPixelPWM(6, 3, COLOR_GREEN_R);   // com6-row3  R
			HT16D33_SetPixelPWM(7, 3, COLOR_GREEN_G);   // com7-row3  G
			HT16D33_SetPixelPWM(8, 3, COLOR_GREEN_B);   // com8-row3  B 
			
			HT16D33_SetPixelPWM(6, 4, COLOR_GREEN_R);   // com6-row4  R
			HT16D33_SetPixelPWM(7, 4, COLOR_GREEN_G);   // com7-row4  G
			HT16D33_SetPixelPWM(8, 4, COLOR_GREEN_B);   // com8-row4  B
		}
		else  if((s_mode&0x7f0 ) == COOL){
			HT16D33_SetPixelPWM(6, 3, COLOR_BLUE_R);   // com6-row3  R
			HT16D33_SetPixelPWM(7, 3, COLOR_BLUE_G);   // com7-row3  G
			HT16D33_SetPixelPWM(8, 3, COLOR_BLUE_B);   // com8-row3  B 
			
			HT16D33_SetPixelPWM(6, 4, COLOR_BLUE_R);   // com6-row4  R
			HT16D33_SetPixelPWM(7, 4, COLOR_BLUE_G);   // com7-row4  G
			HT16D33_SetPixelPWM(8, 4, COLOR_BLUE_B);   // com8-row4  B
		}
		else{
			
			HT16D33_SetPixelPWM(6, 3, COLOR_WHITE_R);   // com6-row3  R
			HT16D33_SetPixelPWM(7, 3, COLOR_WHITE_G);   // com7-row3  G
			HT16D33_SetPixelPWM(8, 3, COLOR_WHITE_B);   // com8-row3  B 
			
			HT16D33_SetPixelPWM(6, 4, COLOR_WHITE_R);   // com6-row4  R
			HT16D33_SetPixelPWM(7, 4, COLOR_WHITE_G);   // com7-row4  G
			HT16D33_SetPixelPWM(8, 4, COLOR_WHITE_B);   // com8-row4  B
		}
	}
	else{ //정지시 화이트 
		HT16D33_SetPixelPWM(6, 3, COLOR_WHITE_R);   // com6-row3  R
		HT16D33_SetPixelPWM(7, 3, COLOR_WHITE_G);   // com7-row3  G
		HT16D33_SetPixelPWM(8, 3, COLOR_WHITE_B);   // com8-row3  B 
		
		HT16D33_SetPixelPWM(6, 4, COLOR_WHITE_R);   // com6-row4  R
		HT16D33_SetPixelPWM(7, 4, COLOR_WHITE_G);   // com7-row4  G
		HT16D33_SetPixelPWM(8, 4, COLOR_WHITE_B);   // com8-row4  B
	}
	
}

extern uint8_t bTouchResult1;
extern uint8_t bTouchResult2;
extern WORD touch_data;
extern WORD 	key_value;
extern uint16_t test_cur_sw;
extern ULONG filer1_life;
extern BYTE filter_error ,filter_error_fg;

extern ULONG	 lFlowSum;
extern BYTE  bFlushingStep;

extern BYTE bRestoreMute,voice_add;
extern ULONG disp_flow_hot; 
extern long lPump_rpm;
//---------------------------------------------------------
void fcDisplay(void)
{
 /* static WORD t = 0;

    HT16D33_ClearBuffer();

    if (t < 100) {                       
        HT16D33_SetPixelPWM(6,  0, 255); 
        HT16D33_SetPixelPWM(7,  0, 0);  
        HT16D33_SetPixelPWM(8,  0, 0);  
    } else {                             
        HT16D33_SetPixelPWM(9,  0, 0);   
        HT16D33_SetPixelPWM(10, 0, 255); 
        HT16D33_SetPixelPWM(11, 0, 0);
    }
    if (++t >= 200) t = 0;

    HT16D33_Update();
    return;*/
	
	/*  HT16D33_ClearBuffer();
    HT16D33_SetPixelPWM(6, 3, 0);   // ← R 이라고 가정한 라인
    HT16D33_SetPixelPWM(7, 3,   255);
    HT16D33_SetPixelPWM(8, 3,   0);
    HT16D33_Update();
    return;*/
/*
	if( (s_mode&0x7ff) == HOT )
	{
			HT16D33_ClearBuffer();
		DISP_SetDigitNum(0, disp_flow_hot/1000%10, 0);
		DISP_SetDigitNum(1, disp_flow_hot/100%10, 0);
		DISP_SetDigitNum(2, disp_flow_hot/10%10, 0);
		
		DISP_SetDigitNum(3, lPump_rpm/1000%10, 0);
		DISP_SetDigitNum(4, lPump_rpm/100%10, 0);
		DISP_SetDigitNum(5, lPump_rpm/10%10, 0);
		HT16D33_Update();
		return;
	}*/
	
    HT16D33_ClearBuffer();
	
	if(backlight_en_fg==0){

		// filter_error ,filter_error_fg	
		Display_Ready();// 백색 led
		HT16D33_Update();
		
		return ;
	}
		
	if( (s_mode&0x7ff) != FLUSHING){	
		
			Display_Number();	
	
			Display_Pure();
			Display_Cool();
			Display_Alkali();
			Display_AlkaliCool();
			Display_Hot();
			Display_FilterChange();
			

			Display_clean();
			Display_Set();
			Display_mL();
			Display_Uv();
			Display_CorkLed();
			Display_Ph_Temp_L();
			Display_Run();
			Display_CleanBtn();
		
	}
	else{
		Display_Flushing();
	}

    HT16D33_Update();

}

extern WORD test;
extern BYTE 	voice_add_old;
extern ULONG I_GAIN_M;

extern BYTE save_volume_level  , volume_level;

extern ULONG	set_pi_ref  ,flow_in_new;
extern int32_t avg_cool_temp;

extern int32_t avg_pelier_temp;

extern WORD	clean_delay_cnt;

extern   uint32_t wUvcCorkCnt;
extern BYTE bUvcCorkFg;

void fcDispErrorBuffer(void)
{
	uint8_t bError;
	uint8_t i;
	//  발생한 에러 검출 
	bErr_count = 0;
	for(i = 0; i <= 13; i++) {
		// 0번 비트부터 14번 비트까지 확인 (비트 0 -> 에러 1, 비트 14 -> 에러 15)
		if(lError & (1 << i)) {
			bActive_errors[bErr_count++] = i ; 
		}
	}

	//  동작 중에 에러가 줄어들어서 인덱스가 꼬이는 현상 방지
	//if(bErr_disp_idx >= bErr_count) {
	//	bErr_disp_idx = 0;
	//}
	// 에러 1개 : 점멸 / 2개 이상 : 로테이션 + 점멸
	if(bErr_count > 0) {
		if(bErr_count > 1) {
			if(++wErr_rotate_timer >= 200) {
				wErr_rotate_timer = 0;
				if(++bErr_disp_idx >= bErr_count)  bErr_disp_idx = 0;
			}
		}
		else {
			wErr_rotate_timer = 0;
			bErr_disp_idx = 0;
		}

		bError = bActive_errors[bErr_disp_idx];

		if(flicker_cnt <= FLICKER_ON) {
			DISP_SetDigitNum(3, FONT_E, 0);
			DISP_SetDigitNum(4, bError/10, 0);
			DISP_SetDigitNum(5, bError%10, 0);
		} else {
			DISP_SetDigitNum(3, FONT_BLANK, 0);
			DISP_SetDigitNum(4, FONT_BLANK, 0);
			DISP_SetDigitNum(5, FONT_BLANK, 0);
		}
	}
	/*// 에러가 1개일 때: 상시 점등
	if(bErr_count == 1) {
		bError = bActive_errors[0];
		DISP_SetDigitNum(3, FONT_E, 0);
		DISP_SetDigitNum(4, bError/10, 0);
		DISP_SetDigitNum(5, bError%10, 0);
	}
	// 에러가 2개 이상일 때: 로테이션 & 점멸
	else if(bErr_count > 1) {
	
		if(++wErr_rotate_timer >= 200) {
			wErr_rotate_timer = 0;
			
			if(++bErr_disp_idx >= bErr_count) {
				bErr_disp_idx = 0;
			}
		}

		bError = bActive_errors[bErr_disp_idx];


		if(flicker_cnt <= FLICKER_ON) { //50
			DISP_SetDigitNum(3, FONT_E, 0);
			DISP_SetDigitNum(4, bError/10, 0);
			DISP_SetDigitNum(5, bError%10, 0);
		} else {
	
			DISP_SetDigitNum(3, FONT_BLANK, 0);
			DISP_SetDigitNum(4, FONT_BLANK, 0);
			DISP_SetDigitNum(5, FONT_BLANK, 0);
		}
	}*/
}
extern uint16_t wModeSetOut_10SecCnt ;
extern int32_t avg_ho_temp ;

extern WORD clean_delay_cnt;
extern uint32_t  lDispenseTick;

extern ULONG auto_temp , auto_clean_cnt;
extern BYTE	 flow_error_cnt;
void Display_Number(void)
{
	uint8_t alkali_ph_temp;


	int16_t display_val,ho_temp1;

//ho_temp = 1234;
	
/*	 DISP_SetDigitNum(2, s_mode/1000%10, 0);
    DISP_SetDigitNum(3, s_mode/100%10,  0);
    DISP_SetDigitNum(4, s_mode/10%10,   0);
    DISP_SetDigitNum(5, s_mode%10,      0);
    return;*/
/*
	DISP_SetDigitNum(0, flow_liter/1000%10, 0);  //flow_hot_liter
	DISP_SetDigitNum(1, flow_liter/100%10, 0);
	DISP_SetDigitNum(2, flow_liter/10%10, 0);

	DISP_SetDigitNum(3, flow_error_cnt/100%10, 0);  
	DISP_SetDigitNum(4,  flow_error_cnt/10%10, 0);
	DISP_SetDigitNum(5,   flow_error_cnt/1%10, 0);	
return;*/

	
		if(bSetIntroFg){                      // 진입 애니메이션 중 : 숫자 표시 없음
		}
		else if((s_mode&0x7ff) == VOICE_SET){
			DISP_SetDigitNum(0, FONT_MINUS, 0);      // 좌측 : - n -
			DISP_SetDigitNum(1, volume_level, 0);
			DISP_SetDigitNum(2, FONT_MINUS, 0);
			
			DISP_SetDigitNum(3, FONT_BLANK, 0);      // 우측 : 소등
			DISP_SetDigitNum(4, FONT_BLANK, 0);
			DISP_SetDigitNum(5, FONT_BLANK, 0);
		}
		else if((s_mode&0x7ff) == LANG_SET){
			DISP_SetDigitNum(0, FONT_MINUS, 0);      // 좌측 : - n -
			DISP_SetDigitNum(1, lang_level+1, 0);
			DISP_SetDigitNum(2, FONT_MINUS, 0);
			
			DISP_SetDigitNum(3, FONT_BLANK, 0);      // 우측 : 소등
			DISP_SetDigitNum(4, FONT_BLANK, 0);
			DISP_SetDigitNum(5, FONT_BLANK, 0);
		}
		else if((s_mode&0x7ff) == CLEAN_SET){	//clean set
			if(auto_clean_value>=10)	 DISP_SetDigitNum(0, auto_clean_value/10%10, 0);
			DISP_SetDigitNum(1, auto_clean_value/1%10, 0);   // 좌측 : 10 ~ 100
			DISP_SetDigitNum(2, 0, 0);
			
			DISP_SetDigitNum(3, FONT_BLANK, 0);      // 우측 : 소등
			DISP_SetDigitNum(4, FONT_BLANK, 0);
			DISP_SetDigitNum(5, FONT_BLANK, 0);
		}
		else if((s_mode&0x7ff) == PH_SET){		//ph curr
			
			//ph
			fcLed_AlkaliStep(1);
			alkali_ph_temp = alkali_ph_step_disp[1-1];
			if(alkali_ph_temp/100%10) {
				DISP_SetDigitNum( 0 ,alkali_ph_temp/100%10 , 0);	//ph 10 단위		
			}
		
			DISP_SetDigitNum(1, alkali_ph_temp/10%10, 1);
			DISP_SetDigitNum(2, alkali_ph_temp/1%10, 0);
			
			//전류 설정  pi_ref_tbl[alkali_ph_step[ion_state-1]]
			DISP_SetDigitNum(3, alkali_ph_step[1-1]/100%10, 0);  //  disp_ion_i/1000%10
			DISP_SetDigitNum(4, alkali_ph_step[1-1]/10%10, 0);
			DISP_SetDigitNum(5,  alkali_ph_step[1-1]/1%10, 0);
		}
		else if((s_mode&0x7ff) == PH_SET2){		//ph curr
				
			//ph
			fcLed_AlkaliStep(2);
			alkali_ph_temp = alkali_ph_step_disp[2-1];
			if(alkali_ph_temp/100%10) {
				DISP_SetDigitNum( 0 ,alkali_ph_temp/100%10 , 0);	//ph 10 단위		
			}
		
			DISP_SetDigitNum(1, alkali_ph_temp/10%10, 1);
			DISP_SetDigitNum(2, alkali_ph_temp/1%10, 0);
			
		
			
			
			//전류 설정  pi_ref_tbl[alkali_ph_step[ion_state-1]]
			DISP_SetDigitNum(3, alkali_ph_step[2-1]/100%10, 0);  //  disp_ion_i/1000%10
			DISP_SetDigitNum(4, alkali_ph_step[2-1]/10%10, 0);
			DISP_SetDigitNum(5,  alkali_ph_step[2-1]/1%10, 0);
			
			
			
		}
		else if((s_mode&0x7ff) == PH_SET3){		//ph curr
	
			//ph
			fcLed_AlkaliStep(3);
			
			alkali_ph_temp = alkali_ph_step_disp[3-1];
			if(alkali_ph_temp/100%10) {
				DISP_SetDigitNum( 0 ,alkali_ph_temp/100%10 , 0);	//ph 10 단위		
			}
		
			DISP_SetDigitNum(1, alkali_ph_temp/10%10, 1);
			DISP_SetDigitNum(2, alkali_ph_temp/1%10, 0);
			
			   	//debug							
			//		DISP_SetDigitNum(0, avg_current/100%10, 0);  //hi_temp
			//	DISP_SetDigitNum(1, avg_current/10%10, 0);
			//DISP_SetDigitNum(2, avg_current/1%10, 0);
			//	DISP_SetDigitNum(0, I_GAIN_M/1000%10, 0);  //  disp_ion_i/1000%10   alkali_ph_step[3-1]
				//DISP_SetDigitNum(1, I_GAIN_M/100%10, 0);
				//DISP_SetDigitNum(2,  I_GAIN_M/10%10, 0);
				
			
			if(m_state & 0x02){
				DISP_SetDigitNum(3, alkali_ph_step[3-1]/100%10, 0);  //  disp_ion_i/1000%10   alkali_ph_step[3-1]
				DISP_SetDigitNum(4, alkali_ph_step[3-1]/10%10, 0);
				DISP_SetDigitNum(5,  alkali_ph_step[3-1]/1%10, 0);
				
				//DISP_SetDigitNum(3,  disp_ion_i/1000%10, 1);  //  disp_ion_i/1000%10   alkali_ph_step[3-1]
				//DISP_SetDigitNum(4,  disp_ion_i/100%10, 0);
				//DISP_SetDigitNum(5,  disp_ion_i/10%10, 0);
				
				
			}
			else{
				//전류 설정  pi_ref_tbl[alkali_ph_step[ion_state-1]]
				DISP_SetDigitNum(3, alkali_ph_step[3-1]/100%10, 0);  //  disp_ion_i/1000%10   alkali_ph_step[3-1]
				DISP_SetDigitNum(4, alkali_ph_step[3-1]/10%10, 0);
				DISP_SetDigitNum(5,  alkali_ph_step[3-1]/1%10, 0);
			}	
		}
		else if((s_mode&0x7ff) == UV_STOP_SET){	//UV_M_SET
			DISP_SetDigitNum(0,  set_uv_stop_time/100%10, 0);
			DISP_SetDigitNum(1,  set_uv_stop_time/10%10, 0);
			DISP_SetDigitNum(2,  set_uv_stop_time/1%10, 0);

		}
		else if((s_mode&0x7ff) == UV_RUN_SET){	//UV_S_SET
			DISP_SetDigitNum(0,  set_uv_stop_time/100%10, 0);
			DISP_SetDigitNum(1,  set_uv_stop_time/10%10, 0);
			DISP_SetDigitNum(2,  set_uv_stop_time/1%10, 0);
			
			DISP_SetDigitNum(3,  set_uv_run_time/100%10, 0);
			DISP_SetDigitNum(4,  set_uv_run_time/10%10, 0);
			DISP_SetDigitNum(5,  set_uv_run_time/1%10, 0);
		}
		else if((s_mode&0x7ff) == UV_CORK_SET){	//UV_CORK_SET
			DISP_SetDigitNum(0,  FONT_MINUS, 0);      // 좌측 : - U -
			DISP_SetDigitNum(1,  FONT_U, 0);
			DISP_SetDigitNum(2,  FONT_MINUS, 0);
			
			DISP_SetDigitNum(3,  set_uv_cork_sec_time/100%10, 0);
			DISP_SetDigitNum(4,  set_uv_cork_sec_time/10%10, 0);
			DISP_SetDigitNum(5,  set_uv_cork_sec_time/1%10, 0);
		}
		else if((s_mode&0x7ff) == UV_CORK_ONOFF){	//
			DISP_SetDigitNum(0,  FONT_L, 0);
			DISP_SetDigitNum(1,  FONT_E, 0);
			DISP_SetDigitNum(2,  FONT_d, 0);
			
			if(set_uv_cork_onoff==0){
				DISP_SetDigitNum(3,  FONT_o, 0);
				DISP_SetDigitNum(4,  FONT_F, 0);
				DISP_SetDigitNum(5,  FONT_F, 0);
			}
			else {
				DISP_SetDigitNum(3,  FONT_o, 0);
				DISP_SetDigitNum(4,  FONT_n, 0);
				DISP_SetDigitNum(5,  FONT_BLANK, 0);
			}
		}
		else if((s_mode&0x7ff) == BRIGHTNESS_SET){	//BRIGHTNESS_SET
			DISP_SetDigitNum(0,  FONT_MINUS, 0);
			DISP_SetDigitNum(1,  FONT_MINUS, 0);
			DISP_SetDigitNum(2,  FONT_MINUS, 0);
			
			DISP_SetDigitNum(3,  con_brightness/100%10, 0);
			DISP_SetDigitNum(4,  con_brightness/10%10, 0);
			DISP_SetDigitNum(5,  con_brightness/1%10, 0);
		}  
		/*else if((s_mode&0x7ff) == DEACTIVATE_SET){	//BRIGHTNESS_SET
			DISP_SetDigitNum(0,  FONT_MINUS, 0);
			DISP_SetDigitNum(1,  FONT_MINUS, 0);
			DISP_SetDigitNum(2,  FONT_MINUS, 0);
			
			DISP_SetDigitNum(3,  con_deactivate/100%10, 0);
			DISP_SetDigitNum(4,  con_deactivate/10%10, 0);
			DISP_SetDigitNum(5,  con_deactivate/1%10, 0);
		} */
		else if((s_mode&0x7ff) == HOT_OVER_T_SET){     /* 온수 과열 감지 온도 90~110 ℃ */
            DISP_SetDigitNum(0, FONT_MINUS, 0);
            DISP_SetDigitNum(1, FONT_MINUS, 0);
            DISP_SetDigitNum(2, FONT_MINUS, 0);

            DISP_SetDigitNum(3, con_hot_over_temp/100%10, 0);
            DISP_SetDigitNum(4, con_hot_over_temp/10%10,  0);
            DISP_SetDigitNum(5, con_hot_over_temp/1%10,   0);
        }
        else if((s_mode&0x7ff) == HOT_OVER_S_SET){     /* 온수 과열 감지 시간 1~15 초 */
            DISP_SetDigitNum(0, FONT_MINUS, 0);
            DISP_SetDigitNum(1, FONT_MINUS, 0);
            DISP_SetDigitNum(2, FONT_MINUS, 0);

            DISP_SetDigitNum(3, FONT_BLANK, 0);
            if(con_hot_over_sec >= 10) DISP_SetDigitNum(4, con_hot_over_sec/10%10, 0);
            else                       DISP_SetDigitNum(4, FONT_BLANK, 0);
            DISP_SetDigitNum(5, con_hot_over_sec%10, 0);
        }
        else if((s_mode&0x7ff) == TOUCH_SENS_SET){     /* 터치 감도 0~15 */
            DISP_SetDigitNum(0, FONT_MINUS, 0);
            DISP_SetDigitNum(1, FONT_MINUS, 0);
            DISP_SetDigitNum(2, FONT_MINUS, 0);

            DISP_SetDigitNum(3, FONT_BLANK, 0);
            if(con_touch_sens >= 10) DISP_SetDigitNum(4, con_touch_sens/10%10, 0);
            else                     DISP_SetDigitNum(4, FONT_BLANK, 0);
            DISP_SetDigitNum(5, con_touch_sens%10, 0);
        }
		
		else if((s_mode&0x7ff) == DEACTIVATE_SET){	// 비활성 버튼 표시 on/off + 밝기
			if(con_deact_show == DEACT_SHOW_ON){
				DISP_SetDigitNum(0,  FONT_o, 0);
				DISP_SetDigitNum(1,  FONT_n, 0);
				DISP_SetDigitNum(2,  FONT_BLANK, 0);   // "on "
			}
			else{
				DISP_SetDigitNum(0,  FONT_o, 0);
				DISP_SetDigitNum(1,  FONT_F, 0);
				DISP_SetDigitNum(2,  FONT_F, 0);       // "oFF"
			}

			DISP_SetDigitNum(3,  con_deactivate/100%10, 0);
			DISP_SetDigitNum(4,  con_deactivate/10%10, 0);
			DISP_SetDigitNum(5,  con_deactivate/1%10, 0);
		} 
		else{

		
			
			//seg 1,2,3--------------------------------------------------------
			if( (m_state&0x02)==0 ){   //출수가 아닐때
				if( (s_mode&0x7f0)==CLEAN){
					
				}
				else if((s_mode&0x7f0)==HOT){

					DISP_SetDigitNum(0, wSetHotTemperDisp[bSetHotTemper]/100%10, 0);
					DISP_SetDigitNum(1, wSetHotTemperDisp[bSetHotTemper]/10%10, 1);
					DISP_SetDigitNum(2, wSetHotTemperDisp[bSetHotTemper]/1%10, 0);
					
					fcLed_HotStep(bSetHotTemper);

				}
				else if( (s_mode&0x7f0)==ALKA|| (s_mode&0x7f0)==COOLALKALI){
					//ph
					fcLed_AlkaliStep(ion_state);
					alkali_ph_temp = alkali_ph_step_disp[ion_state-1];
					if(alkali_ph_temp/100%10) {
						DISP_SetDigitNum( 0 ,alkali_ph_temp/100%10 , 0);	//ph 10 단위		
					}
				
					DISP_SetDigitNum(1, alkali_ph_temp/10%10, 1);
					DISP_SetDigitNum(2, alkali_ph_temp/1%10, 0);   
		
										
			
				}  
				else if((s_mode&0x7f0)==COOL)
				{
					if(wCoolTemper>=0){
						display_val = wCoolTemper;
					
						if(display_val>=100)	{
							DISP_SetDigitNum(0, wCoolTemper/100%10, 0);
							DISP_SetDigitNum(1, wCoolTemper/10%10, 1);
						}
						else if(display_val>=10)		DISP_SetDigitNum(1, wCoolTemper/10%10, 1);
						
						DISP_SetDigitNum(2, wCoolTemper/1%10, 0);
					}
					else{
						//DISP_SetDigitNum(0,FONT_MINUS, 0);
						//DISP_SetDigitNum(1,0, 1);
						//DISP_SetDigitNum(2,0, 0);
						display_val = wCoolTemper;
						
						if (display_val < 0) {
							DISP_SetDigitNum(0, FONT_MINUS, 0); 
							display_val = -display_val;       
						} else {
							//DISP_SetDigitNum(0, FONT_SPACE, 0); 
						}

						
						if (display_val > 99) { //99 => -9.9
							display_val = 99; // 
						}

						if (display_val >= 10) {
							DISP_SetDigitNum(1, (display_val / 10) % 10, 1); // x.
						} else {
							DISP_SetDigitNum(1, 0, 1); // 0.
						}

						
						DISP_SetDigitNum(2, display_val % 10, 0);// 1의 자리
						
					}
					
				}
				else{

					DISP_SetDigitNum(0, FONT_MINUS, 0);
					DISP_SetDigitNum(1, FONT_MINUS, 0);
					DISP_SetDigitNum(2, FONT_MINUS, 0);			
				}
			}
			else{ //출수일때 유량 
				if( (s_mode&0x7f0)==CLEAN){
					if(flicker_cnt <= FLICKER_ON)	 {
						DISP_SetDigitNum(0, FONT_MLC1, 0);
						DISP_SetDigitNum(1, FONT_MLC2, 0);
						DISP_SetDigitNum(2, FONT_MLC3, 0);		
					}
				}
				else{
					//알칼리수 ,냉알칼리수 강중약 
					if( (s_mode&0x7c)==ALKA|| (s_mode&0x7c)==COOLALKALI){
						fcLed_AlkaliStep(ion_state); //강중약 
					}
					
					if(s_mode == HOT_OUT){  
						DISP_SetDigitNum(0, disp_flow_hot/1000%10, 1);  
						DISP_SetDigitNum(1, disp_flow_hot/100%10, 0);
						DISP_SetDigitNum(2, disp_flow_hot/10%10, 0);
						
						fcLed_HotStep(bSetHotTemper);
					}
					else{
						
					DISP_SetDigitNum(0, FONT_BLANK, 0);
					DISP_SetDigitNum(1, disp_flow_out/1000%10, 1);
					DISP_SetDigitNum(2, disp_flow_out/100%10, 0);

				// DISP_SetDigitNum(0, disp_ion_i/1000%10, 1);  //  disp_ion_i/1000%10  ???
				//DISP_SetDigitNum(1, disp_ion_i/100%10, 0);
				//DISP_SetDigitNum(2, disp_ion_i/10%10, 0);
					
					}
				
				}
				
			}
			//debug  flow_temp  filter1_life
			//	DISP_SetDigitNum(0, flow_temp/100%10, 0);
			//	DISP_SetDigitNum(1, flow_temp/10%10, 0);
			//	DISP_SetDigitNum(2, flow_temp/1%10, 0);
		
				
			//mL
			//seg 3,4,5--------------------------------------------------------
			if(lError==0){
				if( (s_mode&0x7f0)==CLEAN){
					if(m_state&0x02){
						if(flicker_cnt <= FLICKER_ON)	 {
							DISP_SetDigitNum(3, FONT_MLC1, 0);
							DISP_SetDigitNum(4, FONT_MLC2, 0);
							DISP_SetDigitNum(5, FONT_MLC3, 0);		
						}
					}
				}
				else if( (s_mode&0xfff)==HOT_OUT){  //debug  ULONG disp_flow_hot  saved_flow_hot_pulse
					
					if(ho_temp<0){
						ho_temp1 = 0-ho_temp;
						DISP_SetDigitNum(3, FONT_MINUS, 0);   
						DISP_SetDigitNum(4, ho_temp1/100%10, 0);
						DISP_SetDigitNum(5, ho_temp1/10%10, 0);
					}
					else{
						DISP_SetDigitNum(3, ho_temp/100%10, 0);   
						DISP_SetDigitNum(4, ho_temp/10%10, 1);
						DISP_SetDigitNum(5, ho_temp/1%10, 0);
					}
				}
			
				else{
					
					
					if(bSet_mL<=2){ //120,250,500 mL
						
						if(bSet_mL_FlickerOn==0){
							DISP_SetDigitNum(3, bDisp_mL[bSet_mL]/100%10, 0);
							DISP_SetDigitNum(4, bDisp_mL[bSet_mL]/10%10, 0);
							DISP_SetDigitNum(5, bDisp_mL[bSet_mL]/1%10, 0);
						}
					}
					else{ // 연속출수  
						
						if(m_state&0x02){
							DISP_UpdateMlRingRotate();
							DISP_ShowMlRingRotate();
						}
						else{
							if(bSet_mL_FlickerOn==0){
								DISP_SetDigitNum(3, FONT_MLC1, 0);
								DISP_SetDigitNum(4, FONT_MLC2, 0);
								DISP_SetDigitNum(5, FONT_MLC3, 0);
							}
						}
					}
					
					if(bSet_mL_FlickerFg){
						if(++bSet_mL_FlickerCnt>=30){
							bSet_mL_FlickerCnt = 0;
							
							bSet_mL_FlickerOn= ~bSet_mL_FlickerOn;		

							if( ++bSet_mL_Flicker_n >= bSet_mL_FlickerNum)
							{
								bSet_mL_Flicker_n = 0;
								bSet_mL_FlickerFg= 0;
								bSet_mL_FlickerOn = 0;
							}
						}
					}
					else{
						bSet_mL_FlickerOn = 0;
					}
					
					if(bSet_mL_Old != bSet_mL)
					{
						bSet_mL_FlickerOn=0xff;
						bSet_mL_Flicker_n = 0;
						if(bSet_mL<=2)	bSet_mL_FlickerNum=2;
						else bSet_mL_FlickerNum=6;
						bSet_mL_FlickerCnt=0;
						bSet_mL_FlickerFg=1;
						
					}
					bSet_mL_Old = bSet_mL;
					
				}
			}
			else{
					fcDispErrorBuffer();				
			}				
		}
}

//자동세정  [clean] icon
void Display_clean(void)
{

	//if(filter_change_en)
	{
		//if((s_mode&0x7ff)==CLEAN_SET) {	
	  	//  	if(flicker_cnt <= FLICKER_ON) 	fcLed_Clean(1);
		//}
		//else
		if((s_mode&0x7ff)==CLEAN || (s_mode&0x7ff)==PCLEAN)	
		{
			if(bManualCleanFg){                              // 수동 세정 : 점멸
				if(flicker_cnt <= FLICKER_ON)	fcLed_Clean(1);
			}
			else	fcLed_Clean(1);                          // 자동 세정 : 점등
		}
		
		if(cleaning_fg && ((s_mode&0x7f0) == CLEAN))
	  	{
			if(++clean_speed_cnt >= 25) //10ms*25
			{
				clean_speed_cnt = 0;

				if(++clean_cnt >= 5) clean_cnt = 0;						
			}
		}
	}
}

//세정   btn, char 
void Display_CleanBtn(void)
{
	if((s_mode&0x7f0) == CLEAN) {
		if(bManualCleanFg){                              // 수동 세정 : 점멸
			if(flicker_cnt <= FLICKER_ON){
				fcLed_BtnCleanSel(1);
				fcLed_BtnClean(1);
			}
			else{
				fcLed_BtnCleanSel(0);
				fcLed_BtnClean(0);
			}
		}
		else{                                            // 자동 세정 : 점등
			
			fcLed_BtnCleanSel(1);		// 세정버튼 아이콘 led 100%
			fcLed_BtnClean(1);			// 세정버튼 [세정] led
		}
	}
	else{
		fcLed_BtnCleanSel(0);		// 세정버튼 아이콘 led 20%
		fcLed_BtnClean(0);			// 세정버튼 [세정] led
	}
	
}

//설정 
void Display_Set(void)
{

	if((s_mode&0xf70) == 0x10) {	
		
		///설정모드 진입 애니메이션 : 음량 -> 언어 -> 세정주기 -> PH 순차 점등 (나머지 OFF), 2회 반복
		if(bSetIntroFg)
		{
			if(++wSetIntroCnt >= SET_INTRO_CNT)
			{
				wSetIntroCnt = 0;
				if(++bSetIntroStep >= 8)        // 4항목 x 2회 끝
				{
					bSetIntroStep = 0;
					bSetIntroFg   = 0;
				}
			}

			if(bSetIntroFg)
			{
				switch(bSetIntroStep & 0x03)
				{
					case 0 : fcLed_Volume(1);	break;
					case 1 : fcLed_Lang(1);		break;
					case 2 : fcLed_Cl(1);		break;
					case 3 : fcLed_Ph(1);		break;
					default : break;
				}
				bLast_s_mode = 0;               // 애니메이션 종료후 점멸 타이머 재시작
				return;
			}
		}
		
        if (bLast_s_mode != s_mode) {
            wBlink_timer = 0;
			bBlinkingFg = 0;
			bFlikerFg = 0;
			bBlink_OnTimer = 0;
            bLast_s_mode = s_mode;
        }

        if (++wBlink_timer >=200) {
            wBlink_timer = 0;
			bBlinkingFg = 1;
        }
		if(++bBlink_OnTimer>=50){
			bBlink_OnTimer = 0;
			bFlikerFg= ~bFlikerFg ; 
		}			

      
		
		//volume
	  	if((s_mode&0x7ff) == VOICE_SET){// 설정시 점멸
	  		if(bBlinkingFg==0){
				if(bFlikerFg==0)	 fcLed_Volume(1);
			}
			else{
				 fcLed_Volume(1);
			}				
	
	  	}

		//lang
		if((s_mode&0x7ff) == LANG_SET){
			if(bBlinkingFg==0){
				if(bFlikerFg==0)		 fcLed_Lang(1);
			}
			else{
				fcLed_Lang(1);
			}	
	  	}

		//clean set
		if((s_mode&0x7ff) == CLEAN_SET){
			if(bBlinkingFg==0){
				if(bFlikerFg==0)		 fcLed_Cl(1);
			}
			else{
				fcLed_Cl(1);
			}	
		}

		//ph curr
	  	if(((s_mode&0x7ff) >= PH_SET)  && ((s_mode&0x7ff) <= PH_SET3) ){
			if(bBlinkingFg==0){
				if(bFlikerFg==0)		 fcLed_Ph(1);
			}
			else{
				fcLed_Ph(1);
			}	

	  	}

		if((s_mode&0x7ff) == UV_STOP_SET || (s_mode&0x7ff) == UV_RUN_SET ){
			if(bBlinkingFg==0){
				if(bFlikerFg==0)		 fcLed_Uv(1);
			}
			else{
				fcLed_Uv(1);
			}	 

	  	}
	}
	else{
		wBlink_timer = 0;
		bLast_s_mode=0;
		bSetIntroFg   = 0;       
		bSetIntroStep = 0;
		wSetIntroCnt  = 0;
	}
}
void Display_Uv(void)
{
	if((s_mode&0xf70) == 0x10) {	//설정
	//	if((s_mode&0x7ff) == UV_M_SET || (s_mode&0x7ff) == UV_S_SET   ){
			
	//	}
	}
	else if(bUvcCoolFg){
		fcLed_Uv(1);
	}
	
	//if(bUvcCorkFg)
	//{
	//	  fcLed_CorkUv(1);
	//}
	//else{
	//	 fcLed_CorkUv(0);
	//}
}

/* 출수구(콕크) LED : com9=R, com10=G, com11=B, row0 */
static void CorkLed_Set(BYTE r, BYTE g, BYTE b)
{
    HT16D33_SetPixelPWM(9 , 0, r);
    HT16D33_SetPixelPWM(10, 0, g);
    HT16D33_SetPixelPWM(11, 0, b);
	HT16D33_SetPixelPWM(9 , 1, r);
    HT16D33_SetPixelPWM(10, 1, g);
    HT16D33_SetPixelPWM(11, 1, b);
	HT16D33_SetPixelPWM(9 , 2, r);
    HT16D33_SetPixelPWM(10, 2, g);
    HT16D33_SetPixelPWM(11, 2, b);
	HT16D33_SetPixelPWM(9 , 3, r);
    HT16D33_SetPixelPWM(10, 3, g);
    HT16D33_SetPixelPWM(11, 3, b);
	HT16D33_SetPixelPWM(9 , 4, r);
    HT16D33_SetPixelPWM(10, 4, g);
    HT16D33_SetPixelPWM(11, 4, b);
	HT16D33_SetPixelPWM(9 , 5, r);
    HT16D33_SetPixelPWM(10, 5, g);
    HT16D33_SetPixelPWM(11, 5, b);
}

void Display_CorkLed(void)
{
    /* 콕크램프 설정이 oFF 거나, 출수 중이 아니면 소등 */
    if (set_uv_cork_onoff == 0 || !(m_state & 0x02)) {
        CorkLed_Set(0, 0, 0);
        return;
    }

    switch (s_mode & 0x7f0)
    {
    case PURE:                                      //정수 : GREEN 
        CorkLed_Set(COLOR_GREEN_R, COLOR_GREEN_G, COLOR_GREEN_B);
        break;

    case COOL:                                      // 냉수 : BLUE 
        CorkLed_Set(COLOR_BLUE_R, COLOR_BLUE_G, COLOR_BLUE_B);
        break;

    case HOT:                                       //온수 : 온도 단계별 
        if (bSetHotTemper == 2)                     // 강 : 적색 
            CorkLed_Set(COLOR_RED_R,    COLOR_RED_G,    COLOR_RED_B);
        else if (bSetHotTemper == 1)                //중 : 주황 
            CorkLed_Set(COLOR_ORANGE_R, COLOR_ORANGE_G, COLOR_ORANGE_B);
        else                                        //약 : 노랑 
            CorkLed_Set(COLOR_YELLOW_R, COLOR_YELLOW_G, COLOR_YELLOW_B);
        break;

    case ALKA:                                      //알칼리 / 냉알칼리 : PH 단계별 
    case COOLALKALI:
        if (ion_state == 3)                         //강 : 자홍 
            CorkLed_Set(COLOR_MAGENTA_R,     COLOR_MAGENTA_G,     COLOR_MAGENTA_B);
        else if (ion_state == 2)                    // 중 : 블루 
            CorkLed_Set(COLOR_BLUE_R,        COLOR_BLUE_G,        COLOR_BLUE_B);
        else                                        //약 : 약 그린 
            CorkLed_Set(COLOR_LIGHT_GREEN_R, COLOR_LIGHT_GREEN_G, COLOR_LIGHT_GREEN_B);
        break;

    default:                                        //세정/플러싱 등 : 사양 없음 → 소등 
        CorkLed_Set(0, 0, 0);
        break;
    }
}

// mL
void Display_mL(void)
{
	
	if((s_mode&0xf70 )== MODE_SET)
	{
		//if((s_mode&0xf70 )== CLEAN_SET){
		//	fcLed_mL(1); // [mL] 
		//}
		fcLed_mL_Btn(1); // btn icon,char
	}
	else if((s_mode&0x7f0 )== CLEAN)
	{		
		
	}
	else if((s_mode &0xff0 )!= MODE_SET){
		if( lError==0){
			if(bSet_mL<=2)	{
				fcLed_mL(1); // [mL] 
			}
			else {
				fcLed_ContinuousPut(1); // [연속출수]
			}
		
			fcLed_mL_Btn(1); // btn icon,char
		}
	}
}

//[강] [중] [약] ,temp unit(c) , [LPM]
void Display_Ph_Temp_L(void)
{
	
	if( m_state&0x02 && ((s_mode &0x7ff )!=CLEAN)  ){ //출수시 
		fcLed_Lpm(1); // [LPM]
	}
	else if((s_mode &0xff0 )==HOT  ||  (s_mode &0xff0 )==COOL ){
		fcLed_TempUnit(1);	//[c]
	}
	else if((s_mode &0xff0 )==ALKA || (s_mode &0xff0 )==COOLALKALI){
		fcLed_AlkaliStep(ion_state); // [강] [중] [약]
	}

}


//정수 led
void Display_Pure(void)
{
	if((s_mode&0xf70 )== MODE_SET)
	{
		fcLed_PureSel(1); // - , 정수버튼 아이콘
	
	}
	else if((s_mode&0x7f0 )== PURE)
	{
		fcLed_Pure(1);		// - , 정수버튼 글자 led
		fcLed_PureSel(1);// - , 정수버튼 아이콘 led
	}
	else{
		fcLed_Pure(0); //20%
		fcLed_PureSel(0); //20%
	}
}


//냉수 btn, char 
void Display_Cool(void)
{
	
	//bCoolTempStatus = 1; // ???
	if((s_mode&0x7f0 )== MODE_SET)
	{
		fcLed_CoolSel(1); //냉수버튼 아이콘 led
	
	}
	else if( ((s_mode&0x7f0 ) == COOL))// || ((s_mode&0x7f0 ) == COOLALKALI) )
	{
		
		fcLed_CoolSel(1);//냉수버튼 아이콘 led
		fcLed_Cool(1);		//냉수버튼 [냉수] led

		
	}
	else{
		fcLed_CoolSel(0);//냉수버튼 아이콘 led   20%
		fcLed_Cool(0);		//냉수버튼 [냉수] led  20%
		
	}
	
	if( !(lError & ERR_COOL_TEMP_OPEN) && (bCoolReadyFg) )
		fcLed_CoolDot(1);           // 10.0도 이하 -> 점등
	                                // 초과 -> 그리지 않음 = 소등
}
//알칼리수 btn, char 
void Display_Alkali(void)
{
	if((s_mode&0xf70 )== MODE_SET)
	{
		fcLed_AlkaliSel(1) ; //알칼리수버튼 아이콘 led
	
	}
	else if((s_mode&0x7f0 ) == ALKA)
	{
		fcLed_Alkali(1);      //알칼리수버튼 [알칼리수]] led
		fcLed_AlkaliSel(1); //알칼리수버튼 아이콘 led

	}
	else{
		fcLed_Alkali(0);      //알칼리수버튼 [알칼리수]] led  20%
		fcLed_AlkaliSel(0); //알칼리수버튼 아이콘 led 20%
	}
}

//냉알칼리수 btn, char 
void Display_AlkaliCool(void)
{

	if((s_mode&0x7f0) == COOLALKALI)
	{
		fcLed_AlkaliCoolSel(1); //냉 알칼리수버튼 아이콘 led
		
		fcLed_AlkaliCool(1);		//냉 알칼리수버튼 [냉알칼리수]] led
		
	}
	else{ //20%
		
		fcLed_AlkaliCoolSel(0); //냉 알칼리수버튼 아이콘 led  20%
		fcLed_AlkaliCool(0);		//냉 알칼리수버튼 [냉알칼리수]] led  20%
		
	}
	
	if( !(lError & ERR_COOL_TEMP_OPEN) && (bCoolReadyFg) )
		fcLed_AlkaliCoolDot(1);
}

//온수  btn, char 
void Display_Hot(void)
{
	if((s_mode&0x7f0 ) == HOT) // && (bHotLockFg==0))
	{
		fcLed_Hot(1);		//온수버튼 [온수]] led
		fcLed_HotSel(1);  //온수버튼 아이콘 led
		
		
		//온수 잠금
		if(bHotLockFg){
			fcLed_HotDot(1);  // 온수잠금 
		}
		else{ //온수 잠금이 아니면
			fcLed_HotDot(0);  // 온수잠금 
		}
		
	}
	else{
		fcLed_Hot(0);		//온수버튼 [온수]] led 20%
		fcLed_HotSel(0);  //온수버튼 아이콘 led 20%
		
		fcLed_HotDot(0);  // 온수잠금 20%
	}
}


void Display_FilterChange(void)
{
	// 필터교체
	ULONG f_temp;
	if((s_mode &0xff0 )!= MODE_SET){
		if( (filter_change_fg&0x30) || ( (s_mode&0x7f)== FLUSHING))// 점멸
		{
			if(flicker_cnt <= FLICKER_ON)	 fcLed_FilterChange(1);
		}
		else if(filter_change_fg&0x03)// 점등
		{
			fcLed_FilterChange(1);
		}
		
		// 필터 양 표시 :  더 작은 필터값 기준으로 
		if(filter1_life > filter2_life)	f_temp = filter2_life;
		else 	f_temp = filter1_life;
			
		if(f_temp  >= FILTER_60P)			fcLed_Filter(3);
		else if(f_temp  >= FILTER_30P)	fcLed_Filter(2);	
		else if(f_temp  >= FILTER_0P)	fcLed_Filter(1);	
		else {                                          // 100L 미만 : 1개 점멸
			if(flicker_cnt <= FLICKER_ON)	fcLed_Filter(1);
		}
	}	
	
	
}
void Display_Flushing(void)
{
	if(  s_mode == FLUSHING_OUT )// 점멸
	{
		if(flicker_cnt <= FLICKER_ON)	 fcLed_FilterChange(1);
	}
	else if(  s_mode == FLUSHING)// 점등
	{
		fcLed_FilterChange(1);
	}

	// 온수 관로 배수 구간 : 온수 온도 LED 순차 점등 (약 -> 중 -> 강 반복)
	if( (s_mode == FLUSHING_OUT) && (bFlushingStep >= 4) )
	{
		if(++wFlushHotLedCnt >= FLUSH_HOT_LED_CNT)   // 300ms 마다 이동
		{
			wFlushHotLedCnt = 0;
			if(++bFlushHotLedStep >= 3)   bFlushHotLedStep = 0;
		}
		fcLed_HotStep(bFlushHotLedStep);             // 0=노랑 1=주황 2=적색
	}
	else
	{
		wFlushHotLedCnt  = 0;
		bFlushHotLedStep = 0;
	}

	
	if(lError){
		fcDispErrorBuffer();
	}
	
	/*
	DISP_SetDigitNum(0, 0 /100000%10, 0);  //flow_hot_liter
				DISP_SetDigitNum(1, 0/10000%10, 0);
				DISP_SetDigitNum(2, s_mode/1000%10, 0);
			
				DISP_SetDigitNum(3, s_mode/100%10, 0);  
				DISP_SetDigitNum(4,  s_mode/10%10, 0);
			   DISP_SetDigitNum(5,   s_mode/1%10, 0);	*/
}

void Display_Ready(void)
{
	//if( m_state & 0x02){
		fcLed_Run_Ready(1);
	//}
}
void Display_Run(void)
{
	if( m_state & 0x02){
		
		if(flicker_cnt <= FLICKER_ON)	 fcLed_Run(1);
		else  fcLed_Run(0);
	}
	else{
		fcLed_Run(1);
	}
}

