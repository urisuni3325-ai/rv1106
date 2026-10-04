/**********************************************************************
* @file		main.c
* @brief	WT Polling Example Code tested on A31G22x StarterKit
* @version	1.00
* @date		18 Aug 2020
* @author	ABOV AE team
*
* Copyright(C) 2020, ABOV Semiconductor
* All rights reserved.
**********************************************************************/
//260311

//26.0707
//#define FILTER_FLUSHING	  4000000L/// ??? 3000000L   이거 변경해야 함 3000으로
// DEBUG_FLOW


#include "main_conf.h"



/* Private typedef -----------------------------------------------------------*/

typedef unsigned char BYTE ;
typedef unsigned int   WORD ;
typedef unsigned long   ULONG ;
/* Private define ------------------------------------------------------------*/
/* Private macro ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
//260223
#define START_ADDR (0x0E000000UL)
#define END_ADDR     (0x0E008000UL)
#define BufferSize   (4096)
static unsigned char Buffer[BufferSize];


#define	F_LIFE			    1	//190408 0:3,4 1:3,6 2:4,8 3:6,9 4:8,8 5:3,5
#define SMPS_TYPE		1	//190502 0:전해조5P 1:전해조7P 2:전해조9P

#include "user_define.h"

#include "Display_16D33.c"
#include "GD25d10.c"
#include "aes128.c"
#include "Ntc.c"


#define	PH_DISP			0	//1:usa 수출용TB 0:korea 국내용

#define 	I_LIMIT  		3000	
#define 	SENSOR			1
#define	FLOW_GAIN		432 // 370;       	// 유량 게인 
#define	WARNING_DELAY	25	//210812 20->25
#define	ERROR_CNT		5		//151222_1 필터 데이터 처리 변경  231005-5 3->5 

// 출수 시작 지연
//  출수 버튼을 눌러도 바로 나가지 않고 1초 뒤에 시작한다.
//  1초가 되기 전에 다시 누르면 출수를 하지 않고 카운트도 지운다.
#define START_DELAY_TICK   10    // 100ms x 10 = 1초
void Start_Delay_Check(void);


//	전류 테이블 _ PI 제어시 사용되는 목표치
/*#if SMPS_TYPE == 0	//180629 0:전해조5P
const WORD	pi_ref_tbl[76] =	//150320_1
						{ 220, 230, 240, 250, 260,  270, 280, 290, 300, 310,  320, 330, 340, 350, 360,  370, 380, 390, 400,
						  400, 405, 410, 420, 430,  440, 450, 460, 470, 480,  490, 550, 570, 590, 610,  640, 670, 680, 690,
						  690, 710, 730, 750, 770,  790, 800, 850, 900, 950, 1000,1100,1200,1300,1400, 1500,1600,1700,1800,
						 2150,2200,2250,2280,2330, 2350,2450,2550,2650,2750, 2850,2950,3150,3350,3550, 3750,3950,4150,4350};


#elif SMPS_TYPE == 1 	//180629 1:전해조7P

const WORD	pi_ref_tbl[76] =	//150320_1 
					  //0,      1,    2,    3,     4,    5,     6,    7,    8,    9,    10,   11,  12,    13,  14,  15,   16,   17,   18
				  	 { 100, 120, 140, 160, 180, 200, 250, 300, 310, 320, 330, 340, 350, 360, 370, 380, 390, 400, 410,//18
					    420, 430, 440, 450, 460, 470, 480, 500, 520, 540, 560, 580, 600, 620, 640, 660, 680, 700, 725,//37
					    750, 775, 800, 825, 850, 875, 900, 925, 950, 975,1000,1100,1200,1300,1400,1500,1600,1700,1800,
					  1900,2000,2100,2200,2300,2400,2500,2600,2700,2800,2900,3200,3400,3600,3800,4000,4500,5000,5500};

#endif*/
const WORD	pi_ref_tbl[57] =	
					  //0,       1,     2,     3,      4,     5,     6,      7,     8,      9,      10,      11,     12,      13,     14,   15,   16,      17,   18
					 { 100, 120, 140,   160,  180,  200,  250,   300,   310,   320,   330,   340,   350,  360,   370,   380,   390,   400,   410,//18
					    750, 775, 800,   825,  850,  875,  900,   925,   950,   975, 1000, 1100, 1200, 1300, 1400, 1500,  1600, 1700, 1800,//36
					  1900,2000,2100,2200,2300,2400, 2500, 2600, 2700, 2800,  3000, 3200, 3400, 3600, 3800, 4000,  4500, 5000, 5500};
				  	// { 100, 120,  140,  160,  180,  200,  250,  300,  310, 320,  330,  340,  350,  360,   370,  380, 390,  400, 410,
                    //    600, 620,  640,  660,  680,  780,  800,  825,  850, 875,  900,  925,  950,  975, 1000, 1050,1100,1200,1300,
					//  1400,1500,1600,1700,1800, 1900,2000,2100,2200,2300,2400,2500,2600, 2700, 2800, 2900,3200,3400,3500};
					 
//	PWM 테이블 _ PI 제어시 사용되는 초기 PWM
 const WORD pwm_init_tbl[57] =
						   //0,       1,     2,     3,      4,      5,   6,       7,        8,      9,      10,     11,    12,     13,     14,     15,    16,      17,   18
					  	 {110,   140,  170,  200,  230,   260,  330,   400,   410,   420,    430,   450,   460,   470,   490,   500,   520,   530,   540,
					       980, 1010,1040,1070, 1110, 1130,1170,  1190, 1230, 1260,  1290,  1420, 1550, 1670,  1800, 1930, 2050, 2180, 2300,
					     2420, 2540,2670,2790, 2910, 3040,3160,  3280,  3410, 3530, 3560,  4030, 4280, 4530,  4780, 4980, 4980, 4980, 4980};
				  	//   {  110,  140,   170,   200,   230,    260,   330,   400,   410,   420,   430,   450,   460,   470,   490,   500,   520,   530,  540,
					//	   870,  890,   920,   940,   980,  1010, 1040,  1070, 1110, 1130, 1170,  1190, 1230, 1260, 1290,  1420, 1550, 1670, 1800,
					//	 1800, 1930, 2050, 2180, 2300,  2420, 2540,  2670, 2790, 2910, 3040,  3160, 3280,  3410, 3530, 3560, 4030, 4280 ,4530};
/*const WORD pwm_init_tbl[76] =
				  	   {110, 140, 170, 200, 230, 260, 330, 400, 410, 420, 430, 450, 460, 470, 490, 500, 520, 530, 540,
						550, 570, 580, 590, 600, 620, 630, 640, 680, 710, 740, 760, 790, 810, 840, 870, 890, 920, 940,
						980,1010,1040,1070,1110,1130,1170,1190,1230,1260,1290,1420,1550,1670,1800,1930,2050,2180,2300,
					  2420,2540,2670,2790,2910,3040,3160,3280,3410,3530,3560,4030,4280,4530,4780,4980,4980,4980,4980};*/

	          	  			  	     
//	단계마다의 pH 테이블
/*const BYTE pure_ph_tbl[5][16] =
            	{	{		17, 16, 15, 14, 13,  12, 11, 10,  9,  8,   7,  6,  5,  4,  3,   2 },
            	  	{ 	36, 35, 34, 33, 32,  31, 30, 29, 28, 27,  26, 25, 24, 23, 22,  21},
            	  	{ 	55, 54, 53, 52, 51,  50, 49, 48, 47, 46,  45, 44, 43, 42, 41,  40},
            	  	{ 	74, 73, 72, 71, 70,  69, 68, 67, 66, 65,  64, 63, 62, 61, 60,  59},
            	  	{ 	65, 66, 67, 68, 69,  70, 71, 72, 73, 74,  75, 76, 77, 78, 79,  80}};
*/
BYTE ion_tbl[4]	= {70,85,95,100};	
//const BYTE orp_hi_tbl[9] = {25,18,27,36,45,54,59,64,69};		//ORP상한 설정 한계값
//const BYTE orp_lo_tbl[9] = {25,10,19,28,37,50,55,60,65};		//ORP하한 설정 한계값
const BYTE sound_volum_tbl[5] = {0x80,0x83,0x85,0x86,0x87};	//	볼륨 값            	                              
const BYTE sound_lang_tbl[8] = {0,29,58,87,116,145,174,203};				//	언어 선택용

//#define LOW_TABLE_SIZE 5
//const uint16_t adc_table_low[LOW_TABLE_SIZE] = {172, 186, 219, 273, 337};
//const uint16_t ma_table_low[LOW_TABLE_SIZE]  = {120, 200, 300, 400, 500};
#define LUT_SIZE 10
const int adc_lut[LUT_SIZE]     = {159, 200, 240, 300, 360, 420, 480, 540, 600, 660};
const int current_lut[LUT_SIZE] = {0,   200, 300, 400, 500, 600, 700, 800, 900, 1000};


static ADC_CFG_Type ADC_Config;

//I2C
#define TSM16_I2Cn						(0) // I2C2
#define HT16K33_I2Cn						(1) // I2C0
#define BUFFER_SIZE					192 //(192-1) 		// Data Buffer Size (Excluded slave address byte size)
#define I2C_IP_INDEX_MAX			(2)
#define ADDR_TSM16		            (0xD0)
#define SLAVE_ADDR	                (ADDR_TSM16 >> 1) // Slave Address
#define I2C_SPEED	                	(400000)	// 400kHz
#define SLAVE_ADDR_16K33	    (0xe0 >> 1) 

#define SLAVE_ADDR_16D33 0x64

// I2C_PORT_Type Structure
typedef struct {
	// Base Address
	I2C_Type *pI2Cx;
 	// SCL
	PORT_Type *pSclPort;
	uint32_t SclPort_Number;
	PCU_ALT_FUNCTION_Type SclAltFunc;
	// SDA
	PORT_Type *pSdaPort;
	uint32_t SdaPort_Number;
	PCU_ALT_FUNCTION_Type SdaAltFunc;
	// Interrupt Handler
	IRQn_Type IRQ_Type;
	// Module Name
	uint8_t I2cName[5];
} I2Cn_PORT_Type;

typedef enum {
    MASTER = 0,
    SLAVE = 1
} I2Cn_Type;

static const I2Cn_PORT_Type I2C_PORT_CFG[I2C_IP_INDEX_MAX] = {
	{I2C2, (PORT_Type *)PC , 6, PCU_ALT_FUNCTION_1, (PORT_Type *)PC, 5, PCU_ALT_FUNCTION_1, I2C2_SPI20_IRQn,       "I2C2"},
	{I2C0, (PORT_Type *)PD,  0, PCU_ALT_FUNCTION_1, (PORT_Type *)PD, 1, PCU_ALT_FUNCTION_1, I2C0_IRQn,       "I2C0"},
};
static const I2Cn_PORT_Type * Master = &(I2C_PORT_CFG[TSM16_I2Cn]);
 const I2Cn_PORT_Type *I2C_DISP = &(I2C_PORT_CFG[HT16K33_I2Cn]);
static void Buffer_Init(uint8_t* buffer, uint8_t type, uint16_t len);

int isMatched = 0;
uint8_t WriteBuffer[BUFFER_SIZE];
uint8_t WriteReqBuffer[BUFFER_SIZE];
uint8_t ReadBuffer[BUFFER_SIZE];

uint8_t Disp_WriteBuffer[BUFFER_SIZE];
uint8_t Disp_WriteReqBuffer[BUFFER_SIZE];
uint8_t Disp_ReadBuffer[BUFFER_SIZE];


//door

uint16_t wDoorCnt = 0;
uint8_t bDoorState = 0;
uint8_t bPrevDoorState = 0;
uint16_t wDoorOpenCnt = 0;
   

I2C_M_SETUP_Type MasterCfg;
I2C_M_SETUP_Type DisplayCfg;

uint8_t bTouchData[3][4];
uint8_t bTouchData_Old[3][4];
uint8_t bTouchResult1=0;
uint8_t bTouchResult2=0;
uint8_t bTouchResult3=0;
uint16_t test1,test2;
uint16_t test_cur_sw;
// 인터럽트 사용 보이스 칩 제어
BYTE 	VD_1=5, VD_2=1, VD_3=2;

BYTE 	voice_real_fg=0,voice_delay_cnt=0,voice_add_old=0;
int  	voice_out_cnt=0;
BYTE 	language_jump=0;

uint32_t lmsec=0;
BYTE 	sysfg=0;

//	필터 교체
BYTE	filter_change_cnt=0, filter_change_en=0;

//	시간  
WORD	m_n5sec_cnt=0;
WORD	return_mode_cnt=0;
WORD	backlight_off_cnt=0;
WORD	clean_delay_cnt=0;

BYTE 	m_n1Sec_cnt=0, auto_inc_cnt=0;
BYTE 	m_n500ms_cnt=0;
BYTE 	timer_10m_fg=0, timer_10m_cnt=0;
BYTE 	m_n500ms_ok=0;
BYTE 	comm_error=0,rotate_cnt=0;
  
//timer
BYTE 	by10msCnt=0, by20msCnt=0;
BYTE 	byTimer10msFg=0, byTimer20msFg=0;
     	
WORD 	s_mode=PURE, s_mode_old=PURE, prev_alkali_mode=ALKA2;    
WORD  s_mode_return = PURE;
WORD  s_mode_return_alkali = ALKA2;
WORD  s_mode_return_coolalkali = COOLALKALI2;

//voice
BYTE 	m_state=0x00;    
        		          	// 0    0    0     0       0    0    0      0
        		          	// 단수 예비 필터2 필터1   점검 세정 물유입 전해중
        		          	// 0x02 :: 물유입
        		          	// 0x80 :: 단수동작
        		  
BYTE 	backlight_en_fg=0,bl_color=0;
                  		// 0 : 꺼짐
                  		// 1 : 정수    :: 밝은 녹색
                  		// 2 : 산성수1 :: 어두운 녹색
                  		// 3 : 산성수2 :: 적색
                  		// 4 : 알칼리1 :: 밝은 청색
                  		// 5 : 알칼리2 :: 청색
                  		// 6 : 알칼리3 :: 어두운 청색
                  		// 7 : 알칼리4 :: 밝은 자색  
  
 // 저장용       
BYTE 	save_backlight_on_fg=0;
BYTE	save_eosled_on_fg=0;		
BYTE 	save_volume_level=2;            
BYTE 	save_lang_level=0;    
BYTE 	save_pure_ph=8;       		
BYTE 	save_auto_clean_value=0;   	// 세정 리터
BYTE 	save_alkali_water=1;      	// 알칼리 pH
 
 // 디스플레이용
BYTE 	backlight_on_fg=0;
BYTE	eosled_on_fg=0;	
BYTE 	volume_level=2;       		// 소리 볼륨      
BYTE 	pure_ph_value=8;         	// 정수 pH
BYTE 	auto_clean_value=0;      	// 세정 리터    		
BYTE 	lang_level=0;       			
ULONG auto_temp=0;

BYTE 	pure_out_state=0;        	// 정수 출수 상, 하
BYTE	pure_out_fg=0,first_clean_fg=0;
BYTE 	ionize_fg=0; 					// 전해중
BYTE 	flow_in_fg=0;					// 물유입
BYTE 	cleaning_fg=0;					// 세정중
     	     	
BYTE 	filter_error_fg=0;
BYTE 	filter_change_fg=0;
BYTE    bFilterChgVFg=0;
BYTE 	filter_error=0, filter_life_read_fg=0, f1_life_read_fg=0, f2_life_read_fg=0;	
BYTE 	ch_select=0;
BYTE b1SecCnt=0;

BYTE   alkali_ph_step_disp[3]={85,95,100};
BYTE   alkali_ph_step[3]={9,29,49};
BYTE   save_alkali_ph_step[3]={9,30,49} ;      // 알칼리 스텝 ; 3단계

	
//온도
WORD wSetHotTemperDisp[3]={450,750,850};	
BYTE bSetHotTemper=2; // 2=95
BYTE bHotLockFg = 0;
BYTE bWaterMode=0;
BYTE bHotOut_WaitFg=0;
WORD wHotOut_WaitTimeCnt=0;	
BYTE bHotOutEndFg=0;
WORD wHotOutEndCnt=0;
BYTE bStopDelayCnt = 0;				
WORD wHoTempErrCnt = 0;
WORD wHiTempErrCnt= 0;
//Key	
	BYTE key_num=0;
WORD 	key_value=0, key_new=0, old_key_new=0,key_old=0,buffer_key_new=0,old_key_value=0;
WORD 	sw_data=0,old_sw_data=0,key_buffer=0,old_key_buffer=0;
BYTE 	key_fg=0;
WORD touch_data = 0;
     	
BYTE 	key_up_fg=0, key_dn_fg=0;
WORD 	key_cnt=0;			//53
BYTE 	key_set_fg=0, key_mode_fg=0;
BYTE 	auto_key_value=1, auto_key_cnt=0;
BYTE 	cal_start_cnt=0;

ULONG save_f1_life=0L, save_f2_life=0L;       

ULONG read_f1_ex_life=0L, read_f2_ex_life=0L;       
                                        
ULONG filter1_life=1000000L;       		// 6000.0
ULONG filter2_life=1000000L;       		// 9000.0
BYTE	byF1_serial_err_cnt=0,byF1_life_err_cnt=0,byF2_serial_err_cnt=0,byF2_life_err_cnt=0;
WORD	wF1_life=0,wF1_life_old=0,wF2_life=0,wF2_life_old=0;


// 유량
ULONG flow_liter=0;      			// 1초당 리터
ULONG flow_hot_liter=0; 
ULONG flow_out_liter=0; 
ULONG flow_in_new=0, flow_in_old=		0, auto_clean_cnt=0;
ULONG flow_in_out_new=0, flow_in_out_old=		0; 
ULONG flow_in_hot_new=0, flow_in_hot_old=		0;
ULONG	flow_pulse_cnt=0;        	// 외부 인터럽트 카운터
ULONG	saved_flow_pulse=0;
ULONG	saved_flow_hot_pulse=0;
ULONG	saved_flow_out_pulse=0;

ULONG disp_flow_hot = 0;
ULONG disp_flow= 0;
ULONG disp_flow_out= 0;



ULONG disp_flow_hot_sum=0;
ULONG disp_flow_sum=0;
BYTE    disp_flow_hot_cnt = 0;

ULONG disp_flow_out_sum=0;

BYTE bFlowCnt= 0;

ULONG flow_temp=0;
ULONG flow_temp_sum=0;
ULONG disp_ho_temp =0;
ULONG ho_temp_sum=0;
ULONG disp_hi_temp =0;
ULONG hi_temp_sum=0;

ULONG flow_out_temp =0;
ULONG flow_out_temp_sum=0;

ULONG	flow_hot_pulse_cnt=0;    
ULONG	flow_out_pulse_cnt=0;    

ULONG lFlowSum=0;
ULONG  lTargetFlow=0;
ULONG lFlowSum_Hot=0;
ULONG lFlowSum_Out=0;
ULONG lFlowSum_In=0;
WORD wFlowSum_ErrCnt=0;


WORD 	pwm_value=0;

// 전압, 전류     		
ULONG ion_i_sum=0,ion_v_sum=0,disp_ion_i=0,disp_ion_v=0,ion_v=0,ion_i=0;
ULONG avg_volt, avg_current, old_volt=0, old_current=0;
ULONG avg_temp=0, old_temp=0, temp_sum=0, disp_temp=0;	
ULONG ad_current=0,ad_volt=0,ad_temp=0,ad_wtemp=0;     
BYTE  ad_return_cnt=0,ad_ok_fg=0;
ULONG ad_data_buf=0, ad_data_sum=0;
WORD  ad_data_l=0,ad_data_h=0;
BYTE 	_10ms_ad_cnt=0,ion_disp_cnt=0;

// 켈리브레이션
BYTE	ion_i_cal_cnt=0,ion_v_cal_cnt=0;

// 전압값 정정
ULONG V_GAIN_M=139;	
ULONG V_GAIN_D=10;

// 전류값 정정
ULONG I_GAIN_M=660; // 1493;   //  260223 1350;
ULONG I_GAIN_D=1000;
ULONG I_GAIN_OFFSET=70;

WORD 	pclean_out_cnt=0;	
WORD 	ion_stop_cnt=0, error_stop_cnt=0, flow_over_cnt=0, temp_over_cnt=0;	
BYTE 	new_active=0,old_active=0;

// 세정
BYTE 	auto_clean_fg=0, first_clean_en_fg=1;
BYTE	after_clean_fg=1;	//LED_type에서 Power off 시키기 위해 after_clean_fg 변수 추가   ????
BYTE 	byWarning_delay_cnt=0;
BYTE 	voice_add=0,old_voice_add=0;

// PID 변수
ULONG	set_pi_ref=0;
long 	error_0_old=0;
long 	error_0=0;

long 	GAIN_P=70 ; // 빠르게 35;	//
long 	GAIN_I=1;		//
ULONG temp_pi_ref;
long 	pi_value;

// 필터 EEPROM 초기화 변수
BYTE	eeprom_test=0,eeprom_save_fg=0;

// 필터 EEPROM 초기화 값
BYTE	flow_error=0, flow_error_cnt=0;
BYTE	temp_error=0, temp_error_cnt=0, t_err_clear_cnt=0;	//080129 온도에라 추가
BYTE  ph_disp_fg=1;

WORD	ph_value=0,ph_temp=0;
BYTE 	orp_value=0,orp_temp=0;

BYTE 	flow_out_fg=0, ion_ok_fg=0,orp_minus_fg=0;
BYTE 	ion_state=1, ion_state_old=0;
WORD	ph_disp_cnt=0, ph_cnt_const=0;
BYTE  eep1_err_cnt=0,eep2_err_cnt=0;	
BYTE 	sound_out_fg=0,display_on_fg=0,life_save_fg=0,life_check_fg=0;

ULONG serial_number=0,serial_number1=0;
BYTE 	serial_err=0,serial_err1=0;

BYTE	f1_life_save_fg=0, f2_life_save_fg=0, f1_life_check_fg=0, f2_life_check_fg=0;
BYTE	f1_read_error_cnt=0, f2_read_error_cnt=0;
BYTE	bySerial_check_fg=1;

WORD 	smps_protect_cnt=0, smps_clear_cnt=0;	
     	
BYTE 	flicker_cnt=0,eep_save_fg=0,ion_speed_cnt=0, ionizing_cnt=0;
	  	BYTE 	flicker_cnt1=0;
BYTE 	uParamAddr;
BYTE 	save_data[FLASH_SIZE]={0,};

//key
WORD 	keyval=0;
BYTE 	input_data[3]={0,0,0};
BYTE 	dec_en_fg=0,inc_en_fg=0;

//touch key 260223
WORD 	key_mode_delay_cnt=0;
BYTE 	fg=0;
BYTE 	reg_w_data[10];
BYTE 	reg_r_data[22];
BYTE 	byte_wr_data,byte_rd_data,WAddress,RAddress,NbData;
BYTE 	cmd_bit,ack=0;
BYTE 	input_buffer[3]={0,0,0};
BYTE 	input_count[3]={0,0,0};

BYTE 	alka_max_fg=0,smps_protect_fg=0;
     	
WORD 	rotate_speed_cnt=0,rotate_speed_value=2;
BYTE 	clean_cnt=0,clean_speed_cnt=0;
     	
BYTE 	bAdcCnt=0,bAd_Ch=0;			
ULONG lAdc_Sum[8]= {0,};
ULONG lAdc_Ave[8]= {0,};

int32_t avg_hi_temp = 0;
int32_t avg_ho_temp = 0;
int32_t avg_cool_temp = 0;
int32_t avg_pelier_temp=0;
int32_t old_pelier_temp=0;

int32_t hi_temp= 0;
int32_t ho_temp= 0;
int32_t cool_temp= 0;
int32_t cool_temp_sum=0;
uint8_t bCoolSumCnt=0;
uint8_t bCoolT_AveCnt=200;
int32_t wCoolTemper=0;
ULONG avg_cool_temp_sum=0;
WORD adc_cool_temp =0;
BYTE bCoolLowTempCnt = 0;    // 냉수 이상 저온 연속 카운터
BYTE bCoolLowTempFg  = 0;    // 냉수 이상 저온 에러 래치

ULONG lVolt_Ave=0;
ULONG lCurr_Ave=0;
ULONG lTemp_Ave=0;

uint8_t bCoolCon_Start = 0;
uint16_t bCoolStartWaitCnt=0;
uint8_t  bCoolFg = 0;
uint16_t wCoolReCnt = 0;
uint8_t bCoolReFg=0;

int16_t nPeltierPwm = 0;    // 현재 제어 중인 Peltier PWM 값
int16_t nActualPwm=0;
int16_t nActualFanPwm =0;
int16_t nFanPwm=0;
int16_t nTargetAdc = 0;     // 현재 구간의 목표 ADC 값
int16_t nBasePwm = 0;       // 구간 진입 시 초기 기준 PWM 값
int16_t nError = 0;         // 오차 (목표 - 현재)
int16_t nStep = 0;          // PWM 수정량
int16_t  nRampStep = 200; 

uint32_t lCoolRunTick = 0;   // 연속 동작 틱 카운터
uint32_t lCoolRestTick = 0;
uint32_t bInRestPeriod=0;
uint32_t wCoolTempCnt=0;
BYTE bCoolReadyFg = 0;



uint32_t 	eep_data=0;

#if FILTER_WRITER
// 필터 칩 쓰기 치구
const ULONG filter_wr_tbl[5] = { FW_VAL_0, FW_VAL_1, FW_VAL_2, FW_VAL_3, FW_VAL_4 };
BYTE  bFwSel   = 0;       // 선택 인덱스 0~4
BYTE  bFwState = 0;       // 0 대기 , 1 쓰는중 , 2 완료 , 3 실패
BYTE  bFwErr1  = 0;       // 필터1 원인코드 (0 = 정상)
BYTE  bFwErr2  = 0;       // 필터2 원인코드 (0 = 정상)
ULONG lFwValue = 0;       // 쓸 값 (mL)
ULONG lFwRead1 = 0;       // 검증 읽기 값
ULONG lFwRead2 = 0;
BYTE  bFwDispMode = 0;    // 0 = 값 표시 , 1 = UID 확인 표시
BYTE  bFwUidRdCnt = 0;    // UID 갱신 분주
BYTE  bFwRbClrCnt = 0;    // 되쓰기 기록 초기화 표시 잔여 (10ms 단위)
WORD  wFwUid1 = 0;        // 필터1 UID 16바이트 합 % 1000
WORD  wFwUid2 = 0;        // 필터2 UID 16바이트 합 % 1000
#endif

// 되쓰기(롤백) 차단 기록 : 슬롯별로 최근 RB_HIST 개 칩의 chip_id 와 최소 잔량(L)
ULONG	lRbId1[RB_HIST]={0,}, lRbId2[RB_HIST]={0,};
WORD	wRbLife1[RB_HIST]={0,}, wRbLife2[RB_HIST]={0,};
BYTE	bRbIdx1=0, bRbIdx2=0;
WORD	wRbMagic=0;
BYTE 	language_num=0,temp_language_jump=0;
WORD 	eep_data_fg=0;

BYTE temp_protect_fg=0, temp_error_out_cnt=0;
BYTE temp_err_disp_cnt=0;
WORD temp_protect_cnt=0;
BYTE f1_save_cnt=0,f2_save_cnt=0;

uint16_t adcval[6];

//260223
BYTE bTouchFg = 0;
WORD bTouchCnt = 0;
WORD wTouchNum=7;
WORD prev_sw_data = 0;
BYTE ttt=2;

BYTE bSet_mL=0; //0=120ml
BYTE bSet_mL_Old=0;
WORD bDisp_mL[3] = { 120 ,250 , 500};
WORD wML_Cnt[3] = { 312 ,700 , 1470}; //325=120
WORD wML_Cnt_Hot[3] = { 335 ,780 , 1580}; //340=120

WORD ml_hold_cnt = 0;
BYTE     ml_pressed = 0;
BYTE     ml_long_sent = 0;

WORD hot_hold_cnt = 0;
BYTE     hot_pressed = 0;
BYTE     hot_long_sent = 0;
WORD wHot_out_cnt =0;
WORD wHot_stop_cnt =0;

BYTE clean_pressed = 0;
BYTE clean_long_sent = 0;
WORD clean_hold_cnt = 0;


BYTE bCoolTempStatus=0;
BYTE bCoolRunFg = 0;
BYTE bCoolRetryFg = 0;  


WORD wPwmPump_data = 0;

BYTE bl_phase = 0;		
		
		
BYTE bWaitCnt = 0;
BYTE bWaitFg=0;

//int16_t Kp=15; //		float Kp = 1.5f; 
//int16_t Ki=2;		//float Ki = 0.2f; 

int16_t Kp=2;
int16_t Ki=1;


long lFlowHot_error=0;  
long lPump_rpm_init = 700;
long lFlowHot_integral_error=0;
long lPump_rpm=0;


long target_flow=0;
ULONG lHotPulseBuf[HOT_FLOW_WIN] = {0,};   // 100ms 펄스 이동창
BYTE  bHotPulseIdx  = 0;
ULONG lHotPulseSum  = 0;
long  flow_hot_meas = 0;               


BYTE  bHeaterDutyCnt = 0;
BYTE  bHeaterDuty    = 100;   // 0~100 %
long  lHotTempErr    = 0;
long  lHotTempInteg  = 0;

//WORD HOT_FLOW[3] = { 652 , 632 , 430};
WORD HOT_FLOW[3] = { 700 , 632, 430};  
 
WORD HOT_PWM_INIT[3] ={700 ,700,700};
	// PID 제어 게인
#define KP 		5   // 비례 게인
#define KI 		1   // 적분 게인


uint8_t bFirstRun = 1; // 초기값 설정을 위한 플래그
uint8_t bFirstCnt=0;
// 유량 매핑을 위한 상수 (기울기 m = 1.35)
#define FLOW_SLOPE    17937L
#define FLOW_OFFSET   430
long PWM_OFFSET = 700;



BYTE bInitialCooling = 1;

uint32_t wUvcCorkCnt=0;
uint32_t wUvcCoolCnt=0;

BYTE bUvcCorkFg=0;
BYTE bUvcCoolFg=0;


BYTE bDoorOpen_20SecFg = 0;
BYTE bFlushingStep = 0;
BYTE bFlushHeaterFg = 0;    
BYTE bFlushingEnd=0;
BYTE bFlushingEndCnt=0;
WORD wFlushingCnt=0;

// 출수 시작 지연
BYTE bStartDelayFg  = 0;   // 1 = 출수 시작을 기다리는 중
BYTE bStartDelayCnt = 0;   // 100ms 카운트
WORD test=0;

uint32_t lDispenseTick = 0;      // 연속 출수 시간 카운트
uint32_t lDispenseRestTick = 0;  // 출수 제한 휴식 카운트
uint8_t  bDispenseRestPeriod = 0; // 0:정상, 1:출수제한(휴식) 중

uint16_t wKeyInput_10SecCnt = 0;
uint8_t bKeyInputFg = 0;

uint16_t wModeSetOut_10SecCnt = 0;
uint8_t  bModeSetOutFg = 0;
			
uint32_t lError=0;
uint32_t  lError_Old=0;
//uint16_t wError = 0;
//uint16_t  wError_Old=0;
uint16_t wFlowCheckDelayTick = 0;   // 출수 후 5초 대기용 카운터
uint16_t wFlowErrorDurationTick = 0; // 에러 상태 3초 지속 체크용 카운터
uint16_t ERROR_FLOW_TICK =6;

WORD set_uv_stop_time=0,set_uv_run_time=0,set_uv_cork_sec_time=0;
WORD save_uv_stop_time=0,save_uv_run_time=0,save_uv_cork_sec_time=0;
ULONG con_uv_stop_time=0,con_uv_run_time=0,con_uv_cork_sec_time=0; 
ULONG con_uv_time=0;
BYTE save_uv_cork_onoff = 0;
BYTE set_uv_cork_onoff = 0;

// main.c:617-618
BYTE con_brightness = 100;      // 255 → 100
BYTE save_brightness = 100;     // 255 → 100
BYTE con_deactivate  = 20;      // 0   → 20   (사양 "비활성 20%")
BYTE save_deactivate = 20;      // 255 → 20

BYTE con_deact_show  = DEACT_SHOW_DEF;    // 0=비활성 버튼 소등, 1=con_deactivate 밝기로 표시
BYTE save_deact_show = DEACT_SHOW_DEF;
//BYTE con_brightness = 255;
//BYTE save_brightness = 255;
//BYTE con_deactivate = 0;
//BYTE save_deactivate = 255;
BYTE con_hot_over_temp  = HOT_OVER_T_DEF;   //  90~110 
BYTE save_hot_over_temp = HOT_OVER_T_DEF;
BYTE con_hot_over_sec   = HOT_OVER_S_DEF;   //초  1~15  
BYTE save_hot_over_sec  = HOT_OVER_S_DEF;
BYTE con_touch_sens     = TOUCH_SENS_DEF;  //     0~15   
BYTE save_touch_sens    = TOUCH_SENS_DEF;

//터치 감도 변경 반영 (손 뗀 뒤에 적용해야 베이스라인이 안 깨짐) 
BYTE bTouchSensApplyFg  = 0;
WORD wTouchSensApplyCnt = 0;


//DFT  
float xhalf,y;
int j;

int32_t lBFRealData[SAMPLE_CNT] = {0};
int32_t lBFImagData[SAMPLE_CNT] = {0};

int32_t dDftRealSum = 0;
int32_t dDftImagSum = 0;

volatile int32_t dBufDftRealSum = 0;
volatile int32_t dBufDftImagSum = 0;
int32_t real_part = 0;
int32_t imag_part = 0;
float mag_sq =0;
uint32_t magnitude=0;
uint8_t bSamplingVal = 0;
volatile uint8_t b1CycleFg = 0;
int32_t wAdcConvValue=0;

int32_t lHGain=1000;
BYTE bHeaterAdcEnd=0;

BYTE bHeaterFlowCnt=0;
BYTE bHeaterFg=0;
BYTE bHeaterOverCnt=0;
BYTE bHeaterStartFg = 0;

BYTE bRestoreMute = 0;


BYTE 	bManualCleanFg = 0;  
BYTE 	bSetIntroFg = 0;


/* Private define ------------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/

void mainloop(void);
int main (void);
//void HT16K33_WriteCmd(BYTE cmd);
//void HT16K33_WriteRamAll(BYTE *pRam);

void HT16D33_WriteCmd(BYTE cmd);

// 명령어 + 1바이트 파라미터 전송 (초기화 및 설정용)
void HT16D33_WriteCmdParam(BYTE cmd, BYTE param);

// RAM 데이터 전체 업데이트
void HT16D33_WriteRamAll(BYTE *pRam);
void HT16D33_WriteLedCtrlAll(BYTE val)  ;

// void HT16K33_WriteCmd(BYTE cmd);
// void HT16K33_WriteRamAll(void);
/* Private variables ---------------------------------------------------------*/
// Timer1n PWM demo configuration structure definition

/*******************************************************************************
* Private Variable
*******************************************************************************/

static TIMER1n_CFG_Type TIMER1n_Config;

/* Public Function ------------------------------------------------------------*/
// Timer1n PWM demo configuration structure definition
typedef struct {
	// base address
	TIMER1n_Type *pTIMER1x;
	// IRQ number
	IRQn_Type TIMER1x_IRQ;
 	// Output port
	PORT_Type *pOutputPort_Group;
	uint32_t OutputPort_Number;
	PCU_ALT_FUNCTION_Type OutputPort_AltFunc;
    uint8_t timer_name[7];
} TIMER1n_PWM_Type;

/*******************************************************************************
* Private Variable
*******************************************************************************/

//-------------------------------------------------------------------------------
//		Time_PWM_Init
//-------------------------------------------------------------------------------
static const TIMER1n_PWM_Type TIMER1n_PWM_CFG[2] = {
	{TIMER14, TIMER14_IRQn, (PORT_Type *)PF, 11, PCU_ALT_FUNCTION_1, "TIMER14"},     // 4
	{TIMER13, TIMER13_IRQn, (PORT_Type *)PF, 10, PCU_ALT_FUNCTION_1, "TIMER13"},   
};


static TIMER1n_CFG_Type TIMER1n_Config;
static TIMER1n_CFG_Type TIMER13_Config;
const TIMER1n_PWM_Type * pConfig;
const TIMER1n_PWM_Type * pConfigCool;

//static TIMER3n_CFG_Type TIMER3n_Config;
//static TIMER3n_OUTPUT_CFG_Type TIMER3n_OutputA_Config;
//static TIMER3n_OUTPUT_CFG_Type TIMER3n_OutputB_Config;
//static TIMER3n_HiZ_CFG_Type TIMER3n_HiZ_Config;

//timer 2n
typedef struct {
	TIMER2n_Type *pTIMER2x;
	IRQn_Type TIMER2x_IRQ;
	PORT_Type *pOutputPort_Group;
	uint32_t OutputPort_Number;
	PCU_ALT_FUNCTION_Type OutputPort_AltFunc;
} TIMER2n_PWM_Type;

static const TIMER2n_PWM_Type TIMER2n_PWM_CFG[2] = {
	{TIMER20, TIMER20_IRQn, PC, 0, PCU_ALT_FUNCTION_1},
	{TIMER21, TIMER21_IRQn, PC, 1, PCU_ALT_FUNCTION_1}
};


static TIMER2n_CFG_Type TIMER20_Config;
static TIMER2n_CFG_Type TIMER21_Config;
const TIMER2n_PWM_Type * pConfig_HotPump;
const TIMER2n_PWM_Type * pConfig_Fan;

void TIMER14_Pwm(void) {


	// Timer1n and clock source setting
	HAL_SCU_Timer1n_ClockConfig(T1NCLK_PCLK);

	TIMER1n_Config.StartSync = DISABLE;
	TIMER1n_Config.ClearSync = DISABLE;
	TIMER1n_Config.ExtClock = DISABLE; // PCLK 32MHz
	TIMER1n_Config.OutputPolarity = TIMER1n_OUTPUT_POLARITY_HIGH;
	TIMER1n_Config.MatchInterrupt = DISABLE;
	TIMER1n_Config.CaptureInterrupt = DISABLE;//DISABLE;

	TIMER1n_Config.PrescalerData =4;// 48 - 1; // 48MHz / (1+4) = 48MHz -> 1/48000000 us  220704 0->4


	pConfig = &(TIMER1n_PWM_CFG[0]);


	HAL_TIMER1n_Stop(pConfig->pTIMER1x);
	NVIC_DisableIRQ(pConfig->TIMER1x_IRQ);
	HAL_TIMER1n_DeInit(pConfig->pTIMER1x);

	// Timer1n output port setting
	HAL_GPIO_ConfigOutput(pConfig->pOutputPort_Group, pConfig->OutputPort_Number, PCU_MODE_ALT_FUNC);
	HAL_GPIO_ConfigFunction(pConfig->pOutputPort_Group, pConfig->OutputPort_Number, pConfig->OutputPort_AltFunc);

	TIMER1n_Config.AData =5000;//period 1/48MHz*5000 ==104us
	TIMER1n_Config.BData = 0; //duty

	HAL_TIMER1n_Init(pConfig->pTIMER1x, TIMER1n_PWM_MODE, &TIMER1n_Config);

	NVIC_SetPriority(pConfig->TIMER1x_IRQ, 3);
	NVIC_EnableIRQ(pConfig->TIMER1x_IRQ);

	HAL_TIMER1n_Stop(pConfig->pTIMER1x);		
		
}//-------------------------------------------------------------------------------
//		Pwm_off
//-------------------------------------------------------------------------------
void Pwm_off(void)
{
	TIMER1n_Config.BData = 0;
	HAL_TIMER1n_Init(pConfig->pTIMER1x, TIMER1n_PWM_MODE, &TIMER1n_Config);
	HAL_TIMER1n_Stop(pConfig->pTIMER1x);	
}
//-------------------------------------------------------------------------------
//		Pwm_out
//-------------------------------------------------------------------------------
void Pwm_out(WORD pwm_data)
{
	if(pwm_data>=5000)	pwm_data = 4999;
	TIMER1n_Config.BData =  pwm_data; // DUTY = GRA
	HAL_TIMER1n_Init(pConfig->pTIMER1x, TIMER1n_PWM_MODE, &TIMER1n_Config);
    HAL_TIMER1n_Start(pConfig->pTIMER1x);		
}


void TIMER13_Pwm(void) {

	// timer13 cool pwm
	HAL_SCU_Timer1n_ClockConfig(T1NCLK_PCLK);

	TIMER13_Config.StartSync = DISABLE;
	TIMER13_Config.ClearSync = DISABLE;
	TIMER13_Config.ExtClock = DISABLE; // PCLK 32MHz
	TIMER13_Config.OutputPolarity = TIMER1n_OUTPUT_POLARITY_HIGH;
	TIMER13_Config.MatchInterrupt = DISABLE;
	TIMER13_Config.CaptureInterrupt = DISABLE;//DISABLE;

	TIMER13_Config.PrescalerData =4;// 48 - 1; // 48MHz / (1+4) = 48MHz -> 1/48000000 us  220704 0->4


	pConfigCool = &(TIMER1n_PWM_CFG[1]);

	HAL_TIMER1n_Stop(pConfigCool->pTIMER1x);
	NVIC_DisableIRQ(pConfigCool->TIMER1x_IRQ);
	HAL_TIMER1n_DeInit(pConfigCool->pTIMER1x);

	// Timer1n output port setting
	HAL_GPIO_ConfigOutput(pConfigCool->pOutputPort_Group, pConfigCool->OutputPort_Number, PCU_MODE_ALT_FUNC);
	HAL_GPIO_ConfigFunction(pConfigCool->pOutputPort_Group, pConfigCool->OutputPort_Number, pConfigCool->OutputPort_AltFunc);

	TIMER13_Config.AData =5000;//period 1/48MHz*5000 ==104us
	TIMER13_Config.BData = 0; //duty

	HAL_TIMER1n_Init(pConfigCool->pTIMER1x, TIMER1n_PWM_MODE, &pConfigCool);

	NVIC_SetPriority(pConfigCool->TIMER1x_IRQ, 3);
	NVIC_EnableIRQ(pConfigCool->TIMER1x_IRQ);

	HAL_TIMER1n_Stop(pConfigCool->pTIMER1x);		
		
}
void PwmCool_off(void)
{
	TIMER13_Config.BData = 0;
	HAL_TIMER1n_Init(pConfigCool->pTIMER1x, TIMER1n_PWM_MODE, &TIMER13_Config);
	HAL_TIMER1n_Stop(pConfigCool->pTIMER1x);	
}
void PwmCool_out(WORD pwm_data)
{
	if(pwm_data>=5000)	pwm_data = 4999;
	TIMER13_Config.BData = pwm_data; // DUTY = GRA
	HAL_TIMER1n_Init(pConfigCool->pTIMER1x, TIMER1n_PWM_MODE, &TIMER13_Config);
    HAL_TIMER1n_Start(pConfigCool->pTIMER1x);		
}

void TIMER2n_Init(void)
{
	HAL_SCU_Timer20_ClockConfig(T20CLK_PCLK);

	//timer20 hotpumpt
	TIMER20_Config.ExtClock = DISABLE; // PCLK 32MHz
	TIMER20_Config.PrescalerData = 32 - 1; // 32MHz / 32 = 1MHz -> 1us
	TIMER20_Config.Mode = TIMER2n_MODE_PWM;
	TIMER20_Config.OutputPolarity = TIMER2n_OUTPUT_POLARITY_HIGH;
	TIMER20_Config.MatchInterrupt = DISABLE;
	TIMER20_Config.CaptureInterrupt = DISABLE;


	pConfig_HotPump = &(TIMER2n_PWM_CFG[0]);

	// Timer1n output port setting
	HAL_GPIO_ConfigOutput(pConfig_HotPump->pOutputPort_Group, pConfig_HotPump->OutputPort_Number, PCU_MODE_ALT_FUNC);
	HAL_GPIO_ConfigFunction(pConfig_HotPump->pOutputPort_Group, pConfig_HotPump->OutputPort_Number, pConfig_HotPump->OutputPort_AltFunc);

	TIMER20_Config.AData = 0 ;
	TIMER20_Config.BData = 0 ;

	HAL_TIMER2n_Init(pConfig_HotPump->pTIMER2x, &TIMER20_Config);

	NVIC_SetPriority(pConfig_HotPump->TIMER2x_IRQ, 3);
	NVIC_EnableIRQ(pConfig_HotPump->TIMER2x_IRQ);

	HAL_TIMER2n_Stop(pConfig_HotPump->pTIMER2x);

	TIMER21_Config.ExtClock = DISABLE; // PCLK 32MHz
	TIMER21_Config.PrescalerData = 5 - 1; // 32MHz / 32 = 1MHz -> 1us
	TIMER21_Config.Mode = TIMER2n_MODE_PWM;
	TIMER21_Config.OutputPolarity = TIMER2n_OUTPUT_POLARITY_HIGH;
	TIMER21_Config.MatchInterrupt = DISABLE;
	TIMER21_Config.CaptureInterrupt = DISABLE;


	pConfig_Fan = &(TIMER2n_PWM_CFG[1]);

	// Timer1n output port setting
	HAL_GPIO_ConfigOutput(pConfig_Fan->pOutputPort_Group, pConfig_Fan->OutputPort_Number, PCU_MODE_ALT_FUNC);
	HAL_GPIO_ConfigFunction(pConfig_Fan->pOutputPort_Group, pConfig_Fan->OutputPort_Number, pConfig_Fan->OutputPort_AltFunc);

	TIMER21_Config.AData = 0 ;
	TIMER21_Config.BData = 0;

	HAL_TIMER2n_Init(pConfig_Fan->pTIMER2x, &TIMER21_Config);

	NVIC_SetPriority(pConfig_Fan->TIMER2x_IRQ, 3);
	NVIC_EnableIRQ(pConfig_Fan->TIMER2x_IRQ);

	HAL_TIMER2n_Stop(pConfig_Fan->pTIMER2x);
}
void PwmHotPump_off(void)
{

	TIMER20_Config.AData = 0;
	TIMER20_Config.BData = 0;
	//HAL_TIMER2n_SetAData(TIMER20, TIMER20_Config.AData);
	//HAL_TIMER2n_SetBData(TIMER20, TIMER20_Config.BData);
	HAL_TIMER2n_Stop(TIMER20);
//	HAL_TIMER2n_Init(pConfig_HotPump->pTIMER2x,  &TIMER20_Config);
//	HAL_TIMER2n_Stop(pConfig_HotPump->pTIMER2x);	
}
void PwmHotPump_out(WORD pwm_data)
{
	
	if(pwm_data>=2000)	pwm_data = 1999;
	
	TIMER20_Config.AData = 2000; //  peroid
	TIMER20_Config.BData = pwm_data;  ///  DUTY
	
	//HAL_TIMER2n_SetAData(TIMER20, TIMER20_Config.AData);
	//HAL_TIMER2n_SetBData(TIMER20, TIMER20_Config.BData);
	//HAL_TIMER2n_Start(TIMER20);
    HAL_TIMER2n_Init(pConfig_HotPump->pTIMER2x, &TIMER20_Config);
    HAL_TIMER2n_Start(pConfig_HotPump->pTIMER2x);
}
void PwmCoolFan_off(void)
{

	TIMER21_Config.AData = 0;
	TIMER21_Config.BData = 0;
	//HAL_TIMER2n_SetAData(TIMER21, TIMER20_Config.AData);
	//HAL_TIMER2n_SetBData(TIMER21, TIMER20_Config.BData);
	HAL_TIMER2n_Stop(TIMER21);	
}
void PwmCoolFan_out(WORD pwm_data)
{
	
	if(pwm_data>=5000)	pwm_data = 4999;
	
	TIMER21_Config.AData = 5000; //  peroid
	TIMER21_Config.BData = pwm_data;  ///  DUTY
    HAL_TIMER2n_Init(pConfig_Fan->pTIMER2x, &TIMER21_Config);
    HAL_TIMER2n_Start(pConfig_Fan->pTIMER2x);
}


/*
void TIMER30_Pwm(void) {

	HAL_TIMER3n_DeInit(TIMER30);


	// Set PWM pin
	// PWM30BA(PE2)
	//HAL_GPIO_ConfigOutput(PE, 2, PCU_MODE_ALT_FUNC);
	//HAL_GPIO_ConfigFunction(PE, 2, PCU_ALT_FUNCTION_1);
	// PWM30BB(PE3)
	HAL_GPIO_ConfigOutput(PE, 3, PCU_MODE_ALT_FUNC);
	HAL_GPIO_ConfigFunction(PE, 3, PCU_ALT_FUNCTION_1);
	// PWM30CA(PE4)
	HAL_GPIO_ConfigOutput(PE, 4, PCU_MODE_ALT_FUNC);
	HAL_GPIO_ConfigFunction(PE, 4, PCU_ALT_FUNCTION_1);


	// Initialize Motor PWM
	HAL_SCU_Timer30_ClockConfig(T30CLK_PCLK);

	TIMER3n_Config.ExtClock = DISABLE;
	TIMER3n_Config.Mode = TIMER3n_MODE_BACKTOBACK;
	TIMER3n_Config.OutputMode = TIMER3n_OUTPUT_MODE_6_CHANNEL;
	TIMER3n_Config.DelayInsertion = DISABLE;
	TIMER3n_Config.DelayPosition = TIMER3n_DELAY_POSITION_FRONT;
	TIMER3n_Config.CapturePolarity = TIMER3n_CAPTURE_POLARITY_RISING;
	TIMER3n_Config.ReloadTime = TIMER3n_DATA_RELOAD_TIME_MATCH;
	TIMER3n_Config.MatchIntInterval = TIMER3n_MATCH_INT_INTERVAL_1;
	TIMER3n_Config.PrescalerData = 31; // 32MHz / (31 + 1) = 1MHz
	TIMER3n_Config.PData = 1000; // 10kHz, 100us(Back to Back)
	TIMER3n_Config.AData = 300; // 30.0%
	TIMER3n_Config.BData = 500; // 50.0%
	TIMER3n_Config.CData = 700; // 70.0%
	TIMER3n_Config.DelayData = 0; // (0 + 1) / 1MHz = 1us
	HAL_TIMER3n_Init(TIMER30, &TIMER3n_Config);

	// Setting High Impedence control
	TIMER3n_OutputA_Config.StartLevel = TIMER3n_LEVEL_HIGH;
	TIMER3n_OutputA_Config.Output_PWM30Ax = DISABLE;
	TIMER3n_OutputA_Config.Output_PWM30Bx = DISABLE;
	TIMER3n_OutputA_Config.Output_PWM30Cx = DISABLE;
	TIMER3n_OutputA_Config.DisableLevel_PWM30Ax = TIMER3n_LEVEL_LOW;
	TIMER3n_OutputA_Config.DisableLevel_PWM30Bx = TIMER3n_LEVEL_LOW;
	TIMER3n_OutputA_Config.DisableLevel_PWM30Cx = TIMER3n_LEVEL_LOW;
	TIMER3n_OutputB_Config.StartLevel = TIMER3n_LEVEL_HIGH;
	TIMER3n_OutputB_Config.Output_PWM30Ax = DISABLE;
	TIMER3n_OutputB_Config.Output_PWM30Bx = ENABLE;
	TIMER3n_OutputB_Config.Output_PWM30Cx = DISABLE;
	TIMER3n_OutputB_Config.DisableLevel_PWM30Ax = TIMER3n_LEVEL_HIGH;
	TIMER3n_OutputB_Config.DisableLevel_PWM30Bx = TIMER3n_LEVEL_HIGH;
	TIMER3n_OutputB_Config.DisableLevel_PWM30Cx = TIMER3n_LEVEL_HIGH;
	HAL_TIMER3n_ConfigureOutput(TIMER30, TIMER3n_OUTPUT_CTRL_ALL, &TIMER3n_OutputA_Config, &TIMER3n_OutputB_Config);

	TIMER3n_HiZ_Config.HiZ = DISABLE;
	TIMER3n_HiZ_Config.BLNK_Edge = TIMER3n_BLNK_EDGE_FALLING;
	HAL_TIMER3n_ConfigureHiZ(TIMER30, &TIMER3n_HiZ_Config);
	HAL_TIMER3n_SetHiZOutput(TIMER30, RESET);

	//HAL_TIMER3n_ConfigureInterrupt(TIMER30, (TIMER3n_INTERRUPT_Type)(TIMER3n_INTERRUPT_PMATCH_INT \
	//															| TIMER3n_INTERRUPT_BOTTOM_INT \
	//															| TIMER3n_INTERRUPT_AMATCH_INT \
	//															| TIMER3n_INTERRUPT_BMATCH_INT \
	//															| TIMER3n_INTERRUPT_CMATCH_INT), ENABLE);
	NVIC_SetPriority(TIMER30_IRQn, 3);
	NVIC_EnableIRQ(TIMER30_IRQn);
	//__enable_irq();

	//HAL_TIMER3n_ClearStatus(TIMER30, (TIMER3n_STATUS_Type)TIMER3n_INTFLAG_STATUS_CLEAR_MASK);	// Clear Interrupt Flag

	// Clear Counter value and Start Motor PWM
	HAL_TIMER3n_ClearCounter(TIMER30);
	HAL_TIMER3n_Start(TIMER30);


}
void PwmHotPump_off(void)
{

	TIMER3n_Config.BData = 0;
	HAL_TIMER3n_Init(TIMER30,  &TIMER3n_Config);
	HAL_TIMER3n_Stop(TIMER30);	
}
void PwmHotPump_out(WORD pwm_data)
{
	TIMER3n_Config.PData = 1000; 
	
	if(pwm_data>=5000)	pwm_data = 4999;
	TIMER3n_Config.BData = pwm_data; // DUTY = GRA
	  HAL_TIMER3n_Stop(TIMER30);

    HAL_TIMER3n_Init(TIMER30, &TIMER3n_Config);

    HAL_TIMER3n_ConfigureOutput(TIMER30, TIMER3n_OUTPUT_CTRL_ALL,
                                &TIMER3n_OutputA_Config, &TIMER3n_OutputB_Config);

    HAL_TIMER3n_ConfigureHiZ(TIMER30, &TIMER3n_HiZ_Config);
    HAL_TIMER3n_SetHiZOutput(TIMER30, RESET);

    HAL_TIMER3n_ClearCounter(TIMER30);
    HAL_TIMER3n_Start(TIMER30);
}*/
/*uint16_t Get_Current_mA(uint16_t adc_val) 
{
uint8_t i;
    uint32_t adc_diff;
    uint32_t ma_diff;
    uint32_t offset;
    uint32_t current_ma;
    // -----------------------------


    if (adc_val <= adc_table_low[0]) {
        return 0 ; //adc_table_low[0]; 
    }
    
    //  부하전류 500mA 
    if (adc_val <= adc_table_low[LOW_TABLE_SIZE - 1]) {
        for (i = 0; i < LOW_TABLE_SIZE - 1; i++) {
            if (adc_val >= adc_table_low[i] && adc_val <= adc_table_low[i+1]) {
                adc_diff = adc_table_low[i+1] - adc_table_low[i];
                ma_diff  = ma_table_low[i+1] - ma_table_low[i];
                offset   = adc_val - adc_table_low[i];
                
                return ma_table_low[i] + (uint16_t)((offset * ma_diff) / adc_diff);
            }
        }
    }
    

    current_ma = ((uint32_t)adc_val * I_GAIN_M) / I_GAIN_D;
    
    return (uint16_t)current_ma;

}*/
uint16_t Get_Current_mA(uint16_t adc) {

	BYTE i;
	long x0;
	 long x1 ;
	 long y0 ;
	 long y1;
	uint16_t curr;
	
    if (adc <= adc_lut[0]) return 0;

    //  0 ~ 1000mA 
    if (adc < I_GAIN_M ) {//660) {
        for ( i = 0; i < (LUT_SIZE - 1); i++) {
            if (adc >= adc_lut[i] && adc <= adc_lut[i + 1]) {
             
                x0 = adc_lut[i];
                x1 = adc_lut[i + 1];
                y0 = current_lut[i];
                y1 = current_lut[i + 1];
                
                return y0 + (adc - x0) * (y1 - y0) / (x1 - x0);
            }
        }
    }

    //  1000mA 이상
    // 수식: 1000 + (현재ADC - 660) * (100 / 60)
    // 100/60
	
	if(adc<I_GAIN_M)  curr=1000;
	else{	
		curr = 1000 + ((adc - I_GAIN_M ) * 100 / 62);
	}
    return curr; //660
}


void SysTick_Handler_func(void) {    // SysTick Interrupt Handler @ 1000Hz
	
	
//	HT16K33_VDimService_1ms();
	if(++by10msCnt>=10)
	{
		by10msCnt = 0;
		byTimer10msFg = 1;
		
	
		
			
		// 유량 계측용 500 msec 
	  	if(++m_n500ms_cnt>=50)
	  	{
			m_n500ms_cnt = 0;
			m_n500ms_ok = 1; 
	  	}
	  	
	  	// 보이스 출력후 일정시간 이후 경보음 출력
	  	if(voice_out_cnt>0)
	  	{
	  		--voice_out_cnt;
	  	}
	  	
	  	if(++flicker_cnt>=FLIKER) {	//100 = 1sec
		  	flicker_cnt=0;
		}
	}
	
	//1ms
		bAd_Ch = 0;		
		HAL_ADC_ConfigureOneSequence(ADC, ADC_SEQUENCE_0, (ADC_CHANNEL_Type)(ADC_CHANNEL_0 ));
		HAL_ADC_Start(ADC);
	

}


void delay_1ms(WORD delay)
{
	lmsec = delay;
//	lmsec *= 2000;//1ms   260223
	lmsec *= 1000;//1ms
	while(lmsec--);
}
void Heater_Current(void)
{
    if(b1CycleFg) {
        b1CycleFg = 0; // 플래그 클리어
        
        //  (1024 스케일링 복원)
        real_part = dBufDftRealSum /lHGain ; //>> 10;
        imag_part = dBufDftImagSum/lHGain ; //>> 10;
        
        mag_sq = (float)real_part * real_part + (float)imag_part * imag_part;
        
        magnitude = (uint32_t)fast_sqrt(mag_sq);
        
        // 진폭  전압 
        //uint32_t amplitude_adc = (magnitude * 2) / SAMPLE_CNT;
        //uint32_t amplitude_mV = (amplitude_adc * 3300) / 4096;
        
     
    }
}

void ADC_IRQHandler_ADC_Interrupt(void) {
	ADC_STATUS_Type status; // check interrupt flag

	status = HAL_ADC_GetStatus(ADC);
	
	// EOC Flag	
	if (status & ADC_STATUS_SINGLE_INT) {
		if(bAd_Ch==0)
		{	
			lAdc_Sum[0] +=  HAL_ADC_GetConversionData(ADC, ADC_DATA_SEL_LAST);
			
			HAL_ADC_ConfigureOneSequence(ADC, ADC_SEQUENCE_0, (ADC_CHANNEL_Type)(ADC_CHANNEL_1 ));
			HAL_ADC_Start(ADC);
			bAd_Ch=1;	
		}
		else if(bAd_Ch==1)
		{	
			lAdc_Sum[1] +=HAL_ADC_GetConversionData(ADC, ADC_DATA_SEL_LAST);
			HAL_ADC_ConfigureOneSequence(ADC, ADC_SEQUENCE_0, (ADC_CHANNEL_Type)(ADC_CHANNEL_2 ));
			HAL_ADC_Start(ADC);
			bAd_Ch=2;	
		}
		else if(bAd_Ch==2)
		{	
			lAdc_Sum[2] +=HAL_ADC_GetConversionData(ADC, ADC_DATA_SEL_LAST);
			HAL_ADC_ConfigureOneSequence(ADC, ADC_SEQUENCE_0, (ADC_CHANNEL_Type)(ADC_CHANNEL_3 ));
			HAL_ADC_Start(ADC);
			bAd_Ch=3;	
		}
		else if(bAd_Ch==3)
		{	
			lAdc_Sum[3] +=HAL_ADC_GetConversionData(ADC, ADC_DATA_SEL_LAST);
			HAL_ADC_ConfigureOneSequence(ADC, ADC_SEQUENCE_0, (ADC_CHANNEL_Type)(ADC_CHANNEL_4 ));
			HAL_ADC_Start(ADC);
			bAd_Ch=4;	
		}
		else if(bAd_Ch==4)
		{	
			lAdc_Sum[4] =HAL_ADC_GetConversionData(ADC, ADC_DATA_SEL_LAST);
			HAL_ADC_ConfigureOneSequence(ADC, ADC_SEQUENCE_0, (ADC_CHANNEL_Type)(ADC_CHANNEL_5 ));
			HAL_ADC_Start(ADC);
			bAd_Ch=5;	
		}
		else if(bAd_Ch==5)
		{	
			lAdc_Sum[5] +=HAL_ADC_GetConversionData(ADC, ADC_DATA_SEL_LAST);
			HAL_ADC_ConfigureOneSequence(ADC, ADC_SEQUENCE_0, (ADC_CHANNEL_Type)(ADC_CHANNEL_6 ));
			HAL_ADC_Start(ADC);
			bAd_Ch=6;	
		}
		else if(bAd_Ch==6)
		{	
			lAdc_Sum[6] +=HAL_ADC_GetConversionData(ADC, ADC_DATA_SEL_LAST);
			HAL_ADC_ConfigureOneSequence(ADC, ADC_SEQUENCE_0, (ADC_CHANNEL_Type)(ADC_CHANNEL_7 ));
			HAL_ADC_Start(ADC);
			bAd_Ch=7;	
		}
		else if(bAd_Ch==7)
		{
			lAdc_Sum[7]  += HAL_ADC_GetConversionData(ADC, ADC_DATA_SEL_LAST);
			
			lAdc_Ave[4]	 = lAdc_Sum[4];
			bHeaterAdcEnd=1;
			
			
			if(++bAdcCnt>=100)
			{
				bAdcCnt=0;
				
				lAdc_Ave[0]  = lAdc_Sum[0];
				lAdc_Ave[1]	 = lAdc_Sum[1];
				lAdc_Ave[2]	 = lAdc_Sum[2];
				lAdc_Ave[3]	 = lAdc_Sum[3];
				//lAdc_Ave[4]	 = lAdc_Sum[4];
				lAdc_Ave[5]	 = lAdc_Sum[5];
				lAdc_Ave[6]	 = lAdc_Sum[6];
				lAdc_Ave[7]	 = lAdc_Sum[7];
				
				 lAdc_Sum[0] = 0;
				 lAdc_Sum[1] = 0;
				 lAdc_Sum[2] = 0;
				 lAdc_Sum[3] = 0;
				// lAdc_Sum[4] = 0;
				 lAdc_Sum[5] = 0;
				 lAdc_Sum[6] = 0;
				 lAdc_Sum[7] = 0;
				
				ad_ok_fg=1;
				
			}	
		}

		HAL_ADC_ClearStatus(ADC, ADC_STATUS_SINGLE_INT);
	}
	
	if(bHeaterAdcEnd){
		bHeaterAdcEnd = 0;
	
		
		// DC 1.65V(2048) 오프셋 제거 
		wAdcConvValue = (int32_t)lAdc_Ave[4] - DC_OFFSET;

		//  Real / Imag
		lBFRealData[bSamplingVal] = CosX[bSamplingVal] * wAdcConvValue;
		lBFImagData[bSamplingVal] = SinX[bSamplingVal] * wAdcConvValue;

		//
		dDftRealSum += lBFRealData[bSamplingVal];
		dDftImagSum += lBFImagData[bSamplingVal];


		if(++bSamplingVal >= SAMPLE_CNT) {
			bSamplingVal = 0;
			b1CycleFg = 1;  // 100ms(1주기 
		}


		dBufDftRealSum = dDftRealSum;
		dBufDftImagSum = dDftImagSum;


		dDftRealSum -= lBFRealData[bSamplingVal];
		dDftImagSum -= lBFImagData[bSamplingVal];
		
	}
}

void ADC_Init(void)
{
	HAL_ADC_PortInit(0);
	HAL_ADC_PortInit(1);
	HAL_ADC_PortInit(2);
	HAL_ADC_PortInit(3);
	HAL_ADC_PortInit(4);
	HAL_ADC_PortInit(5);
	HAL_ADC_PortInit(6);	
	HAL_ADC_PortInit(7);
	
	
	// Set ADC external clock
	HAL_SCU_MiscClockConfig(7, ADC_TYPE, CLKSRC_MCLK, 12); // 48MHz/12 = 4MHz

	// Initialize and configure ADC
	ADC_Config.TriggerInfo = DISABLE;
	ADC_Config.ChannelInfo = ENABLE;
	ADC_Config.Dma = DISABLE;
	ADC_Config.Restart = DISABLE;

	ADC_Config.Mode = ADC_MODE_SINGLE;
	ADC_Config.TriggerSelection = ADC_TRIGGER_SEL_OLNY_SOFT;

	ADC_Config.SamplingTime = 1;
	ADC_Config.SequenceCounter = ADC_SEQ_NUM_1;

	ADC_Config.ExternalClock = ENABLE;
	ADC_Config.InternalClockDivider = 1;

	HAL_ADC_Init(ADC, &ADC_Config);
	HAL_ADC_ConfigureInterrupt(ADC, ADC_INTERRUPT_SINGLE, ENABLE);

	// Enable SysTick and ADC interrupt
	//SysTick_Config(SystemCoreClock / 1000);

	NVIC_SetPriority(ADC_IRQn, 3);
	NVIC_EnableIRQ(ADC_IRQn);
}

//TSM16 setting-------------------------------------------------------------------------------
static void I2C_ChannelConfig(const I2Cn_PORT_Type * pConfig, uint32_t Speed, I2Cn_Type Type) {

	// SCL port
	HAL_GPIO_ConfigOutput(pConfig->pSclPort, pConfig->SclPort_Number, PCU_MODE_ALT_FUNC);
	HAL_GPIO_ConfigFunction(pConfig->pSclPort, pConfig->SclPort_Number, pConfig->SclAltFunc);

	// SDA port
	HAL_GPIO_ConfigOutput(pConfig->pSdaPort, pConfig->SdaPort_Number, PCU_MODE_ALT_FUNC);
	HAL_GPIO_ConfigFunction(pConfig->pSdaPort, pConfig->SdaPort_Number, pConfig->SdaAltFunc);


#ifdef INTERVAL_TEST
    pConfig->pI2Cx->CR |= ((2 & I2C_CR_INTERVAL_Msk) << I2C_CR_INTERVAL_Pos);
#endif

#ifdef SCL_TIMEOUT
	I2C_SetSclLowTimeout(pConfig->pI2Cx, ENABLE, ENABLE, 0xFFFFFF); // SCL Low Time Period : (1/PCLK)*4*(SLTPDR+1) = (1/32MHz)*4*(0xFFFFFF+1)2.09s
#endif

    if (Type == SLAVE) {
        HAL_I2C_SetOwnSlaveAddr0(pConfig->pI2Cx, SLAVE_ADDR, DISABLE);
    }

	HAL_I2C_Init(pConfig->pI2Cx, Speed);
}



void TouchReg_bytewrite(BYTE addr,BYTE dt)
{

    MasterCfg.sl_addr7bit = SLAVE_ADDR; // Slave address

    WriteBuffer[0] = addr &0xff; // reg address
	WriteBuffer[1] =dt;
    MasterCfg.tx_data =WriteBuffer;
    MasterCfg.tx_length =2;
	
    // I2C transmission 
    HAL_I2C_MasterTransmitData(Master->pI2Cx, &MasterCfg, I2C_TRANSFER_POLLING);
	delay_1ms(10);
}

void TouchReg_Init(void)
{
	
	BYTE sens;

    /* 2ch/reg, 채널당 4bit → 상·하위 니블에 동일 값 */
    sens = (BYTE)((con_touch_sens << 4) | con_touch_sens);
	
	delay_1ms(200);

	//address , data
	TouchReg_bytewrite(0x09,0x0b) ;// soft reset
	delay_1ms(100);

	TouchReg_bytewrite(0x02,sens) ;//Sensivity Register  Ch 1 ~ Ch2
	TouchReg_bytewrite(0x03,sens) ;//Sensivity Register  Ch 3 ~ Ch4
	TouchReg_bytewrite(0x04,sens) ;//Sensivity Register  Ch 5 ~ Ch6
	TouchReg_bytewrite(0x05,sens) ;//Sensivity Register  Ch 7 ~ Ch8
	TouchReg_bytewrite(0x06,0xff) ;//Sensivity Register  Ch 9 ~ Ch10
	TouchReg_bytewrite(0x07,0xff) ;//Sensivity Register  Ch 11 ~ Ch12
	TouchReg_bytewrite(0x22,0xff) ;//Sensivity Register  Ch 13 ~ Ch14
	TouchReg_bytewrite(0x23,0xff) ;//Sensivity Register  Ch 15~ Ch16
	
	TouchReg_bytewrite(0x08,0x3A) ;//ctrl1
	TouchReg_bytewrite(0x0A,0x00) ;//
	TouchReg_bytewrite(0x0B,0x00) ;//
	TouchReg_bytewrite(0x0C,0x00) ;//Channel On/Off Control Register (Ch1 ~Ch8) ,Bit "0" = Chnael ON , Bit "1" = Chnael OFF
	TouchReg_bytewrite(0x0D,0xff) ;//Channel On/Off Control Register (Ch9 ~Ch16) ,Bit "0" = Chnael ON , Bit "1" = Chnael OFF
	TouchReg_bytewrite(0x0E,0x00) ;//Channel Calibration Control Register (Ch1 ~Ch8) 
	TouchReg_bytewrite(0x0F,0x00) ;//Channel Calibration Control Register (Ch9 ~Ch16)
	
	/*TouchReg_bytewrite(0x24,0x1f) ;
	TouchReg_bytewrite(0x25,0x14) ;
	TouchReg_bytewrite(0x26,0x1f) ;
	TouchReg_bytewrite(0x27,0x14) ;
	
	TouchReg_bytewrite(0x42,0x12) ;
	TouchReg_bytewrite(0x43,0x77) ;*/
	
	TouchReg_bytewrite(0x09,0x03) ;// soft reset
	delay_1ms(200);
	
}


void TouchData_Read(BYTE addr,BYTE *RdData,BYTE len)
{

    Buffer_Init(WriteReqBuffer, 0, 2);
	MasterCfg.sl_addr7bit = SLAVE_ADDR; // Slave address

    WriteReqBuffer[0] = addr&0xff; //address
    MasterCfg.tx_data = WriteReqBuffer;
    MasterCfg.tx_length = 1;

    // I2C transmission 
    HAL_I2C_MasterTransmitData(Master->pI2Cx, &MasterCfg, I2C_TRANSFER_POLLING);
	
	Buffer_Init(ReadBuffer, 0, BUFFER_SIZE);
    MasterCfg.sl_addr7bit = SLAVE_ADDR;
    MasterCfg.rx_data = RdData;
	
    MasterCfg.rx_length = len;

    HAL_I2C_MasterReceiveData(Master->pI2Cx, &MasterCfg, I2C_TRANSFER_POLLING);

}

//16K33 setting-------------------------------------------------------------------------------
static void I2C0_ChannelConfig(const I2Cn_PORT_Type * pConfig, uint32_t Speed, I2Cn_Type Type) {

	// SCL port
	HAL_GPIO_ConfigOutput(pConfig->pSclPort, pConfig->SclPort_Number, PCU_MODE_ALT_FUNC);
	HAL_GPIO_ConfigFunction(pConfig->pSclPort, pConfig->SclPort_Number, pConfig->SclAltFunc);

	// SDA port
	HAL_GPIO_ConfigOutput(pConfig->pSdaPort, pConfig->SdaPort_Number, PCU_MODE_ALT_FUNC);
	HAL_GPIO_ConfigFunction(pConfig->pSdaPort, pConfig->SdaPort_Number, pConfig->SdaAltFunc);


#ifdef INTERVAL_TEST
    pConfig->pI2Cx->CR |= ((2 & I2C_CR_INTERVAL_Msk) << I2C_CR_INTERVAL_Pos);
#endif

#ifdef SCL_TIMEOUT
	I2C_SetSclLowTimeout(pConfig->pI2Cx, ENABLE, ENABLE, 0xFFFFFF); // SCL Low Time Period : (1/PCLK)*4*(SLTPDR+1) = (1/32MHz)*4*(0xFFFFFF+1)2.09s
#endif

    if (Type == SLAVE) {
   //     HAL_I2C_SetOwnSlaveAddr0(pConfig->pI2Cx, SLAVE_ADDR_16K33, DISABLE);
    }

	HAL_I2C_Init(pConfig->pI2Cx, Speed);
}/*
void HT16D33_WriteLedCtrlAll(BYTE val)   // 0xFF = 전 도트 마스크 해제
{
    BYTE i;
    HT16D33_WriteCmdParam(HT16D33_CMD_RAM_PAGE_ADDR, 0x00);

    DisplayCfg.sl_addr7bit = SLAVE_ADDR_16D33;
    Disp_WriteBuffer[0] = 0x84;   // Write LED Control Data
    Disp_WriteBuffer[1] = 0x00;   // 시작 주소
    for (i = 0; i < 18; i++) Disp_WriteBuffer[2 + i] = val;   // 12x12 packed = 18바이트

    DisplayCfg.tx_data   = Disp_WriteBuffer;
    DisplayCfg.tx_length = 18 + 2;
    HAL_I2C_MasterTransmitData(I2C_DISP->pI2Cx, &DisplayCfg, I2C_TRANSFER_POLLING);
}*/
// 1바이트 단일 명령어 전송 (예: Soft Reset)
void HT16D33_WriteCmd(BYTE cmd)
{
    DisplayCfg.sl_addr7bit = SLAVE_ADDR_16D33; // HT16D33의 실제 주소(예: 0x60)로 정의 필요
    Disp_WriteBuffer[0] = cmd;
    DisplayCfg.tx_data   = Disp_WriteBuffer;
    DisplayCfg.tx_length = 1;
    HAL_I2C_MasterTransmitData(I2C_DISP->pI2Cx, &DisplayCfg, I2C_TRANSFER_POLLING);
}

// 명령어 + 1바이트 파라미터 전송 (초기화 및 설정용)
void HT16D33_WriteCmdParam(BYTE cmd, BYTE param)
{
    DisplayCfg.sl_addr7bit = SLAVE_ADDR_16D33;
    Disp_WriteBuffer[0] = cmd;
    Disp_WriteBuffer[1] = param;
    DisplayCfg.tx_data   = Disp_WriteBuffer;
    DisplayCfg.tx_length = 2;
    HAL_I2C_MasterTransmitData(I2C_DISP->pI2Cx, &DisplayCfg, I2C_TRANSFER_POLLING);
}

// RAM 데이터 전체 업데이트
void HT16D33_WriteRamAll(BYTE *pRam)
{
  /* BYTE i;

    // 1. RAM Page Address Set (0번 페이지 지정)
    HT16D33_WriteCmdParam(HT16D33_CMD_RAM_PAGE_ADDR, 0x00);

    // 2. Write Display Data 명령어(0x80) + 32바이트 RAM 데이터[cite: 1]
    DisplayCfg.sl_addr7bit = SLAVE_ADDR_16D33;
	
    Disp_WriteBuffer[0] = HT16D33_CMD_WRITE_RAM;   // RAM Write command 
	Disp_WriteBuffer[1] = 0x00;     
	
    for (i = 0; i < HT16D33_RAM_SIZE; i++) {
        Disp_WriteBuffer[2+ i] = pRam[i];
    }

    DisplayCfg.tx_data   = Disp_WriteBuffer;
    DisplayCfg.tx_length = HT16D33_RAM_SIZE + 2;   // Command 1 byte + Data 32 bytes 

    HAL_I2C_MasterTransmitData(I2C_DISP->pI2Cx, &DisplayCfg, I2C_TRANSFER_POLLING);*/
	
	
    BYTE addr = 0, i, n;
    HT16D33_WriteCmdParam(HT16D33_CMD_RAM_PAGE_ADDR, 0x00);   // page 0 (1회)

    while (addr < HT16D33_RAM_SIZE) {           // 188바이트를 32씩 전송
        n = (BYTE)(HT16D33_RAM_SIZE - addr);
        if (n > 16) n = 16;

        DisplayCfg.sl_addr7bit = SLAVE_ADDR_16D33;
        Disp_WriteBuffer[0] = HT16D33_CMD_WRITE_RAM;   // 0x80
        Disp_WriteBuffer[1] = addr;                    // 이 조각의 시작 주소
        for (i = 0; i < n; i++) Disp_WriteBuffer[2 + i] = pRam[addr + i];

        DisplayCfg.tx_data   = Disp_WriteBuffer;
        DisplayCfg.tx_length = n + 2;
        HAL_I2C_MasterTransmitData(I2C_DISP->pI2Cx, &DisplayCfg, I2C_TRANSFER_POLLING);
        addr += n;
    }

	/*  WORD i;

    HT16D33_WriteCmdParam(HT16D33_CMD_RAM_PAGE_ADDR, 0x00);

    DisplayCfg.sl_addr7bit = SLAVE_ADDR_16D33;
    Disp_WriteBuffer[0] = HT16D33_CMD_WRITE_RAM;   
    Disp_WriteBuffer[1] = 0x00;                     

    for (i = 0; i < HT16D33_RAM_SIZE; i++)             
        Disp_WriteBuffer[2 + i] = pRam[i];

    DisplayCfg.tx_data   = Disp_WriteBuffer;
    DisplayCfg.tx_length = HT16D33_RAM_SIZE + 2;      
    HAL_I2C_MasterTransmitData(I2C_DISP->pI2Cx, &DisplayCfg, I2C_TRANSFER_POLLING);*/
}

void HT16D33_WriteLedCtrlAll(BYTE val)   // 0xFF = 전 도트 마스크 해제
{
   /* BYTE i;
    HT16D33_WriteCmdParam(HT16D33_CMD_RAM_PAGE_ADDR, 0x00);

    DisplayCfg.sl_addr7bit = SLAVE_ADDR_16D33;
    Disp_WriteBuffer[0] = 0x84;   // Write LED Control Data
    Disp_WriteBuffer[1] = 0x00;   // 시작 주소
    for (i = 0; i < 18; i++) Disp_WriteBuffer[2 + i] = val;   // 12x12 packed = 18바이트

    DisplayCfg.tx_data   = Disp_WriteBuffer;
    DisplayCfg.tx_length = 18 + 2;
    HAL_I2C_MasterTransmitData(I2C_DISP->pI2Cx, &DisplayCfg, I2C_TRANSFER_POLLING);*/
	
	 BYTE i;

    HT16D33_WriteCmdParam(HT16D33_CMD_RAM_PAGE_ADDR, 0x00);

    DisplayCfg.sl_addr7bit = SLAVE_ADDR_16D33;
    Disp_WriteBuffer[0] = 0x84;   /* Write LED Control Data */
    Disp_WriteBuffer[1] = 0x00;   /* 시작 주소 */

    /* LED Control RAM = 커먼당 2바이트(16bit).
       C1~C12 는 00h~17h(24바이트). 여유 두고 32바이트 전체를 채움 */
    for (i = 0; i < 18; i++) Disp_WriteBuffer[2 + i] = val;
	
	if (set_uv_cork_onoff == 0) {                   /* CA12,13,14 의 row0~4 만 off */
        Disp_WriteBuffer[2 + 0x0D] &= 0x0F;         /* com9  row0~3  */
        Disp_WriteBuffer[2 + 0x0E] &= 0xFE;         /* com9  row4    */
        Disp_WriteBuffer[2 + 0x0F] &= 0xE0;         /* com10 row0~4  */
        Disp_WriteBuffer[2 + 0x10] &= 0x0F;         /* com11 row0~3  */
        Disp_WriteBuffer[2 + 0x11] &= 0xFE;         /* com11 row4    */
    }
	

    DisplayCfg.tx_data   = Disp_WriteBuffer;
    DisplayCfg.tx_length = 18 + 2;                 /* 18+2 → 32+2 */
    HAL_I2C_MasterTransmitData(I2C_DISP->pI2Cx, &DisplayCfg, I2C_TRANSFER_POLLING);
}
/* 테스트 전용 ? C11-1 하나만 허용 */
void HT16D33_WriteLedCtrlOne(void)
{
    BYTE i;

    HT16D33_WriteCmdParam(HT16D33_CMD_RAM_PAGE_ADDR, 0x00);

    DisplayCfg.sl_addr7bit = SLAVE_ADDR_16D33;
    Disp_WriteBuffer[0] = 0x84;
    Disp_WriteBuffer[1] = 0x00;

    for (i = 0; i < 18; i++) Disp_WriteBuffer[2 + i] = 0x00;
    Disp_WriteBuffer[2 + 0x0F] = 0x01;     /* C11-1 */

    DisplayCfg.tx_data   = Disp_WriteBuffer;
    DisplayCfg.tx_length = 18 + 2;         /* ★ */
    HAL_I2C_MasterTransmitData(I2C_DISP->pI2Cx, &DisplayCfg, I2C_TRANSFER_POLLING);
}
/*
void HT16K33_WriteCmd(BYTE cmd)
{
    DisplayCfg.sl_addr7bit = SLAVE_ADDR_16K33;

    Disp_WriteBuffer[0] = cmd;
    DisplayCfg.tx_data   = Disp_WriteBuffer;
    DisplayCfg.tx_length = 1;

    HAL_I2C_MasterTransmitData(I2C_DISP->pI2Cx, &DisplayCfg, I2C_TRANSFER_POLLING);
}

void HT16K33_WriteRamAll(BYTE *pRam)
{
    BYTE i;

    DisplayCfg.sl_addr7bit = SLAVE_ADDR_16K33;

    Disp_WriteBuffer[0] = 0x00;// RAM start address 

    for (i = 0; i < HT16K33_RAM_SIZE; i++) {
        Disp_WriteBuffer[1 + i] = pRam[i];
    }

    DisplayCfg.tx_data   = Disp_WriteBuffer;
    DisplayCfg.tx_length = HT16K33_RAM_SIZE + 1;   //17 bytes

    HAL_I2C_MasterTransmitData(I2C_DISP->pI2Cx, &DisplayCfg, I2C_TRANSFER_POLLING);
}

*/

static void Buffer_Init(uint8_t* buffer, uint8_t type, uint16_t len) {
	uint8_t i;

	if (type) {
		for (i = 0; i < len; i++) {
			buffer[i] = i;
		}
	}
	else {
		for (i = 0; i < len; i++) {
			buffer[i] = 0;
		}
	}
}
//260223 touch input
void  GPIOC_IRQHandler_Input(void) {

	uint32_t Status;
    Status = HAL_GPIO_EXTI_GetState(PC);
    
	// fall Edge
	if ((Status & (3 << (7*2))) ){//== (3 << (7*2))) {

		bTouchFg=1;
		HAL_GPIO_EXTI_ClearPin((PORT_Type *)PC, Status);

	}

}

void GPIOE_IRQHandler_Input(void) {
	
	uint32_t Status;
    Status = HAL_GPIO_EXTI_GetState(PE);
    
	// Both Edge
	if ((Status & (3 << (7*2))) ){//pe7 FLOW HOT IN
		flow_hot_pulse_cnt++;
	
		HAL_GPIO_EXTI_ClearPin((PORT_Type *)PE, (Status & (3 << (7*2))) );
			
		//if(fg)	HAL_GPIO_SetPin( (PORT_Type *)PF,_BIT(4));
		//else HAL_GPIO_ClearPin((PORT_Type *)PF, _BIT(4));
		//fg=~fg;
	}
	if ((Status & (3 << (6*2))) ){//pe6 FLOW  OUT
		flow_out_pulse_cnt++;
	
		HAL_GPIO_EXTI_ClearPin((PORT_Type *)PE, (Status & (3 << (6*2))) );
	}
	if ((Status & (3 << (5*2))) ){//pe5 FLOW FILTER
		flow_pulse_cnt++;
	
		HAL_GPIO_EXTI_ClearPin((PORT_Type *)PE, (Status & (3 << (5*2))) );
	}		
	
	
}

void mainloop(void)
{
#if FILTER_WRITER
	backlight_en_fg = 1;        // 치구 모드 : 표시 항상 ON
#endif
	
	while(1)
	{

		if(m_n500ms_ok)  
		{
			m_n500ms_ok=0;
			
			//fctest();

			f1_life_check_fg = 1;
			f2_life_check_fg = 1;

			//Flow_in_check();//유량 체크 
			
			
			Cool_Control();// cool
			
			Uvc_Control();
			
			fcError();
		}

		if(ad_ok_fg)  //100ms
    	{	
			ad_ok_fg = 0;
			
			Flow_in_check();//유량 체크 
			
			Heater_Control();
			Flow_Hot_Pid(); //heater
			
			//Cool_Control();// cool
			Start_Delay_Check();    // 출수 시작 지연 (1초 뒤에 Key_action)
			Output_control(); 

			if(eep_data_fg) {
				Set_Eep_Exe();
			}
			Ad_conversion();	//ad변환
			
			
	Filter_state_check();	
      									
			//세정,알칼리수,산성수,PH단계 설정
			if(((s_mode&0xff0) == CLEAN_OUT) || ((s_mode&0x7f0) == ALKA)   || ((s_mode&0x7f0) == COOLALKALI)  || (s_mode == ION_OUT1)|| (s_mode == ION_OUT2)) //210812 
			{
				if((m_state&0x80)||(m_state&0x02))	//단수(후세정), 물유입
				{			  		
					if(after_clean_fg)	// 080418_1 후세정 후 전류인가 금지하게 하기 위해
					{
						Current_pid();   	// PID 제어 하기

						// 온도에러 발생하면 10분동안 에러 계속 발생
						if(temp_protect_fg)	
						{
							if(++temp_error_out_cnt >= 50)	//시험: 5초 후에 온도 에러 발생
							{
								if(!temp_error)
								{
									voice_out_cnt = 50;		//온도에라가 발생하면 경고음 출력
									temp_error = 1;
									lError |= ERR_SMPS_TEMP;
									temp_error_out_cnt = 0;
									t_err_clear_cnt = 0;	//210730
								
								}
							}
						}
					}
				}
				else 
				{
					//210730 온도에러 발생하면 10분후에 에러 해지
					if(temp_protect_fg)//E5 에러 발생시
					{
						temp_error_out_cnt = 0;
						if(++temp_protect_cnt>6000) //100ms*6000 ==600sec1=10min
						{
							temp_protect_cnt = 0;
							temp_protect_fg = 0;
						}
					}
				}
			}	
			
			//캘리브레이션
			if(s_mode==CAL_OUT) 
			{
				if(ion_i_cal_cnt)
				{
					Cal_ion_i();
				}
			}
			else {
				ion_i_cal_cnt = 0;
			}		
			//	1L가 됬을때 자동정지
		//	if(m_state==0x02) 	Key_action();  //&&((filter1_life<=F_MIN_LIFE)||(filter2_life<=F_MIN_LIFE)))	Key_action();
				
    	}		
    	// ADC 처리 끝
		
		if(byTimer10msFg==1)	//10ms
		{
			byTimer10msFg = 0;
						

			fcDisplay();
			
#if FILTER_WRITER
			lError     = 0;                           // 치구 모드 : 에러로 키가 막히지 않게
			serial_err = 0;
			
			if(bFwRbClrCnt)		bFwRbClrCnt--;        // 기록 초기화 표시 타이머
			
			// UID 확인 화면은 100ms 마다 다시 읽는다 (칩 바꿔 끼우며 비교)
			if(bFwDispMode)
			{
				if(++bFwUidRdCnt >= 10)	{ bFwUidRdCnt = 0;	Fw_Uid_Read(); }
			}
			else	bFwUidRdCnt = 0;
			
			if(bFwState == 1)	Filter_Write_Exe();   // 표시 갱신 후 실행
#endif
			
			Door_Check();

			
			 if(!sound_out_fg && !eeprom_save_fg && !( lError& ERR_LEAK))
			{
	 			key_input();
			}
			
		
			if(++auto_inc_cnt>=15)    //150ms 
			{
				auto_inc_cnt=0;


				
			  	//필터 교체 시기 출력 시점 
			  	if(filter_change_fg && flow_in_fg && !filter_change_en && !voice_out_cnt &&  !bFilterChgVFg)
			  	{
			  		/*if((s_mode&0x7f0)==PURE) {
				  		if(++filter_change_cnt>40)	//10.5sec
				  		{
				  			filter_change_cnt=0;
				  			filter_change_en=1;
				  		}
			  		}*/
			  		//else 
					{
				  		if(++filter_change_cnt>20)	//4.5sec
				  		{
				  			filter_change_cnt=0;
				  			filter_change_en=1;
				  		}
			  		}
			  	}
				
				Warning_voice();
     		}
     		
			//eeprom에 저장하고 있을때 보이스 출력 금지
			if(eeprom_save_fg)
			{
				voice_delay_cnt=0;
			}

			//Voice_control
			if(voice_real_fg)
			{
				if(++voice_delay_cnt>=2) // 260223 1->2
				{
					voice_delay_cnt = 0;
					voice_real_fg = 0;
		     
					Voice_real_output();
				}
			}

 			// 자동 세정
			Auto_clean();
	
 			// 물 유입이 없으면 자동세정도 지연 처리
			if(flow_liter < LOW_LIMIT_LITER)
			{
				if(clean_delay_cnt > 300)   //3sec
				{
					clean_delay_cnt--;
				}
			}
 			// 이전 알칼리 동작상태로 복귀
  	  	/*  	if(((s_mode&0xff0) != 0x30) && ((s_mode&0xff0) != 0x10) && !(m_state&0x02) && !(m_state&0x80))
  	  	  	{
  	  	  	  	if(++return_mode_cnt>=2500)  		// 25초후 모드설정에서 빠져나옴
  	  	  	  	{
  	  	  	  	 	return_mode_cnt=0;
					// 080218 이전모드가 알칼리4단계이면 알칼리 2단계로 이동
    		  	  	if(prev_alkali_mode != ALKA3)
    		  	  	{
    		  	  		s_mode = prev_alkali_mode;    // 알카리수 상태로 이동
    		  	  	}
    		  	  	else
    		  	  	{
    		  	  		s_mode = ALKA2;    
    		  	  	}
  	  	  	  	}
  	  	  	}	*/
  	  	  	// 080214 백라이트 자동 끄기 선택
  	  	  	// 백라이트 자동 감광/소등 : 무입력 1분 -> 30초 100% -> 10초 20% -> OFF
  	  	  	// 백라이트 자동 감광/소등 : 무입력 1분 -> 30초 100% -> 10초 20% -> 출수버튼만 20% 유지
  	  	  	if( (s_mode& 0x7f0) != MODE_SET  && !(m_state & 0x02)    )
  	  	  	{
	  	  	  	BYTE deact = (con_deact_show == DEACT_SHOW_ON) ? con_deactivate : 0;

	  	  	  	if(backlight_off_cnt < BL_T_DIM_END)  backlight_off_cnt++;

	  	  	  	if(backlight_off_cnt >= BL_T_DIM_END)         // 100초~ : 추출 표시등만 감광 유지
	  	  	  	{
	  	  	  	  	if(bl_phase != 3)
	  	  	  	  	{
	  	  	  	  	  	bl_phase = 3;
	  	  	  	  	  	Update_Color(con_deactivate, deact);
	  	  	  	  	  	backlight_en_fg = 0;     // Display_Ready() 만 그림 = 출수버튼
	  	  	  	  	}
	  	  	  	}
	  	  	  	else if(backlight_off_cnt >= BL_T_FULL_END)   // 90~100초 : 전체 감광
	  	  	  	{
	  	  	  	  	if(bl_phase != 2)
	  	  	  	  	{
	  	  	  	  	  	bl_phase = 2;
	  	  	  	  	  	Update_Color(con_deactivate, deact);
	  	  	  	  	}
	  	  	  	}
	  	  	  	else                                          // 0~90초 : 정상
	  	  	  	{
	  	  	  	  	if(bl_phase != 1)
	  	  	  	  	{
	  	  	  	  	  	bl_phase = 1;
	  	  	  	  	  	Update_Color(con_brightness, 0);
	  	  	  	  	}
	  	  	  	}
	  	  	}
			
  	  	  	
	  	  	// 동작중일때는 백라이트 자동끄기 기능 없음.
	  	  	if((s_mode&0x80)||(m_state&0x02))
	  	  	{
	  	  		backlight_off_cnt = 0;
	  	  	}
			else
			{
				//210730b 정지 또는 후세정모드에서 smps 보호모드 종료
				if(smps_protect_fg) 	//smps 과열 보호 모드 이고
				{
					smps_protect_cnt=0;	//전류제한 시간 카운터 초기화
					smps_protect_fg = 0;
				}  	  	
			}
	  	  	
			// 켈리브레이션 모드나 필터에 문제(읽기)가 있으면 키 입력이 안됨 180719 시리얼 에러시 키 입력 안되게 수정
  	  	  	//if((!(filter_error_fg&0xcf)) && (!ion_i_cal_cnt) && (!ion_v_cal_cnt))
  	  	  	if((!ion_i_cal_cnt) && (!ion_v_cal_cnt)&& !serial_err)
  	  	  	{
				Key_exe();
				
  	  	  	}
			
			
			 //  손을 완전히 뗀 뒤에 터치 IC 재초기화
            //  (터치 중에 캘리브레이션하면 베이스라인이 오염됨) 
            if(bTouchSensApplyFg)
            {
                if(sw_data || bTouchFg)
                {
                    wTouchSensApplyCnt = 0;
                }
                else if(++wTouchSensApplyCnt >= 100)   /* 10ms × 100 = 1초 무터치 */
                {
                    wTouchSensApplyCnt = 0;
                    bTouchSensApplyFg  = 0;
                    TouchReg_Init();                   /* 약 500ms 블로킹 */
                }
            }
			
			
		} //if(byTimer10msFg==1)
		
		Heater_Current();
	}
}


/**********************************************************************
 * @brief		Main program
 * @param[in]	None
 * @return	None
 **********************************************************************/
int main (void)
{

	 /* Initialize all port */
	Port_Init();  

	/* Configure the system clock to HSE 8 MHz */
	SystemClock_Config();
	
	//LCD_Init();
	
	TIMER14_Pwm();	
	TIMER13_Pwm();	
	
	//TIMER30_Pwm();
	TIMER2n_Init();
	
	
	ADC_Init();
	
	I2C_ChannelConfig(Master, I2C_SPEED, MASTER);       //key
	I2C0_ChannelConfig(I2C_DISP, I2C_SPEED, MASTER);  //disp
	
	//systick
  SysTick_Config(SystemCoreClock/1000);  //   1msec interrupt  (Use Core Clock)
	
	delay_1ms(250);	//231005-1  전원이 불안정하게 인가될때 설정데이터 리드와 필터  리드에 오류가 발생함을 방지,안정적 부팅 후 설정값리드
	
	//저장할 데이터 설정
	Set_eep_init();
	Set_value_load();
	
	
	
	lTargetFlow =bDisp_mL[ bSet_mL] * 120;
	
	//260223
	GD25D10_Init();
	//Aes128Init();
	
	F2_life_reload();	delay_1ms(50);	
	F1_life_reload();	delay_1ms(50);	
	

    TouchReg_Init();	
	
	delay_1ms(250);	
	//160219_1 초기 음량 커지는 문제
	Voice_delay_output(sound_volum_tbl[volume_level]);   
	delay_1ms(50);	//old 35

	//PORTe 5,6,7
	NVIC_EnableIRQ(GPIOE_IRQn);
    NVIC_SetPriority(GPIOE_IRQn, 3);

    HAL_GPIO_EXTI_ClearPin((PORT_Type *)PE, HAL_GPIO_EXTI_GetState((PORT_Type *)PE));
    HAL_GPIO_EXTI_Config((PORT_Type *)PE, 7 ,PCU_INTERRUPT_MODE_EDGE, PCU_INTERRUPT_CTRL_EDGE_BOTH);  // PCU_INTERRUPT_CTRL_EDGE_BOTH
	HAL_GPIO_EXTI_Config((PORT_Type *)PE, 6 ,PCU_INTERRUPT_MODE_EDGE, PCU_INTERRUPT_CTRL_EDGE_BOTH);  
	HAL_GPIO_EXTI_Config((PORT_Type *)PE, 5 ,PCU_INTERRUPT_MODE_EDGE, PCU_INTERRUPT_CTRL_EDGE_BOTH);  
	
	//PORTC INT 7
	NVIC_EnableIRQ(GPIOCD_IRQn);
    NVIC_SetPriority(GPIOCD_IRQn, 3);
	
	HAL_GPIO_EXTI_ClearPin((PORT_Type *)PC, HAL_GPIO_EXTI_GetState((PORT_Type *)PC));
    HAL_GPIO_EXTI_Config((PORT_Type *)PC, 7 ,PCU_INTERRUPT_MODE_EDGE, PCU_INTERRUPT_CTRL_EDGE_FALLING) ; //); 
	
	HT16D33_Init();
	delay_1ms(1000);	
	
	
	backlight_en_fg  = 1;        //전원 인가 후 표시 ON 
	backlight_off_cnt = 0;
	bl_phase          = 0;       //다음 틱에서 정상 밝기로 진입 



	__enable_irq();
	
/*
filter1_life = 3000000;
filter2_life = 3000000;
f1_life_save_fg=1;
f2_life_save_fg=1;

while(1)
{	
	if(m_n500ms_ok)  
	{
		m_n500ms_ok=0;
		
		Filter_life_save();
		
		if(f1_life_save_fg==0 && f2_life_save_fg==0)  break;
	}
}

*/
mainloop();

	return (0);
}




/**************************************************************************************************
		자동 세정
**************************************************************************************************/
void Auto_clean(void)
{
	ULONG pwm_temp=0;
	
	if((auto_clean_fg) && (s_mode!=CLEAN_OUT))
	{
		s_mode = CLEAN;
		if(++m_n5sec_cnt >= 300)		// 3초
		{
			m_n5sec_cnt = 0;
			
			//s_mode = CLEAN;

			pwm_temp = alkali_ph_step[2];	// 3단계				
			pwm_value = pwm_init_tbl[pwm_temp-1]; 
			if(pwm_value>3280)  pwm_value = 3280;	//경제형 PWM 최대값 제한  	  		    			
			Pwm_out(pwm_value);
     	
			Key_action();
		}
	}
	else
	{
		m_n5sec_cnt = 0;
	}

	if(filter_error_fg&0xfc)  // ??? 
	{
		clean_delay_cnt=2000;
	  	cleaning_fg=0;
	}

	if(auto_clean_fg && (s_mode==CLEAN_OUT))
	{

		if(++clean_delay_cnt>=2200)   //  세정 경고음 끊김 현상 수정 20초 -> 22초
		{
			clean_delay_cnt = 0;
			auto_clean_fg = 0; 
			bManualCleanFg = 0;
			auto_clean_cnt = 0;
			first_clean_fg = 0;			// 초기세정완료 

			voice_add = SND_CONFIRM ;   
			m_state &= 0xfd;     // 동작정지
			m_state |= 0x80;     // 단수 동작 시작 

			s_mode_old = s_mode&0x7ff;
			
			s_mode = s_mode_return; // 세정 완료후 이전모드로 
		}
	}
	else
	{
	  	clean_delay_cnt=0;
	}
}
/**************************************************************************************************
		PID 제어
**************************************************************************************************/
void Current_pid(void)
{

	if(m_state&0x80)	// 단수시 후세정 
	{
		switch(s_mode&0x7f0)	// 후세정 전류값 고정
	  {
	  	case CLEAN : 	//  세정	
			set_pi_ref = 500;	   					
			break; 
	  	case ALKA : 	//  알카리
		case COOLALKALI : 	
	  	case MODE_SET : 	//  설정중의 알카리
			set_pi_ref = 500;
  			break;
	  	default :
  	  	break;  
	  }
	}
	else	// 출수시 
	{
	 	switch(s_mode&0x7f0)
	 	{
	 	 	case CLEAN : 	//  세정 
    			set_pi_ref = 500;	// 후세정 전류값 고정
    			
    			break;
	 	 	case ALKA : 	//  알카리
			case COOLALKALI : 	
				set_pi_ref = (ULONG)pi_ref_tbl[alkali_ph_step[(s_mode&0x0f)-1]-1];  // ??? -1
	    		if(alka_max_fg && ((s_mode&0x0f)==0x03)) {	
	    			set_pi_ref = 5000;	//5000;	//전해조 9장 7A 제한
	    		}
				
	    		break;
	 	 	case MODE_SET : //  설정중의 알카리
    			set_pi_ref = (ULONG)pi_ref_tbl[alkali_ph_step[ion_state-1]-1];
    			break;  
	 	 	default :
     			break;  
	 	}
	}

	// smps 보호 모드 변경
 	if(ion_i >= 5000) 	//5A
	{
		if(!smps_protect_fg) //smps 온도 보호 상태 아니고
		{
			if(++smps_protect_cnt>1800) // 출수 연속 3분 이상하면 smps 보호 설정 5분(300sec)->3분(180sec)
			{
				smps_protect_cnt=1800;			
				smps_protect_fg = 1;
				alka_max_fg = 0;
			}
		}
	}
	

	if(smps_protect_fg && ((s_mode&0x7f0)!=MODE_SET)) {
		if(set_pi_ref>3000) {
			set_pi_ref = 3000;
		}
	}	  	

	// 유량에 관계없이 일정전류제어  
	error_0 = set_pi_ref - ion_i; 	//현재 error = ref - 계측값   	  
	
	if((s_mode &0x7f0) == COOLALKALI)	GAIN_P=90;
	else GAIN_P = 70;
	  	
	pi_value = pi_value + (error_0*GAIN_P/1000L) + (error_0_old*GAIN_I/10000L);
	error_0_old = error_0;          //이전 error
	
	//pwm 최대 최소값 제한
	if((m_state==0x80) && !ion_i)		
	{
		pi_value = 0;
	}
	else
	{
  	if(pi_value<1) pi_value = 1;
  	
  	if(ion_i > 5300)		//전류값 크기에 따라 PWM값 제한
  	{
  		if(pi_value>5000)  pi_value = 5000;
  		pi_value -= 150;		// 경제형 PWM 최대값 제한
  	}
  	else
  	{
  		if(pi_value>5000)  pi_value = 5000;
  	}
	}
	  	
	pwm_value = (WORD)pi_value;
	
	//단계별 pwm 값 제한
	if(((s_mode&0x0f) == 0x01) && (pi_value<0)  ){
		pwm_value = 45; //200mA 정도 
	}
	//else if(((s_mode&0x0f) == 0x01) && (pwm_value>1500)) 	pwm_value = 1500;
	//else if(((s_mode&0x0f) == 0x02) && (pwm_value>2000)) 	pwm_value = 2000;

	if(first_clean_fg) {	//190701 초기세정후 전류값 제한
		pi_value = 0;
		error_0_old = 0;
		pwm_value = 0;	
	}
	
	if(lError & ERR_FLOW){  // ???
	
		pi_value = 0;
		error_0_old = 0;

		Pwm_off();//PWM_DATA  = 0;   
		RELAY_OFF;
	}
	else{ // 정상 
		Pwm_out(pwm_value);
  	}
}
/**************************************************************************************************
		전류 켈리브레이션
**************************************************************************************************/
void Cal_ion_i(void)
{  	
	//BYTE eep_temp[2];	//게인기준값 :1482
/*
  	if(ion_i<1005L)     // 1 A 기준
  	{
  	  	if(I_GAIN_M < 4000)	I_GAIN_M++;	//1800  260223  2000  ??? 
  	}
  	else if(ion_i>1005L)
  	{	
  	 	if(I_GAIN_M > 300)	I_GAIN_M--;	// 1000  260223  1000   ???
  	}
  	*/
	
	I_GAIN_M = avg_current;
	

  //	if((ion_i>=1000L) && (ion_i<=1009L))
	if((I_GAIN_M>=620) && (I_GAIN_M<=700))
  	{
  	  	if(ion_i_cal_cnt) ion_i_cal_cnt--;	
  	}
  	
  	if(ion_i_cal_cnt==0)	//sss
  	{
  		//eep_temp[0] = I_GAIN_M;
  		//eep_temp[1] = I_GAIN_M>>8;
  		
  	  	Voice_output(SND_CONFIRM);
		
		eep_data_fg=1; //260223 save flag
  	}
}

void Flow_Hot_Pid(void)
{
	uint16_t temper = 0;
	long base_pwm ;
	long control_output ;
	BYTE i;

	if( (s_mode == HOT_OUT )  && !lError ){
		
		if( s_mode == HOT_OUT)		temper = bSetHotTemper;
		else temper = 0; //45도  FLUSHING_OUT
		
		target_flow = HOT_FLOW[temper] ; //온도에 따른 유량 제어
		
		// 피드포워드 : 목표 유량 -> 기준 PWM
		base_pwm = HOT_PWM_INIT[temper] +( (target_flow - FLOW_OFFSET) * FLOW_SLOPE/10000);
		
		// 이동창 갱신 : 100ms 마다 가장 오래된 샘플을 빼고 새 샘플을 더함
		// 100ms 창은 430ml/min 에서 펄스가 1.9개뿐이라 분해능이 231ml/min -> 제어 불가
		// 500ms 이동창이면 9.3개 -> 분해능 46ml/min , 갱신은 100ms 유지
		lHotPulseSum -= lHotPulseBuf[bHotPulseIdx];
		lHotPulseBuf[bHotPulseIdx] = saved_flow_hot_pulse;
		lHotPulseSum += saved_flow_hot_pulse;
		if(++bHotPulseIdx >= HOT_FLOW_WIN)	bHotPulseIdx = 0;
		
		flow_hot_meas = (long)lHotPulseSum * 20000L / FLOW_GAIN;   // ml/min
		
		if (bFirstRun) {
			// 기동 1.2초 : 피드포워드만 출력, 그 동안 이동창을 실제 펄스로 채움
			lFlowHot_error = 0;
			lFlowHot_integral_error = 0;
			
			if(++bFirstCnt>= 12 ){ // 0.7초후 on 되서  7+5=12
				bFirstCnt = 0;
				bFirstRun = 0;
			}
		}
		else {
			lFlowHot_error = target_flow - flow_hot_meas;
			
			// anti-windup : 출력이 포화되지 않은 구간에서만 적분 누적
			control_output = base_pwm + (Kp * lFlowHot_error/10)
			                          + (Ki * lFlowHot_integral_error/10);
			
			if( (control_output < 1650) && (control_output > 500) )
			{
				lFlowHot_integral_error += lFlowHot_error;
				
				if (lFlowHot_integral_error > HOT_PID_I_LIMIT)       lFlowHot_integral_error = HOT_PID_I_LIMIT;
				else if (lFlowHot_integral_error < -HOT_PID_I_LIMIT) lFlowHot_integral_error = -HOT_PID_I_LIMIT;
			}
		}
		
		control_output = base_pwm + (Kp * lFlowHot_error/10)
		                          + (Ki * lFlowHot_integral_error/10);
		
		if (control_output > 1650) control_output = 1650;
		if (control_output < 500)  control_output = 500;
		
		lPump_rpm =  (int16_t)control_output;
		PwmHotPump_out(lPump_rpm);
	}
	else{
		bFirstRun = 1;
		bFirstCnt = 0;
		lFlowHot_error = 0;
		lFlowHot_integral_error = 0;
		
		for(i=0;i<HOT_FLOW_WIN;i++)	lHotPulseBuf[i] = 0;
		lHotPulseSum = 0;
		bHotPulseIdx = 0;
		
		bWaitCnt = 0;
		bWaitFg=0;
		
		// 플러싱의 펌프 구간은 Output_control() 이 고정 PWM 을 준다.
		// 여기서 끄면 100ms 마다 켜고 끄기를 반복해 펌프가 제대로 돌지 않는다.
		if( !( ((s_mode&0x7ff) == FLUSHING)
		       && ((bFlushingStep == 2) || (bFlushingStep == 4) || (bFlushingStep == 5)) ) )
		{
			PwmHotPump_off();
		}
	}
}

/*
void Flow_Hot_Pid(void)
{
	uint16_t temper = 0;
	long base_pwm ;
	long control_output ;

	if( (s_mode == HOT_OUT )  && !lError ){ //  &&  !(lError & ERR_FLOW )){	|| (s_mode==  FLUSHING_OUT &&  bFlushingStep==2 )
		

			if( s_mode == HOT_OUT)		temper = bSetHotTemper;
			else temper = 0; //45도  FLUSHING_OUT
			
			
			//  (목표 유량 - 현재 유량)
			target_flow = HOT_FLOW[temper] ; //온도에 따른 유량 제어

		   // 
			if (bFirstRun) {
				flow_in_hot_new = HOT_FLOW[temper] ;// 초기값이 있어야 함

				lFlowHot_integral_error = 0; //  초기화
				
				if(++bFirstCnt>= 12 ){ // 0.7초후 on 되서  7+5=12
					bFirstCnt = 0;
					bFirstRun = 0;
				}
			}		

			// 유량 430->700, 652->1000 
			base_pwm = HOT_PWM_INIT[temper] +( (target_flow - FLOW_OFFSET) * FLOW_SLOPE/10000);

			// 4. 오차 계산 (현재 유량 데이터는 500ms마다 갱신되지만, 제어는 100ms마다 수행)
			lFlowHot_error = target_flow - (flow_in_hot_new *1);


			lFlowHot_integral_error += lFlowHot_error;


			if (lFlowHot_integral_error > 500) lFlowHot_integral_error = 500;
			else if (lFlowHot_integral_error < -500) lFlowHot_integral_error = -500;
			
			Kp=2;
			Ki=1;
			control_output =  base_pwm + (Kp * lFlowHot_error/10) + (Ki * lFlowHot_integral_error/10);

			if (control_output > 1650) control_output = 1650;
			if (control_output < 500)  control_output = 500;
			
			lPump_rpm =  (int16_t)control_output;
			PwmHotPump_out(lPump_rpm);


	}
	else{
		bFirstRun = 1;
		bFirstCnt = 0;
        lFlowHot_integral_error = 0;
		
			bWaitCnt = 0;
				bWaitFg=0;
		
		PwmHotPump_off();		
	}
}

*/
//100ms
void Heater_Control(void)
{
	if( s_mode == HOT_OUT  ){  //  !(lError & ERR_FLOW )){	//|| s_mode==FLUSHING_OUT

		/*if(ho_temp >= 990) //99도   ho_temp ??? 
		{
			bHeaterFg = 0;
		}
		else if(ho_temp <= 950) //80도
		{
			bHeaterFg = 1;
		}	
		
		if(bHeaterStartFg==0){
			bHeaterStartFg = 1;
			bHeaterFg = 1;
		}*/
			
if(bSetHotTemper == 0)   // 45도 : 출수 온도 PI 로 히터 듀티 제어
		{
			if( ho_temp >= ((int32_t)con_hot_over_temp * 10) )
			{
				// 과열 : 듀티 무시하고 즉시 차단
				bHeaterFg = 0;
				lHotTempInteg = 0;
			}
			else
			{
				long ltmp;
				
				// 목표 온도 - 현재 출수 온도 (0.1도 단위)
				lHotTempErr = ((long)wSetHotTemperDisp[bSetHotTemper] + HOT_T_OFFSET) - ho_temp;
				
				// anti-windup : 듀티가 포화되지 않은 구간에서만 적분
				if( (bHeaterDuty > 0) && (bHeaterDuty < 100) )
				{
					lHotTempInteg += lHotTempErr;
					if(lHotTempInteg >  HOT_T_I_LIMIT) lHotTempInteg =  HOT_T_I_LIMIT;
					if(lHotTempInteg < -HOT_T_I_LIMIT) lHotTempInteg = -HOT_T_I_LIMIT;
				}
				
				ltmp = (lHotTempErr * HOT_T_KP / 10) + (lHotTempInteg * HOT_T_KI / 1000);
				if(ltmp > 100) ltmp = 100;
				if(ltmp < 0)   ltmp = 0;
				bHeaterDuty = (BYTE)ltmp;
				
				// 저속 PWM (1초 주기)
				if(++bHeaterDutyCnt >= HOT_DUTY_PERIOD)   bHeaterDutyCnt = 0;
				
				if( bHeaterDutyCnt < ((WORD)bHeaterDuty * HOT_DUTY_PERIOD / 100) )	bHeaterFg = 1;
				else																bHeaterFg = 0;
			}
		}
		else                     // 75도 / 85도 : 출수 중 full power
		{
			bHeaterFg = 1;
			
			bHeaterDutyCnt = 0;      // 45도 복귀시 깨끗하게 시작
			bHeaterDuty    = 100;
			lHotTempErr    = 0;
			lHotTempInteg  = 0;
		}
		
		if(bHeaterStartFg==0){
			bHeaterStartFg = 1;
			bHeaterFg = 1;
		}
	}
	else if( bFlushHeaterFg && (s_mode == FLUSHING_OUT) && (m_state & 0x02) )
	{
		// 플러싱 온수관로 배수 : 히터 동작 테스트 60도
		if(ho_temp >= FLUSH_HOT_TEST_OFF)       bHeaterFg = 0;   // 60.0도 이상 -> OFF
		else if(ho_temp <= FLUSH_HOT_TEST_ON)   bHeaterFg = 1;   // 55.0도 이하 -> ON
		
		bHeaterStartFg = 1;
	}
	else{
	
		bHeaterFg = 0;
		bHeaterFlowCnt = 0;
		
		bHeaterStartFg=0;
		
		bHeaterDutyCnt = 0;
		bHeaterDuty    = 100;
		lHotTempErr    = 0;
		lHotTempInteg  = 0;
	}
	
	
	if(bHeaterFg){

			if(   (m_state&0x02)  && (lError & ERR_FLOW) ){// 출수중 유량에러이면 히터 off

				if(++bHeaterFlowCnt >= 40){ //100ms
					bHeaterFlowCnt = 0;
					HEATER_OFF;	
				}
				
			}
			else if(lError & ERR_HOT_OVER){
				bHeaterFlowCnt = 0;
				HEATER_OFF;	
			}
			else{
				HEATER_ON;
				bHeaterFlowCnt = 0;
			}
	
	}
	else{
		HEATER_OFF;
		bHeaterFlowCnt = 0;
	}
	
	if( ho_temp >= ((int32_t)con_hot_over_temp * 10) )
	{
		if(++bHeaterOverCnt >= (BYTE)(con_hot_over_sec * 10))
		{
			bHeaterOverCnt = (BYTE)(con_hot_over_sec * 10); //0 리셋 X → 에러 래치 유지 
			HEATER_OFF;
			lError |= ERR_HOT_OVER;
		}
	}
	else if( ho_temp <= (((int32_t)con_hot_over_temp * 10) - HOT_OVER_HYST) )
	{
		//히스테리시스 : 설정 105 → 80 이하에서 해제
		bHeaterOverCnt = 0;
		lError &= ~ERR_HOT_OVER;
	}
/*
	if((ho_temp >= 1050) ) //105도   
	{
		if(++bHeaterOverCnt>=50){ //5초
			bHeaterOverCnt = 0;
			HEATER_OFF;	
			lError |= ERR_HOT_OVER	;
		}
	
	}
	
	else{
		bHeaterOverCnt = 0;
		lError &= ~ERR_HOT_OVER	;
	}
	
	*/
	
	
	if(bHotOutEndFg){
		if(++wHotOutEndCnt >= 100) { //100ms*100=10000=10s
			wHotOutEndCnt = 0;
			bHotOutEndFg = 0;
		
    		s_mode = s_mode_return & 0x7ff;
		}
	}
}

void Cool_Control(void)
{
	if(bCoolCon_Start    ){
		
		if (bInRestPeriod == 1) {

			if (++lCoolRestTick >= TICKS_30_MINUTES) {
				//  재시작 
				bInRestPeriod = 0;
				lCoolRestTick = 0;
				lCoolRunTick = 0;
				bInitialCooling = 1; 
			} else {
				// 정지중
				bCoolTempStatus = 0;
				nPeltierPwm = 0;
				PwmCoolFan_off();
				PwmCool_off();
				
			}
		}
		
		//
		if (bInRestPeriod == 1) {
			bCoolFg = 0; // 휴식 중이면 무조건 정지   CLEAN_OUT
		}
		else if( (((s_mode&0x7f0) == COOLALKALI) ||     ((s_mode&0x7f0) == ALKA)||   ((s_mode&0x7f0) == CLEAN)||   ((s_mode&0x7f0) == FLUSHING)||    ((s_mode&0x7ff) == PH_SET) || ((s_mode&0x7ff) == PH_SET2)|| ((s_mode&0x7ff) == PH_SET3) ) &&( (m_state&0x02)||(m_state&0x80) ))
		{ // 전해조  동작시 열전소자 정지  	
			bCoolFg=0; //정지
			
			bCoolReFg=1;
			wCoolReCnt=0;
		}
		else{
			
			if(bCoolReFg){
				if(++wCoolReCnt>=20) { //10sec 전해조 동작으로 열전소자 정지후 10초 후 가동 500ms*20
					wCoolReCnt = 0;
					bCoolReFg=0;
					
					bCoolFg=1; //가동
				}
			}
			else{
				bCoolFg=1; //가동
			}
		}
		
	
		
//	bInitialCooling  = 0;// ???? 
		if(bCoolFg){

        
			// 5시간 연속 동작 시 휴식 모드 진입
		/*	if ( ++lCoolRunTick >= TICKS_5_HOURS) {
				bInRestPeriod = 1;
				lCoolRunTick = 0;
				
			}*/
			
			//열전소자 온도 에러 
			// 5시간 연속 동작 시 : 5.0도 이하로 못 내려가면 에러 판정
			if ( ++lCoolRunTick >= TICKS_5_HOURS) {
				lCoolRunTick = 0;

				if (wCoolTemper > 50) {          // 5시간 가동에도 5.0도 초과
					if (bCoolRetryFg)  lError |= ERR_PELTIER_TEMP;   // 2회째 실패 -> 에러
					bCoolRetryFg = 1;                                 // 30분 후 1회 재시도
				}
				bInRestPeriod = 1;               // 30분 휴식
			}

			// 5.0도 이하로 내려갔으면 재시도 카운트/에러 해제
			if (wCoolTemper <= 50) {
				bCoolRetryFg = 0;
				lError &= ~ERR_PELTIER_TEMP;
			}
			
		
			// cool
			// 냉각 동작/정지 래치 (사양 : 5.0도 이상 가동 / 2.0도 이하 정지)
			if      (wCoolTemper >= 50)  bCoolRunFg = 1;
			else if (wCoolTemper <  20)  bCoolRunFg = 0;

			if (bCoolRunFg == 0) {              // 2.0 ~ 5.0도 : 정지 유지
				bCoolTempStatus = 1;
				bInitialCooling = 0;
				nPeltierPwm   = 0;
				nActualPwm    = 0;
				nFanPwm       = 0;
				nActualFanPwm = 0;
				PwmCoolFan_off();
				PwmCool_off();
			}
			else if (wCoolTemper >= 50) { // ~5.0도
				bCoolTempStatus = 0;
				
				//열전소자
				nTargetAdc = TARGET_ADC_130; //목표 전류
				nBasePwm = 4300;
				nRampStep=150;	
				
				//팬
				nFanPwm = 5000; //24V 
			} 
			else if (wCoolTemper >= 45) { // ~4.5도
				bCoolTempStatus = 0;
				
			
				if (bInitialCooling) {
					//열전소자
					nTargetAdc = TARGET_ADC_130;
					nBasePwm = 4300;
					nRampStep=150;
					
					//팬
					nFanPwm = 5000; //24V 
				} else {
					//열전소자
					nTargetAdc = TARGET_ADC_071;
					nBasePwm = 2500;
					nRampStep=100;
					
					//팬
					nFanPwm = 500; //17 v
				}
			} 
			else if (wCoolTemper >= 35) { // ~3.5도
				bCoolTempStatus = 0;
				
				if (bInitialCooling) {
					//열전소자
					nTargetAdc = TARGET_ADC_130;
					nBasePwm = 4300;
					nRampStep=150;
					
					//팬
					nFanPwm = 5000; //24V 
				} else {
					//열전소자
					nTargetAdc = TARGET_ADC_056;
					nBasePwm = 1900;
					nRampStep=50;
					
					//팬
					nFanPwm = 200; //14 v
				}
			} 
			else if (wCoolTemper >= 20) { // ~2.0도
				bCoolTempStatus = 0;
				
				if (bInitialCooling) {
					//열전소자
					nTargetAdc = TARGET_ADC_130;
					nBasePwm = 4300;
					nRampStep=150;
					//팬
					nFanPwm = 5000; //24V 
				} else {
					//열전소자
					nTargetAdc = TARGET_ADC_040;
					nBasePwm = 1300;
					nRampStep=20;
					//팬
					nFanPwm = 200; //14 v
				}
			} 
			else { // 2.0도 미만 (정지)
				
				bCoolTempStatus = 1;
				
				
				bInitialCooling = 0;
				
				nPeltierPwm = 0;
				nActualPwm = 0;
				nFanPwm = 0;
				nActualFanPwm=0;
				PwmCoolFan_off();
				PwmCool_off();

			}

			if (bCoolTempStatus == 0) {
				
				//열전소자		
				if (nPeltierPwm <= 0) {
                    nPeltierPwm = nBasePwm;
                }

                //
                nError = nTargetAdc - avg_pelier_temp;
                nStep = nError * 2; 
                nPeltierPwm += nStep;

                // 
                if (nPeltierPwm > 5000) nPeltierPwm = 5000;
                if (nPeltierPwm < 0)      nPeltierPwm = 0;
 
                if (nActualPwm < nPeltierPwm) {
                    nActualPwm += nRampStep; // 서서히 증가
                    if (nActualPwm > nPeltierPwm) {
                        nActualPwm = nPeltierPwm; 
                    }
                } 
                else if (nActualPwm > nPeltierPwm) {
                    nActualPwm -= nRampStep; // 서서히 감소
                    if (nActualPwm < nPeltierPwm) {
                        nActualPwm = nPeltierPwm; 
                    }
                }
				
				//팬
				if (nFanPwm > 5000) nFanPwm = 5000;
                if (nFanPwm < 0)      nFanPwm = 0;
				
				if (nActualFanPwm < nFanPwm){ // 서서히 증가
					 nActualFanPwm += nRampStep; // 서서히 증가
                    if (nActualFanPwm > nFanPwm) {
                        nActualFanPwm = nFanPwm; 
                    }
                } 
                else if (nActualFanPwm > nFanPwm) {
                    nActualFanPwm -= nRampStep; // 서서히 감소
                    if (nActualFanPwm < nFanPwm) {
                        nActualFanPwm = nFanPwm; 
                    }
                }

                // 최종 출력
                PwmCoolFan_out( nActualFanPwm); 
                PwmCool_out(nActualPwm);
				
			}
			
		}
		else{
		
			bCoolTempStatus = 0;		
			bInitialCooling = 1;
			nPeltierPwm = 0;
			nActualPwm = 0;
			
			
			if(bCoolReFg==0)	{ //전해조 동작 안할때  정지
				nFanPwm = 0;
				nActualFanPwm=0;
				PwmCoolFan_off(); // FAN: 0V  전해조 동작시에는 정지하지 않음
				
			}
			
			
			PwmCool_off();      // 열전소자: 0V
		}

	}
	else{
		bCoolTempStatus = 0;
		bInitialCooling = 1;
		nPeltierPwm = 0;
		nActualPwm = 0;
		nFanPwm = 0;
		nActualFanPwm=0;
		
		lCoolRunTick = 0;
		lCoolRestTick = 0;
		bInRestPeriod = 0;
		
		PwmCoolFan_off(); //fan
		PwmCool_off();// 열전소자 off
		
		
	}
	
}


void Output_mL_Control(void)
{
	uint32_t flow;
	uint32_t currentCheckSum;
	
	/*if (bDispenseRestPeriod == 1) {
		lDispenseRestTick++;
		
		if (lDispenseRestTick >= TICKS_2_MINUTES) {
			bDispenseRestPeriod = 0;
			lDispenseRestTick = 0;
			lDispenseTick = 0;
		}
		
		// 휴식 중인데 어떤 이유로든 출수가 감지되면 강제 정지
		if (m_state & 0x02) {
			Key_action();
			Voice_output(SND_FAIL);
			return; 
		}
	}*/
	
	if(flow_in_fg && ((s_mode&0x7f0) != CLEAN) && ((s_mode&0x7f0) != MODE_SET) )
	{
		if(( s_mode&0x7ff) == FLUSHING  ){  // 플러싱---------------------------
			// 단계별 진행량 판정
			//  0 / 1 / 3 단계 : 원수 직통 -> 입수 유량센서(lFlowSum_In)
			//  2 / 4 / 5 단계 : 마이크로 펌프 -> 온수 유량센서(lFlowSum_Hot)
			//    펌프 구간은 원수가 바로 흐르지 않아 입수 센서로는 진행되지 않는다.
			//    온수 출수의 정량 판정도 lFlowSum_Hot 을 쓴다.
			if(bFlushingStep==0){
				flow = FLUSH_ML_FILTER;           // 3000ml 필터 세척
				lFlowSum = lFlowSum_In;           // 입수량

				if (lFlowSum >= flow) {
					lFlowSum = 0;
					lFlowSum_In = 0;
					lFlowSum_Hot = 0;
					
					wFlushingCnt=0;
					bFlushingStep=1;
				}
			}
			else if(bFlushingStep==1){            // 냉수 라인 물채우기
				flow = FLUSH_ML_COOL;             // 1000ml
				lFlowSum = lFlowSum_In;           // 입수량

				if (lFlowSum >= flow) {
					lFlowSum = 0;
					lFlowSum_In = 0;
					lFlowSum_Hot = 0;
					
					wFlushingCnt=0;
					bFlushingStep=2;
				}
			}
			else if(bFlushingStep==2){            // 온수 라인 물채우기 (펌프)
				flow = FLUSH_ML_HOT;              // 300ml
				#if FLUSH_PUMP_BY_HOT
				lFlowSum = lFlowSum_Hot;          // 온수 유량센서 (PE7)
#else
				lFlowSum = lFlowSum_In;           // 임시 : 입수 유량센서 (PE5)
#endif

				if (lFlowSum >= flow) {
					lFlowSum = 0;
					lFlowSum_In = 0;
					lFlowSum_Hot = 0;
					
					wFlushingCnt=0;
					bFlushingStep=3;
				}
			}
			else if(bFlushingStep==3){            // 정수 라인 물채우기
				flow = FLUSH_ML_PURE;             // 200ml
				lFlowSum = lFlowSum_In;           // 입수량

				if (lFlowSum >= flow) {
					lFlowSum = 0;
					lFlowSum_In = 0;
					lFlowSum_Hot = 0;
					
					wFlushingCnt=0;
					bFlushingStep=4;
				}
			}
			else if(bFlushingStep==4){            // 온수관로 배수 + 히터 60도 테스트 (펌프)
				flow = FLUSH_ML_HOT_TEST;         // 250ml
				#if FLUSH_PUMP_BY_HOT
				lFlowSum = lFlowSum_Hot;          // 온수 유량센서 (PE7)
#else
				lFlowSum = lFlowSum_In;           // 임시 : 입수 유량센서 (PE5)
#endif

				if (lFlowSum >= flow) {
					lFlowSum = 0;
					lFlowSum_In = 0;
					lFlowSum_Hot = 0;
					
					wFlushingCnt=0;
					bFlushingStep=5;
				}
			}
			else if(bFlushingStep==5){            // 온수관로 잔열 배수 (히터 OFF, 펌프)
				flow = FLUSH_ML_HOT_COOL;         // 300ml
				#if FLUSH_PUMP_BY_HOT
				lFlowSum = lFlowSum_Hot;          // 온수 유량센서 (PE7)
#else
				lFlowSum = lFlowSum_In;           // 임시 : 입수 유량센서 (PE5)
#endif

				if (lFlowSum >= flow) {
					lFlowSum = 0;
					lFlowSum_In = 0;
					lFlowSum_Hot = 0;
					
					wFlushingCnt=0;
					bFlushingStep=0;
					bFlushingEnd=1;
		
					Key_action();//정지
				}
			}
								
		}
		else{  // 출수시 ---------------------------------------------------------
			
			if(bSet_mL<=2){
				if((s_mode&0x7ff) == HOT ){
					
					if((s_mode&0x7ff) == HOT)	   flow = wML_Cnt_Hot[ bSet_mL]; //llTargetFlow;
					else flow = wML_Cnt[ 0]; //l250*120; //250ml
					
					lFlowSum = lFlowSum_Hot;  // flow_hot_liter; //온수 
				}
				else{
					flow = wML_Cnt[ bSet_mL]; //lTargetFlow;
					lFlowSum =   lFlowSum_Out ;// flow_out_liter; //냉수 알칼리수 정수 
			
				}
				//누수에러
				if((lFlowSum==0) && (lFlowSum_In>6)){
					if(++wFlowSum_ErrCnt>=  30){ //100ms
						wFlowSum_ErrCnt = 0;
						lError |= ERR_LEAK;
					}
				}
				else{
					wFlowSum_ErrCnt = 0;
					lError &= ~ERR_LEAK;
				}
				
				if (lFlowSum >= flow) {
					lFlowSum = 0;
				   
					Key_action();//정지
					
			
				}
				
				
			}
			else{ // 연속출수 시간 제한---------------------------
				
			

				if (++lDispenseTick >= TICKS_2_MINUTES) { //2분 
					//bDispenseRestPeriod = 1; // 휴식 모드 진입
					lDispenseTick = 0;
					//lDispenseRestTick = 0;

					Key_action();           // 즉시 정지
				//	Voice_output(SND_SELECT); // 알림음
				}
				
				//누수에러-----------------------------------------
				if((s_mode&0x7ff) == HOT )	lFlowSum = lFlowSum_Hot;  
				else 									lFlowSum =   lFlowSum_Out ;
				
				if((lFlowSum==0 ) && (lFlowSum_In > 6 )){
					if(++wFlowSum_ErrCnt>=  80) {//100ms    30 ???
						wFlowSum_ErrCnt = 0;
						lError |= ERR_LEAK;
					}
				}
				else{
					wFlowSum_ErrCnt = 0;
					lError &= ~ERR_LEAK;
				}
				
			}
			
		}
	}
	else{
		lFlowSum= 0; 
		lFlowSum_Hot = 0;
		lFlowSum_Out = 0;
		
		wFlowCheckDelayTick = 0;
		wFlowErrorDurationTick = 0;
		
		//if (bDispenseRestPeriod == 0) {
			lDispenseTick = 0;
		//}
		wFlowSum_ErrCnt = 0;

	}
}

void fcError(void)
{
	
	
	//필터 에러 
	if(filter_error)
	 {
		
		 if(m_state&0x02	){ //동작중이었으면 정지 
			 Key_action();
		 }
		 
		 
		 //에러발생시 필터에 사용된 유량 저장 안되게 
		f1_life_save_fg = 0;
		f2_life_save_fg = 0;
		f1_save_cnt = 0;
		f2_save_cnt = 0;
		 
		// 필터 교체시 1초마다 체크
		 if(++b1SecCnt>=2){
			 b1SecCnt=0;
			if((filter_error_fg&0x45)) f1_life_read_fg = 1;
			if((filter_error_fg&0x8a)) f2_life_read_fg = 1;
			filter_life_read_fg = 1;	
		 }			 
		
	  
	 }
	 
	// 온도에러 
	if(temp_error && (m_state==0x02))
	{
		if(++temp_over_cnt>=4)				//2sec
		{	                 
			temp_over_cnt = 0;             	

														
			Pwm_off();
			RELAY_OFF;      
			
			SOL2_OFF;     
			SOL3_OFF;     
			SOL4_OFF;                     		
			SOL5_OFF;     
			SOL6_OFF;                     		
			SOL7_OFF;                     		
			SOL8_OFF; 	               	                    	
											
			if(++temp_err_disp_cnt>10)	// 동작시에만 에라 5초간 표시
			{           
				temp_err_disp_cnt = 0;
				
				
				Key_action();//정지모드
				//m_state &= 0xfd;   // 동작정지
				//ion_ok_fg = 0;
				
				temp_error = 0;	//5초후 온도에러 해지
				lError &= ~ERR_SMPS_TEMP;
			}

		}
	}

	
	//  0.7 liter 이하 이거나 3.5리터 이상이면 점검 표시 및 경고 음성 출력 
	// 플러싱의 펌프 구간(2 온수라인 , 4 히터테스트 , 5 잔열배수)은 마이크로 펌프로
	// 진행해 입수 유량이 낮고, 빈 탱크를 채우는 동안 프라이밍으로 0 이 되기도 한다.
	// 정상 동작이므로 유량 에러 판정에서 제외한다.
	if( ((flow_liter<LOW_LIMIT_LITER) || (flow_liter>HIGH_LIMIT_LITER)) && (m_state&0x02)
	    && !( ((s_mode&0x7ff) == FLUSHING)
	          && ((bFlushingStep == 2) || (bFlushingStep == 4) || (bFlushingStep == 5)) ) )
	{
		// Ad_conversion 은 100ms 주기이므로 1틱 = 100ms
		if((s_mode&0x7ff) == CLEAN)				ERROR_FLOW_TICK = 80;   // 세정 8초
		else if((s_mode&0x7ff) == FLUSHING)		ERROR_FLOW_TICK = 100;  // 플러싱 10초 (단계 전환 여유)
		else									ERROR_FLOW_TICK = 50;   // 5초
		
	  	if(++flow_error_cnt >= ERROR_FLOW_TICK)
	  	{
	  		flow_error_cnt = 0;
	  		if(!flow_error) voice_out_cnt = 50;	//080109
			
			flow_error = 1;	
	  		lError |=  ERR_FLOW ;  //
	  	}
	}
	else
	{
		flow_error_cnt = 0;
		flow_error = 0;
		lError &=  (~ERR_FLOW) ;  
		flow_over_cnt = 0;	            	
	}
	//------------------------------------------------------------------------------------
//	else if((lError == ERR_FLOW )&& lError  && ((s_mode&0x7f0)==HOT)) //온수일때 유량에러시 동작 정지 
	if((lError &ERR_FLOW )  && ((s_mode&0x7f0)==HOT)) 
	{
		//SOL1_CLOSE; //NOS 닫기 , 원수 유입 정지 
		
		Voice_output(SND_ERR );
		
		backlight_off_cnt = 0;    
		backlight_en_fg = 1;
		
		if(++bStopDelayCnt>= 3){
			bStopDelayCnt = 0;
			Key_action(); // 정지
		
		}
	}else if(( lError & ERR_NOS_CLOSE))
	{
		SOL1_CLOSE; //NOS 닫기 , 원수 유입 정지 
		

		bStopDelayCnt = 0;
		backlight_off_cnt = 0;    
		backlight_en_fg = 1;
		
		if(m_state & 0x02)		Key_action();	
		
	}
	else if(( lError & ERR_STOP_ONLY)) //
	{
		//SOL1_CLOSE; //NOS 닫기 , 원수 유입 정지 
		

		bStopDelayCnt = 0;
		backlight_off_cnt = 0;    
		backlight_en_fg = 1;
		
		if(m_state & 0x02)		Key_action();	
		
	}
	
	else if(lError ==ERR_FLOW){
		
		bStopDelayCnt = 0;
		backlight_off_cnt = 0;    
		backlight_en_fg = 1;
	}
	else
	{
		SOL1_OPEN;  //NOS 열기 , 원수 유입 
		
		bStopDelayCnt = 0;
	}
	
	lError_Old = lError;
}
/**************************************************************************************************
		물 유입시 유량
**************************************************************************************************/

void	Flow_in_check(void)		
{
	//ULONG	flow_temp = 0;
	long filter_temp = 0;	//100202_4 
	
		if(fg)	HAL_GPIO_SetPin( (PORT_Type *)PB,_BIT(6));
		else HAL_GPIO_ClearPin((PORT_Type *)PB, _BIT(6));
		fg=~fg;
	
	
  	// 수량 카운트 --------------------------------------------------------------------
  	saved_flow_pulse = flow_pulse_cnt; 		// 500 초당 카운터
	saved_flow_hot_pulse = flow_hot_pulse_cnt;    
	saved_flow_out_pulse = flow_out_pulse_cnt;    
		
	lFlowSum_In    += saved_flow_pulse;
	lFlowSum_Hot  += saved_flow_hot_pulse;
	lFlowSum_Out  += saved_flow_out_pulse;
	
	
  	flow_pulse_cnt = 0;  
	flow_hot_pulse_cnt=0;
	flow_out_pulse_cnt = 0;
	
 	    
  	flow_in_new        = (long)saved_flow_pulse       * 100000L / FLOW_GAIN;    // 500 msec->100ms 20000->100000
	flow_in_hot_new = (long)saved_flow_hot_pulse * 100000L / FLOW_GAIN;
	flow_in_out_new = (long)saved_flow_out_pulse * 100000L / FLOW_GAIN;

  	flow_liter =  (flow_in_new + (flow_liter * 7)) >> 3; //  flow_in_new;//(flow_in_new + flow_in_old) / 2;  	  	
	flow_hot_liter = (flow_in_hot_new + (flow_hot_liter * 7)) >> 3; // flow_in_hot_new;// (flow_in_hot_new + flow_in_hot_old) / 2;  	  
	flow_out_liter = (flow_in_out_new + (flow_out_liter * 7)) >> 3;  //flow_in_out_new;// (flow_in_out_new + flow_in_out_old) / 2;  	  
	
	disp_flow_sum += flow_liter;
	disp_flow_hot_sum += flow_hot_liter;
	disp_flow_out_sum += flow_out_liter;
	if(++disp_flow_hot_cnt>=5){
		disp_flow_hot_cnt=0;
		
		disp_flow_hot =disp_flow_hot_sum/ 5;  
		disp_flow = disp_flow_sum/5;
		disp_flow_out = disp_flow_out_sum/5;
		disp_flow_hot_sum=0;
		disp_flow_sum= 0 ;
		disp_flow_out_sum=0;
	}
	
	
	
  	flow_in_old = flow_liter;
	flow_in_hot_old = flow_hot_liter;
	flow_in_out_old = flow_out_liter;
	
	//flow_temp = flow_liter/  120;
	flow_temp_sum += flow_liter;///  120;
	flow_out_temp_sum += flow_out_liter;
	
	if(++bFlowCnt>=5){
		bFlowCnt= 0;
		
		flow_temp = flow_temp_sum /120  / 5; //
		flow_out_temp = flow_out_temp_sum/120/5;
		flow_temp_sum=0;
		flow_out_temp_sum=0;
		
		//100202_4. 필터값이 1리터 보다 작으면 1리터 저장
		filter_temp = filter1_life - flow_temp;
		if(filter_temp>0)	//1리터 보다 크면
		{
			filter1_life = filter_temp;
		}
		else	//1리터 보다 작으면 1리터 저장
		{
			 // 231005-2 필터 미연결->에러 발생 -> 필터 연결시 에러는 해제되었지만 필터값을 1로 표시하는 경우 있음. 에러가없을때만 1리터로 하고, 에러가 있을때는 이전값을 유지 
			 // 260223 if(((filter_error_fg &0x45)==0 )||((serial_err&0x01)==0))  filter1_life = 1000; 
			 if( ((filter_error_fg &0x45)==0 ) && (m_state&0x02))  filter1_life = 1000; 
		}		
		//151222_2 1L마다 필터 라이프 저장
		wF1_life = (WORD)filter1_life/1000;
		

		if(wF1_life!=wF1_life_old) {
			f1_life_save_fg = 1;
		}
		else {
			//231005 f1_life_save_fg = 0; 저장방법 변경
		}

		wF1_life_old = wF1_life;
		
		
		filter_temp = filter2_life - flow_temp;
		if(filter_temp>0)	//1리터 ->0
		{
			filter2_life = filter_temp;
		}
		else	//1리터 보다 작으면 1리터 저장
		{
			//231005-2 필터 미연결->에러 발생 -> 필터 연결시 에러는 해제되었지만 필터값을 1로 표시하는 경우 있음. 에러가없을때만 1리터로 하고, 에러가 있을때는 이전값을 유지 
			// 260223 if(((filter_error_fg &0x8a)==0)||((serial_err&0x02)==0))  filter2_life =  1000;
			if(((filter_error_fg &0x8a)==0 )&& (m_state&0x02))  filter2_life =  1000;
		}		
		//151222_2 1L마다 필터 라이프 저장
		wF2_life = (WORD)filter2_life/1000;

		if(wF2_life!=wF2_life_old) {
			f2_life_save_fg = 1;
		}
		else {
			//231005 f2_life_save_fg = 0; 저장방법 변경
		}

		wF2_life_old = wF2_life;

		// 자동세정 	-----------------------------------------------------------------
		auto_clean_cnt +=  flow_temp; //  ?? (flow_liter/120);  
		
		//??????????????????????????????????????????????????????????????????????????????????????????????????????????????????
		//??????????????????????????????????????????????????????????????????????????????????????????????????????????????????
		/*if(auto_clean_value==1)	auto_temp = auto_clean_value*1000L ;
		else auto_temp =   (auto_clean_value*10000L);
		
		if(auto_clean_cnt >= auto_temp) // (auto_clean_value*10000L))	// 설정한 리터보다 크면 자동세정 동작
		{
			if(auto_clean_fg==0)
			{
				auto_clean_fg = 1;				// 자동세정시작
				
				Voice_output(SND_CLEAN+language_jump);
				
				voice_out_cnt = 350;   // 세정 멘트 시간후 경보음
				auto_clean_cnt = 0;
			}
		}*/
	}
	// 출수량 체크-----------------------------------------------------------------
	Output_mL_Control();

	
  	//1초마다 솔레노이드 제어 -------------------------------------------------------
  	if(++m_n1Sec_cnt>=2)
  	{
		m_n1Sec_cnt = 0;
  	
  		//fcError(); //Output_control(); 
  	}
}
/*
void	Flow_in_check(void)		
{
	ULONG	flow_temp = 0;
	long filter_temp = 0;	//100202_4 
	
  	// 수량 카운트 --------------------------------------------------------------------
  	saved_flow_pulse = flow_pulse_cnt; 		// 500 초당 카운터
	saved_flow_hot_pulse = flow_hot_pulse_cnt;    
	saved_flow_out_pulse = flow_out_pulse_cnt;    
		
	//lFlowSum_Disp  += saved_flow_pulse;
	lFlowSum_Hot  += flow_hot_pulse_cnt;
	lFlowSum_Out  += flow_out_pulse_cnt;
	
	
  	flow_pulse_cnt = 0;  
	flow_hot_pulse_cnt=0;
	flow_out_pulse_cnt = 0;
	
 	    
  	flow_in_new        = (long)saved_flow_pulse       * 20000L / FLOW_GAIN;    // 500 msec->100ms 20000->100000
	flow_in_hot_new = (long)saved_flow_hot_pulse * 20000L / FLOW_GAIN;
	flow_in_out_new = (long)saved_flow_out_pulse * 20000L / FLOW_GAIN;

  	flow_liter = flow_in_new;//  (flow_in_new + (flow_liter * 7)) >> 3; //  flow_in_new;//(flow_in_new + flow_in_old) / 2;  	  	
	flow_hot_liter =flow_in_hot_new;// (flow_in_hot_new + (flow_hot_liter * 7)) >> 3; // flow_in_hot_new;// (flow_in_hot_new + flow_in_hot_old) / 2;  	  
	flow_out_liter =flow_in_out_new;// (flow_in_out_new + (flow_out_liter * 7)) >> 3;  //flow_in_out_new;// (flow_in_out_new + flow_in_out_old) / 2;  	  
	
  	flow_in_old = flow_liter;
	flow_in_hot_old = flow_hot_liter;
	flow_in_out_old = flow_out_liter;
	
	flow_temp = flow_liter/  120;
	
	//100202_4. 필터값이 1리터 보다 작으면 1리터 저장
	filter_temp = filter1_life - flow_temp;
	if(filter_temp>1000)	//1리터 보다 크면
	{
		filter1_life = filter_temp;
	}
	else	//1리터 보다 작으면 1리터 저장
	{
		//231005-2 필터 미연결->에러 발생 -> 필터 연결시 에러는 해제되었지만 필터값을 1로 표시하는 경우 있음. 에러가없을때만 1리터로 하고, 에러가 있을때는 이전값을 유지 
			// 260223 if(((filter_error_fg &0x45)==0 )||((serial_err&0x01)==0))  filter1_life = 1000; 
		 if( ((filter_error_fg &0x45)==0 ) && (m_state&0x02))  filter1_life = 1000; 
	}		
	//151222_2 1L마다 필터 라이프 저장
	wF1_life = (WORD)filter1_life/1000;
	

	if(wF1_life!=wF1_life_old) {
		f1_life_save_fg = 1;
	}
	else {
		//231005 f1_life_save_fg = 0; 저장방법 변경
	}

	wF1_life_old = wF1_life;
	
	
	filter_temp = filter2_life - flow_temp;
	if(filter_temp>1000)	//1리터 보다 크면
	{
		filter2_life = filter_temp;
	}
	else	//1리터 보다 작으면 1리터 저장
	{
		//231005-2 필터 미연결->에러 발생 -> 필터 연결시 에러는 해제되었지만 필터값을 1로 표시하는 경우 있음. 에러가없을때만 1리터로 하고, 에러가 있을때는 이전값을 유지 
		// 260223 if(((filter_error_fg &0x8a)==0)||((serial_err&0x02)==0))  filter2_life =  1000;
		if(((filter_error_fg &0x8a)==0 )&& (m_state&0x02))  filter2_life =  1000;
	}		
	//151222_2 1L마다 필터 라이프 저장
	wF2_life = (WORD)filter2_life/1000;

	if(wF2_life!=wF2_life_old) {
		f2_life_save_fg = 1;
	}
	else {
		//231005 f2_life_save_fg = 0; 저장방법 변경
	}

	wF2_life_old = wF2_life;

  	// 자동세정 	-----------------------------------------------------------------
  	auto_clean_cnt +=  flow_temp; //  ?? (flow_liter/120);  
  	
  	if(auto_clean_cnt >=  (auto_clean_value*10000L))	// 설정한 리터보다 크면 자동세정 동작
  	{
		if(auto_clean_fg==0)
  	  	{
  	    	auto_clean_fg = 1;				// 자동세정시작
		 	
		 	Voice_output(SND_CLEAN+language_jump);
			
  			voice_out_cnt = 550;   // 세정 멘트 시간후 경보음
  	    	auto_clean_cnt = 0;
  	  	}
  	}

	// 출수량 체크-----------------------------------------------------------------
	Output_mL_Control();

	
  	
}
*/
/**************************************************************************************************
		솔레로이드 출력시간 제어
**************************************************************************************************/
void  Output_control(void)	//1sec -> 0.1
{
 	//if(filter_error==0 )	//error==0
	{
		if(m_state&0x80)   // 단수
		{
			switch(s_mode_old)	// s_mode -> s_mode_old
			{
				case PH_SET :         // 알카리 설정시 
				case PH_SET2 : 
				case PH_SET3: 
					if(++ion_stop_cnt>=CLEAN_TIME)
					{
						after_clean_fg = 0;		//  후세정 후 전류인가 금지하게 하기 위해 		
						
						RELAY_OFF; // 알카리수
						Pwm_off(); 
						
						pi_value = 0;
						error_0_old = 0;
						
						ion_stop_cnt = 0;
						m_state &= 0x7d;       	// 대기 상태로 복귀
				
						Pwm_off();
						
						
						SOL3_OFF; 
						SOL4_OFF;
						SOL6_OFF; 
						SOL8_OFF;
					}
					else if(ion_stop_cnt>20)	
					{
						RELAY_ON;       
						
						SOL3_ON; 
						SOL4_OFF;
						SOL6_ON; 
						SOL8_ON;
					}
					else if(ion_stop_cnt>0)	
					{
						
						SOL4_OFF; 
						
						ionize_fg=0;
						flow_in_fg=0;  			// 물유입 정지
						cleaning_fg=0;
						
						
						wUvcCorkCnt = 0;
						bUvcCorkFg = 0;
					
					}
					break;

				default :         
					break;
			}
		
			switch(s_mode_old&0xff0)		//정지하기 이전 모드로 출력 제어
			{
				case CLEAN :             // 세정
					/*if(++ion_stop_cnt>=620) //???
					{
						RELAY_OFF;          		// 알카리수 출수
						Pwm_off();
						POWER_C_OFF;          
						ion_stop_cnt = 0;
						m_state &= 0x7d;       	// 대기 상태로 복귀
			
						wUvcCorkCnt = 0;
						bUvcCorkFg = 0;
						 
					}
					else if(++ion_stop_cnt>=CLEAN_TIME)	 //???
					{
						after_clean_fg = 0;		//  후세정 후 전류인가 금지하게 하기 위해
							

						// pi제어 초기값 입력
						pi_value = 0;
						error_0_old = 0;

						Pwm_off();
						POWER_C_OFF;        		
					}
					else if(ion_stop_cnt==1)		// 080416 출수 시간 변경 080514_4
					{ */
						//???
						after_clean_fg = 0;		//  후세정 후 전류인가 금지하게 하기 위해
						
						m_state &= 0x7d;       	// 대기 상태로 복귀
						after_clean_fg = 0;		//  후세정 후 전류인가 금지하게 하기 위해
						// pi제어 초기값 입력
						pi_value = 0;
						error_0_old = 0;
						
						//s_mode = s_mode_return;
						
						
						RELAY_OFF;          		// 알카리수 출수
						Pwm_off();
						
						ionize_fg=0;
						flow_in_fg=0;  			// 물유입 정지
						cleaning_fg=0;
			
						SOL3_OFF; 
						SOL4_OFF;
						SOL6_OFF; 
						SOL8_OFF;
						

						SOL2_OFF;
						SOL4_OFF;
						SOL5_OFF;
						SOL7_OFF;
					
					//}
					break;
				case ALKA :             				// 알카리
					
					if(++ion_stop_cnt>=CLEAN_TIME)	
					{
						after_clean_fg = 0;			
						
						RELAY_OFF;           		
						SOL3_OFF;
						SOL6_OFF;
						SOL7_OFF;                       // ★ 추가
						SOL8_OFF;
						SOL4_OFF; 
		
						pi_value = 0;
						error_0_old = 0;
						ion_stop_cnt = 0;
						m_state &= 0x7d;       		// 대기 상태로 복귀

						Pwm_off();
					}
					else if(ion_stop_cnt==20) //20=2초후
					{
						RELAY_ON;           		// 극성 on
						
						SOL3_ON;
						SOL6_ON;
						SOL8_ON;
					}
					else if(ion_stop_cnt==3)            // Pressure Purge 종료
					{
						SOL6_OFF;
						SOL7_OFF;
						SOL8_OFF;
					}
					else if(ion_stop_cnt==1)		
					{
						ionize_fg=0;
						flow_in_fg=0;  				// 물유입 정지
						
						SOL3_OFF;                       // 정수 급수 차단
						SOL4_OFF; 
				
						SOL6_ON;                        // ★ 추가 : 6,7,8 동시 OPEN
						SOL7_ON;
						SOL8_ON;
						
						wUvcCorkCnt = 0;
						bUvcCorkFg = 1;
						bUvcCoolFg = 1;
					}
					break;  
				case COOLALKALI :             				// 알카리
					
					
					if(++ion_stop_cnt>=CLEAN_TIME)	
					{
						after_clean_fg = 0;			
						
						RELAY_OFF;           		
						SOL2_OFF;
						SOL6_OFF;
						SOL7_OFF;                       // ★ 추가
						SOL8_OFF;
						SOL4_OFF; 
		
						pi_value = 0;
						error_0_old = 0;
						ion_stop_cnt = 0;
						m_state &= 0x7d;       		// 대기 상태로 복귀

						Pwm_off();
					}
					else if(ion_stop_cnt==20) //2초후
					{
						RELAY_ON;           		// 산성수 출수
						SOL2_ON;
						SOL6_ON;
						SOL8_ON;
					}
					else if(ion_stop_cnt==3)            // ★ 추가 : Pressure Purge 종료
					{
						SOL6_OFF;
						SOL7_OFF;
						SOL8_OFF;
					}
					else if(ion_stop_cnt==1)		
					{
						ionize_fg=0;
						flow_in_fg=0;  				// 물유입 정지
						
						SOL2_OFF;                       // ★ 추가 : 냉수 급수 차단
						SOL4_OFF; 
						
						SOL6_ON;                        // ★ 추가 : 6,7,8 동시 OPEN
						SOL7_ON;
						SOL8_ON;
						
						wUvcCorkCnt = 0;
						bUvcCorkFg = 1;
						bUvcCoolFg = 1;
					}
					break;  
				case PURE :             				// 정수
						
					if(++ion_stop_cnt >= CLEAN_TIME)             // 0.2초 퍼지 완료
					{
						after_clean_fg = 0;

						Pwm_off();
						RELAY_OFF;

						SOL6_OFF;
						SOL7_OFF;
						SOL8_OFF;

						pi_value = 0;
						error_0_old = 0;
						ion_stop_cnt = 0;
						m_state &= 0x7d;       		// 대기 상태로 복귀
					}
					else if(ion_stop_cnt == 1)          // Pressure Purge 시작
					{
						ionize_fg = 0;
						flow_in_fg = 0;                 // 물유입 정지

						SOL3_OFF;                       // 정수 급수 차단
						SOL4_OFF;

						SOL6_ON;                        // 6,7,8 동시 OPEN
						SOL7_ON;
						SOL8_ON;

						wUvcCorkCnt = 0;
						bUvcCorkFg = 1;
						bUvcCoolFg = 1;
					}
					
					
					break;
				case COOL:

						 if(++ion_stop_cnt >= CLEAN_TIME)             // 0.2초 퍼지 완료
						{
							after_clean_fg = 0;

							Pwm_off();
							RELAY_OFF;

							SOL6_OFF;
							SOL7_OFF;
							SOL8_OFF;

							pi_value = 0;
							error_0_old = 0;
							ion_stop_cnt = 0;
							m_state &= 0x7d;       		// 대기 상태로 복귀
						}
						else if(ion_stop_cnt == 1)          // Pressure Purge 시작
						{
							ionize_fg = 0;
							flow_in_fg = 0;                 // 물유입 정지

							SOL2_OFF;                       // 냉수 급수 차단
							SOL4_OFF;

							SOL6_ON;                        // 6,7,8 동시 OPEN
							SOL7_ON;
							SOL8_ON;

							wUvcCorkCnt = 0;
							bUvcCorkFg = 1;
							bUvcCoolFg = 1;
						}
			
					
					break;	
				case HOT:

					if(++wHot_stop_cnt>= 152) {// 15sec
							SOL5_OFF;
							
							SOL6_OFF;
							SOL7_OFF;
							SOL8_OFF;
							
							wHot_stop_cnt = 0;
							m_state &= 0x7d;       		// 대기 상태로 복귀
							

					}
					else if(wHot_stop_cnt== 150) {// 0.2sec
						
						SOL5_OFF;
						
						SOL6_ON;
						SOL7_ON;
						SOL8_ON;
						
				
					}
					else if(wHot_stop_cnt== 3) {// 2
						SOL5_OFF;
						
						SOL6_OFF;
						SOL7_OFF;
						SOL8_OFF;	
					
					}
					
					else if(wHot_stop_cnt==1){

						ionize_fg=0;
						flow_in_fg=0;  		
						 
						SOL5_OFF;
						
						SOL6_ON;     // ???
						 
						SOL7_ON;
						SOL8_ON;
						 
						wUvcCorkCnt = 0;
						bUvcCorkFg = 1;
						bUvcCoolFg = 1;
						
						
						bHotOutEndFg=1;
						wHotOutEndCnt=0;
						
						
						wHotOut_WaitTimeCnt=0;		
						bHotOut_WaitFg=1;

					}

					
				
					break;	
				
				
				case FLUSHING:
						RELAY_OFF;           		
						SOL2_OFF;
						SOL3_OFF;	
						SOL4_OFF;
						SOL5_OFF;
						SOL6_OFF;
						SOL7_OFF;	
						SOL8_OFF;
				
						ionize_fg=0;
						flow_in_fg=0;  		
						
				break;
				default :
					break;  
			}
		}//단수 동작 완료
	  
		if(m_state & 0x02)         		// 출수---------------------------------------
		{
			backlight_off_cnt = 0;       // 백라인트 항상 구동   
			backlight_en_fg = 1;
	
			switch(s_mode)
			{
				case ION_OUT1 :         // 알카리 설정시 
				case ION_OUT2 :	
					SOL2_OFF;
					SOL5_OFF;	
					SOL6_OFF;
					SOL7_OFF;				
				
					RELAY_OFF;  // 알칼리 출수
									
					SOL3_ON;  
					SOL4_ON;  
					SOL8_ON;
	
					Pwm_out(pwm_value);	
					
					flow_in_fg=1;
					ionize_fg=1;
				
					wUvcCorkCnt = 0;
					bUvcCorkFg = 0;
					break;
				case CAL_OUT :         // PH_SET3
					
					SOL2_OFF;
					SOL5_OFF;	
					SOL6_OFF;
					SOL7_OFF;				
				
					RELAY_OFF;  // 알칼리 출수
									
					SOL3_ON;  
					SOL4_ON;  
					SOL8_ON;

					Pwm_out(pwm_value);				// 080418_2 CAL시 전류출력
						
					flow_in_fg=1;
					ionize_fg=1;
				
					wUvcCorkCnt = 0;
					bUvcCorkFg = 0;
				
					break;
				default :         
					break;
			}
  	    
			switch(s_mode&0xff0)
			{
				case CLEAN_OUT :  // 세정 출수
					SOL2_OFF;
					SOL4_OFF;	
					SOL5_OFF;
					SOL7_OFF;
				
					
					RELAY_ON;          // 세정 출수시 극성 변경 
					
					SOL3_ON;  
					SOL6_ON;  
					SOL8_ON;  
					
		   
					flow_in_fg=1;
					cleaning_fg=1;
					ionize_fg=1;
					
					wUvcCorkCnt = 0;
					bUvcCorkFg = 0;
					break;
				case ALKA_OUT : 	// 알칼리 출수
					SOL2_OFF;
					SOL5_OFF;	
					SOL6_OFF;
					SOL7_OFF;
					
					//Pwm_off();
					RELAY_OFF;         
									
					SOL3_ON;  
					SOL4_ON;  
					SOL8_ON;
	
					flow_in_fg=1;
					ionize_fg=1;
					
					wUvcCorkCnt = 0;
					bUvcCorkFg = 0;
					break;
	
				case PURE_OUT :        // 정수 출수
					SOL2_OFF;
					SOL5_OFF;	
					SOL6_OFF;
					SOL7_OFF;
					SOL8_OFF;
				
					Pwm_off();
					RELAY_OFF;        
					
					SOL3_ON;
					SOL4_ON;
							
					ph_temp = 70;
					flow_in_fg=1;
					ionize_fg = 0;
					
					wUvcCorkCnt = 0;
					bUvcCorkFg = 0;
					break;
				
				case HOT_OUT:// 온수  출수 
					SOL2_OFF;
					SOL3_OFF;	
					SOL4_OFF;
					
				
					Pwm_off();
					RELAY_OFF;        		
				
				
					if(++wHot_out_cnt<= 7) {// 0.7sec , 7 8 on ->off
						SOL6_OFF;
						
						SOL7_ON;
						SOL8_ON;
						
						SOL5_OFF;
					}
					else{  //kkk
						SOL6_OFF;
						SOL7_OFF;
						SOL8_OFF;
						
						SOL5_ON; // 출수 시작 
					}
				
					
					flow_in_fg = 1;
					ionize_fg = 0;
					
					bHotOutEndFg=0;
					wHotOutEndCnt=0;
					
					wUvcCorkCnt = 0;
					bUvcCorkFg = 0;
					break;
					
				case COOL_OUT:
					SOL3_OFF;
					SOL5_OFF;	
					SOL6_OFF;
					SOL7_OFF;
					SOL8_OFF;

					Pwm_off();
					RELAY_OFF;        		// 알칼리 출수 
				
				
					SOL2_ON;
					SOL4_ON;	

					flow_in_fg = 1;
					ionize_fg = 0;	
				
					wUvcCorkCnt = 0;
					bUvcCorkFg = 0;
					break;	
					
				case COOLALKALI_OUT : 	// 냉 알칼리 출수
				    SOL3_OFF;  
					SOL5_OFF;
					SOL6_OFF;	
					SOL7_OFF;

					//Pwm_off();
					RELAY_OFF;        		// 알칼리 출수 
								
					SOL2_ON;
					SOL4_ON;  
					SOL8_ON;
	
					flow_in_fg=1;
					ionize_fg=1;
					
				
					wUvcCorkCnt = 0;
					bUvcCorkFg = 0;
					break;
					
				case FLUSHING_OUT:
						//3liter 배수
					wUvcCorkCnt = 0;
					bUvcCorkFg = 0;
				
					ionize_fg=0;
					flow_in_fg=1;  	
				
					bFlushHeaterFg = 0;  
				
					if(bFlushingStep==0){
							SOL2_OFF;
							SOL4_OFF;
							SOL5_OFF;
							SOL7_OFF;
							
							SOL3_ON;
							SOL6_ON;
							SOL8_ON;
					}
					
					else if(bFlushingStep==1){ //냉수라인 물채우기 
									
					
							//SOL1_OFF;
							SOL3_OFF;
							SOL4_OFF;
							SOL5_OFF;
							SOL7_OFF;
							
							SOL2_ON;
							SOL6_ON;
							SOL8_ON;
					}
					else if(bFlushingStep==2){ ///온수라인 물채우기
						
						//SOL1_OFF;
						SOL2_OFF;
						SOL3_OFF;
						SOL4_OFF;
						SOL5_OFF;
						SOL6_OFF;					
						
						SOL7_ON;
						SOL8_ON;
						
						PwmHotPump_out(FLUSH_PUMP_PWM);
					}
					else if(bFlushingStep==3){ //정수라인 물채우기
						
							PwmHotPump_off();
							//SOL1_OFF;
							SOL2_OFF;
							SOL4_OFF;
							SOL5_OFF;
							SOL7_OFF;
				
							SOL3_ON;
							SOL6_ON;
							SOL8_ON;

				}
				else if(bFlushingStep==4){ //온수관로 배수 + 히터 60도 테스트
						
						SOL2_OFF;
						SOL3_OFF;
						SOL4_OFF;
						SOL5_OFF;
						SOL6_OFF;
						
						SOL7_ON;                 // 온수 배수
						SOL8_ON;                 // 산성수 배수
						
						PwmHotPump_out(FLUSH_PUMP_PWM);    // 마이크로 펌프 ON
						
						bFlushHeaterFg = 1;      //  히터 테스트 구간
					}
					else if(bFlushingStep==5){ //온수관로 잔열 배수 (히터 OFF)
						
						SOL2_OFF;
						SOL3_OFF;
						SOL4_OFF;
						SOL5_OFF;
						SOL6_OFF;
						
						SOL7_ON;
						SOL8_ON;
						
						PwmHotPump_out(FLUSH_PUMP_PWM);
						
						bFlushHeaterFg = 0;      //  히터 OFF, 잔열만 배수
					}
					break;
				default :         
					bFlushHeaterFg = 0;  
					break;
			}
		}//출수동작 끝
		else
		{
			ionize_fg=0;   // 전해중
			flow_in_fg=0;  // 물유입
			cleaning_fg=0;
					
			bFlushHeaterFg = 0;     
		}
		
	
				#if	DEBUG_FLOW
						if(flow_in_fg){
							SOL6_ON;  
							SOL7_ON;  
							SOL8_ON;  
						}else{
							SOL6_OFF;  
							SOL7_OFF;  
							SOL8_OFF;  
						}
					#endif
		
		
	}
/*	else	//filter_error == 1 필터 error 발생시
	{
		if(++error_stop_cnt>=1200) //120sec
		{
			  error_stop_cnt = 1201;       	

			  RELAY_OFF;    
			  Pwm_off();
			  
			  SOL2_OFF;     
			  SOL3_OFF;     
			  SOL4_OFF;                     		
			  SOL5_OFF;     
			  SOL6_OFF;                     		
			  SOL7_OFF;                     		
			  SOL8_OFF; 	               	     		
		}                                		
		else if(error_stop_cnt>0)  //120초 동안 에라 체크      		
		{                                		
			

			SOL6_ON;                     		
			SOL7_ON;                     		
			SOL8_ON; 	                    		                    		
												
			//정지모드	               		
			m_state &= 0xfd;  // 동작정지
			ion_ok_fg = 0;                		
												
			new_active = 0;               		
			auto_clean_fg=0;              		
			auto_clean_cnt=0;             		
												
			m_state |= 0x80;   // 단수 동작 시작 
			s_mode &= 0x7ff;
			
			//080529_1 필터 교체시 1초마다 체크
			if((filter_error_fg&0x45)||(serial_err)) f1_life_read_fg = 1;
			if((filter_error_fg&0x8a)||(serial_err)) f2_life_read_fg = 1;
			filter_life_read_fg = 1;		
			
		   //에러발생시 필터에 사용된 유량 저장 안되게 
			f1_life_save_fg = 0;
			f2_life_save_fg = 0;
			f1_save_cnt = 0;
			f2_save_cnt = 0;
		
		}
	}//error 발생 끝
	 */
	 
	 /*
	 if(filter_error)
	 {
		 //동작중이었으면 정지 
		 if(m_state&0x02	){
			 Key_action();
		 }
		 
		 
		 //에러발생시 필터에 사용된 유량 저장 안되게 
		f1_life_save_fg = 0;
		f2_life_save_fg = 0;
		f1_save_cnt = 0;
		f2_save_cnt = 0;
		// 필터 교체시 1초마다 체크
		 
		 
		if((filter_error_fg&0x45)||(serial_err)) f1_life_read_fg = 1;
		if((filter_error_fg&0x8a)||(serial_err)) f2_life_read_fg = 1;
		filter_life_read_fg = 1;		
		
	  
	 }
	 
	// 온도에러 
	if(temp_error && (m_state==0x02))
	{
		if(++temp_over_cnt>=20)				//2sec
		{	                 
			temp_over_cnt = 0;             	

														
			Pwm_off();
			RELAY_OFF;      
			
			SOL2_OFF;     
			SOL3_OFF;     
			SOL4_OFF;                     		
			SOL5_OFF;     
			SOL6_OFF;                     		
			SOL7_OFF;                     		
			SOL8_OFF; 	               	                    	
											
			if(++temp_err_disp_cnt>50)	// 동작시에만 에라 5초간 표시
			{           
				temp_err_disp_cnt = 0;
				
				//정지모드	
				m_state &= 0xfd;   // 동작정지
				ion_ok_fg = 0;
				
				temp_error = 0;	//5초후 온도에러 해지
				lError &= ~ERR_SMPS_TEMP;
			}

		}
	}

	
  	
	// 물 유입 없을때  ??? 
	//if(flow_liter<LOW_LIMIT_LITER && (!(m_state&0x80)))  //  0.7 liter 이하이면 전해조를 멈춘다.
	//{	
		// pi제어 초기값 입력
	//	pi_value = 0;
	//	error_0_old = 0;

	//	Pwm_off();//PWM_DATA  = 0;   
	//	RELAY_OFF;

	//}

	//  0.7 liter 이하 이거나 3.5리터 이상이면 점검 표시 및 경고 음성 출력 
	if((flow_liter<LOW_LIMIT_LITER || flow_liter>HIGH_LIMIT_LITER) && (m_state&0x02) )   // 문 열림
	{
		if((s_mode&0x7ff) == CLEAN	)	ERROR_FLOW_TICK = 80; //세정시 5초후 동작함
		else ERROR_FLOW_TICK = 50;
		
	  	if(++flow_error_cnt >= ERROR_FLOW_TICK)					// 50-5sec_delay
	  	{
	  		flow_error_cnt = 0;
	  		if(!flow_error) voice_out_cnt = 50;	//080109
			
			flow_error = 1;	
	  		lError |=  ERR_FLOW ;  //
	  	}
	}
	else
	{
		flow_error_cnt = 0;
		flow_error = 0;
		lError &=  (~ERR_FLOW) ;  
		flow_over_cnt = 0;	            	
	}*/
}


//500ms
void Uvc_Control(void)
{
	//con_uv_min_time
	//con_uv_sec_time
	if(bUvcCoolFg) { // && !(m_state&0x02 )&& !(m_state&0x80 ) ){
		
		if (wUvcCoolCnt < con_uv_run_time) { //500ms*2*600 =10분
			UVC1_ON; //냉수 uvc
		
		} else {
			UVC1_OFF;
		
		}

		// 로직 실행 후 카운트 증가 및 1시간(3600초) 리셋
		if (++wUvcCoolCnt >= con_uv_time) { //500ms *2 * 3600 
			wUvcCoolCnt = 0;
		}
	}		
	
	
	//cork  출수  5초 
	if(bUvcCorkFg ) {
		
		
		if( (m_state&0x02)==0){
			UVC2_ON;
			
			if(++wUvcCorkCnt >= con_uv_cork_sec_time ) { //500ms*10 = 5000ms
				wUvcCorkCnt = 0;
				
				UVC2_OFF;
				
				bUvcCorkFg = 0;
			}			
		}
		//else{
		//	wUvcCorkCnt = 0;	
		//	bUvcCorkFg = 0;
		//	UVC2_OFF;
				
				
		//}
	}
	else{
		
		UVC2_OFF;
	}
}

void Door_Check(void)
{
	
    if ( (HAL_GPIO_ReadPin(PE) & _BIT(14)) )  //1= 문 열림 
    {
		if(wDoorCnt < 200)  wDoorCnt++;
       
        if ( wDoorCnt >= 200) //10ms *200 2sec
        {
            bDoorState = 1;
			
		    lError |= ERR_COVER_OPEN;
			
        }
    }
    else 
    {
        wDoorCnt = 0;
        bDoorState = 0;
        lError &= ~ERR_COVER_OPEN;
    }
	
	//커버 열림 시간 체크
/*	if (bDoorState == 1)
    {
        // 현재 카운트가 20초(2000) 미만일 때만 증가시킴
        // 20초에 도달하면 조건문이 거짓이 되어 2000으로 계속 고정됨
        if (wDoorOpenCnt < DOOR_OPEN_20SEC_CNT) 
        {
            wDoorOpenCnt++;
			bDoorOpen_20SecFg=1;
        }
		else{
			bDoorOpen_20SecFg=2;
		}
    }
	
    
    // 문이 닫히는 순간 감지 (열림 '1' -> 닫힘 '0' 상태 변화)
    if (bPrevDoorState == 1 && bDoorState == 0)
    {
        // 누적된 시간(wDoorOpenCnt)이 20초로 꽉 차 있는지 판별
        if (wDoorOpenCnt >= DOOR_OPEN_20SEC_CNT)
        {
            s_mode = FLUSHING;
			s_mode_old= FLUSHING;
			
			lError &= ~ERR_COVER_OPEN;
			
			first_clean_en_fg = 0; 
			
			Key_action();
        }
        else
        {
            // [20초 이내에 닫힘] 정상 동작 
			
			s_mode = s_mode_return;
			s_mode_return=s_mode;
        }
        
        wDoorOpenCnt = 0;
		bDoorOpen_20SecFg=0;
    }

    bPrevDoorState = bDoorState;
	
	
*/
	

}

/*
void FlashWrite(void)
{
	uint32_t	i;
	uint32_t SectorSize = 512;
	uint32_t byteSize = 256;
	uint8_t result;


	eeprom_save_fg = 1;
	
	for(i = START_ADDR; i < END_ADDR; i+= SectorSize) {	
		result = HAL_DFMC_EraseSector(DATA_OPTION_NOT_USE, i); // 512B Sector Erase	
	}

	for(i = START_ADDR; i < (END_ADDR+ byteSize); i+= byteSize) {
		result = HAL_DFMC_WordProgram(DATA_OPTION_NOT_USE, i , byteSize, Buffer);
	}
	eeprom_save_fg = 0;

}*/

// 데이터 플래시 1바이트 읽기 (비정렬 32비트 접근 금지 -> 바이트 단위로 읽는다)
static BYTE Rb_Rd(WORD addr)
{
	return *(volatile uint8_t *)(START_ADDR + addr);
}

// 되쓰기 차단 기록을 Buffer 에 반영
//  Buffer 는 부팅시 0 이고 플래시에서 되읽지 않으므로 FlashWrite 마다 채워야 한다
void Rb_Sync_Buffer(void)
{
	BYTE i, p;

	Buffer[ADD_RB_MAGIC]   = (BYTE)( wRbMagic       & 0xff);
	Buffer[ADD_RB_MAGIC+1] = (BYTE)((wRbMagic >> 8) & 0xff);
	Buffer[ADD_RB_IDX1]    = bRbIdx1;
	Buffer[ADD_RB_IDX2]    = bRbIdx2;

	for(i=0;i<RB_HIST;i++)
	{
		p = (BYTE)(ADD_RB_TBL1 + i*6);
		Buffer[p+0] = (BYTE)( lRbId1[i]         & 0xff);
		Buffer[p+1] = (BYTE)((lRbId1[i] >>  8)  & 0xff);
		Buffer[p+2] = (BYTE)((lRbId1[i] >> 16)  & 0xff);
		Buffer[p+3] = (BYTE)((lRbId1[i] >> 24)  & 0xff);
		Buffer[p+4] = (BYTE)( wRbLife1[i]       & 0xff);
		Buffer[p+5] = (BYTE)((wRbLife1[i] >> 8) & 0xff);

		p = (BYTE)(ADD_RB_TBL2 + i*6);
		Buffer[p+0] = (BYTE)( lRbId2[i]         & 0xff);
		Buffer[p+1] = (BYTE)((lRbId2[i] >>  8)  & 0xff);
		Buffer[p+2] = (BYTE)((lRbId2[i] >> 16)  & 0xff);
		Buffer[p+3] = (BYTE)((lRbId2[i] >> 24)  & 0xff);
		Buffer[p+4] = (BYTE)( wRbLife2[i]       & 0xff);
		Buffer[p+5] = (BYTE)((wRbLife2[i] >> 8) & 0xff);
	}
}

// 되쓰기 차단 기록 읽기 : Set_eep_init 맨 앞에서 부른다
void Rb_Load(void)
{
	BYTE i;
	WORD p;

	wRbMagic = (WORD)Rb_Rd(ADD_RB_MAGIC) | ((WORD)Rb_Rd(ADD_RB_MAGIC+1) << 8);

	if(wRbMagic != RB_MAGIC)
	{
		// 기록 없음 (최초 적용 / 기판 교체) -> 첫 칩 읽기에서 현재 상태로 등록된다
		wRbMagic = 0;
		bRbIdx1  = 0;	bRbIdx2 = 0;
		for(i=0;i<RB_HIST;i++){
			lRbId1[i] = 0;	wRbLife1[i] = 0;
			lRbId2[i] = 0;	wRbLife2[i] = 0;
		}
		Rb_Sync_Buffer();
		return;
	}

	bRbIdx1 = Rb_Rd(ADD_RB_IDX1);	if(bRbIdx1 >= RB_HIST)	bRbIdx1 = 0;
	bRbIdx2 = Rb_Rd(ADD_RB_IDX2);	if(bRbIdx2 >= RB_HIST)	bRbIdx2 = 0;

	for(i=0;i<RB_HIST;i++)
	{
		p = (WORD)(ADD_RB_TBL1 + i*6);
		lRbId1[i]   = (ULONG)Rb_Rd(p) | ((ULONG)Rb_Rd(p+1)<<8)
		            | ((ULONG)Rb_Rd(p+2)<<16) | ((ULONG)Rb_Rd(p+3)<<24);
		wRbLife1[i] = (WORD)Rb_Rd(p+4) | ((WORD)Rb_Rd(p+5)<<8);
		if(wRbLife1[i] > (WORD)(F1_MAX_LIFE/1000))	wRbLife1[i] = (WORD)(F1_MAX_LIFE/1000);

		p = (WORD)(ADD_RB_TBL2 + i*6);
		lRbId2[i]   = (ULONG)Rb_Rd(p) | ((ULONG)Rb_Rd(p+1)<<8)
		            | ((ULONG)Rb_Rd(p+2)<<16) | ((ULONG)Rb_Rd(p+3)<<24);
		wRbLife2[i] = (WORD)Rb_Rd(p+4) | ((WORD)Rb_Rd(p+5)<<8);
		if(wRbLife2[i] > (WORD)(F2_MAX_LIFE/1000))	wRbLife2[i] = (WORD)(F2_MAX_LIFE/1000);
	}

	Rb_Sync_Buffer();
}

// 되쓰기(롤백) 판정
//  ch : 1 = 필터1 , 2 = 필터2
//  반환 : 0 = 정상 , 1 = 되쓰기 감지 -> 호출부에서 E03 / E04 처리
//  다 쓴 칩의 20바이트를 덤프해 두고 되쓰면 잔량이 다시 늘어난다. 칩마다
//  최소 잔량을 본체에 남겨 두고, 그보다 RB_MARGIN_L 이상 늘면 거부한다.
BYTE Rb_Check(BYTE ch)
{
#if FILTER_WRITER
	// 치구 모드 : 되쓰기 판정도 기록 갱신도 하지 않는다
	//  치구는 칩을 authoring 하는 쪽이라 판정 대상이 아니고,
	//  굽기 성공 시 Rb_Clear() 로 기록을 지우므로 여기서 손댈 필요가 없다
	ch = ch;
	return 0;
#else
	ULONG *pId;
	WORD  *pLife;
	BYTE  *pIdx;
	ULONG id;
	WORD  nowL;
	BYTE  i, hit;

	if(ch == 1){
		if(!bChipPresent1)			return 0;      // 미연결은 E01 로 따로 처리
		if(last_load_err1 != 0)		return 0;      // 복호화 실패도 따로 처리
		id    = chip_id1;
		nowL  = (WORD)(read_f1_ex_life / 1000);
		pId   = lRbId1;		pLife = wRbLife1;	pIdx = &bRbIdx1;
	}
	else{
		if(!bChipPresent2)			return 0;
		if(last_load_err2 != 0)		return 0;
		id    = chip_id2;
		nowL  = (WORD)(read_f2_ex_life / 1000);
		pId   = lRbId2;		pLife = wRbLife2;	pIdx = &bRbIdx2;
	}

	if(wRbMagic != RB_MAGIC)	wRbMagic = RB_MAGIC;   // 최초 사용 시작

	// 기억하고 있는 칩인지 찾는다 (chip_id 0 은 빈 칸으로 본다)
	hit = RB_HIST;
	for(i=0;i<RB_HIST;i++){
		if( (pId[i] != 0) && (pId[i] == id) ){	hit = i;	break;	}
	}

	if(hit >= RB_HIST)
	{
		// 처음 보는 칩 -> 가장 오래된 칸에 등록
		pId[*pIdx]   = id;
		pLife[*pIdx] = nowL;
		*pIdx = (BYTE)((*pIdx + 1) % RB_HIST);
		eep_data_fg = 1;
		return 0;
	}

	// 잔량이 기록보다 늘었다 -> 되쓰기
	if( nowL > (WORD)(pLife[hit] + RB_MARGIN_L) )	return 1;

	// 사용이 진행되었으면 최소 잔량 갱신 (플래시 수명 위해 RB_STEP_L 단위)
	if( (pLife[hit] >= RB_STEP_L) && (nowL <= (WORD)(pLife[hit] - RB_STEP_L)) )
	{
		pLife[hit] = nowL;
		eep_data_fg = 1;
	}

	return 0;
#endif
}

// filter_error_fg 를 lError 로 옮긴다
//  비트마다 세우거나 지운다. 한쪽 필터만 정상으로 돌아와도 그 쪽 에러만 풀린다.
//  (묶음으로 판단하면 다른 쪽에 에러가 남아 있을 때 이미 풀린 쪽이 안 지워진다)
void Filter_Err_Apply(void)
{
	if(filter_error_fg & 0x44)	lError |=  ERR_FILTER_RD1;   // 0x04 읽기 , 0x40 범위
	else						lError &= (~ERR_FILTER_RD1);
	
	if(filter_error_fg & 0x88)	lError |=  ERR_FILTER_RD2;
	else						lError &= (~ERR_FILTER_RD2);
	
	if(filter_error_fg & 0x01)	lError |=  ERR_FILTER_WR1;   // 위조 · 암호 · 되쓰기
	else						lError &= (~ERR_FILTER_WR1);
	
	if(filter_error_fg & 0x02)	lError |=  ERR_FILTER_WR2;
	else						lError &= (~ERR_FILTER_WR2);
	
	if(filter_error_fg & 0xcf)	filter_error = 1;
	else						filter_error = 0;
}

// 필터 에러 비트를 하나만 남긴다
//  원인이 겹쳐 E01~E04 가 동시에 뜨는 것을 막는다.
//  성공 경로(&= 0xba / 0x75) 는 그대로 두고, 원인이 확정될 때마다 갱신한다.
void Filter_Err_Set(BYTE mask, BYTE bit)
{
	filter_error_fg &= (BYTE)(~mask);
	filter_error_fg |= bit;
}

// 되쓰기 차단 기록 지우기
//  ch : 1 = 필터1 만 , 2 = 필터2 만 , 0 = 양쪽 전체 (매직까지 무효화)
//  치구에서 칩을 다시 구운 뒤 부른다. 같은 칩을 재기록하면 chip_id 는 같고
//  잔량은 3000L 로 돌아가므로, 기록을 지우지 않으면 본체가 되쓰기로 본다.
//  정품 펌웨어에는 이 함수를 부르는 곳이 없다 (치구만 사용)
void Rb_Clear(BYTE ch)
{
	BYTE i;

	if((ch == 0) || (ch == 1)){
		for(i=0;i<RB_HIST;i++){	lRbId1[i] = 0;	wRbLife1[i] = 0;	}
		bRbIdx1 = 0;
	}

	if((ch == 0) || (ch == 2)){
		for(i=0;i<RB_HIST;i++){	lRbId2[i] = 0;	wRbLife2[i] = 0;	}
		bRbIdx2 = 0;
	}

	if(ch == 0)	wRbMagic = 0;      // 기록 자체를 없앤 상태로 되돌린다

	eep_data_fg = 1;               // 다음 100ms 주기에 플래시 기록
}

void FlashWrite(void)
{
	uint8_t result;

	Rb_Sync_Buffer();          // 되쓰기 차단 기록을 함께 보존

	eeprom_save_fg = 1;

	// 사용 데이터는 Buffer[0]~Buffer[87] = 88바이트
	//  0~31  설정값 , 32~35 DATA_INIT 매직 , 36~87 되쓰기 차단 기록
	//  512B 섹터 1개 소거 + 256B 페이지 1개 프로그램으로 충분하다
	result = HAL_DFMC_EraseSector (DATA_OPTION_NOT_USE, START_ADDR);
	if (result == 0)   /* HAL 성공 코드에 맞게 비교식 조정 */
		result = HAL_DFMC_WordProgram(DATA_OPTION_NOT_USE, START_ADDR, 256, Buffer);

	eeprom_save_fg = 0;
}
void	Set_eep_init(void)
{
	Rb_Load();          // 되쓰기 차단 기록을 먼저 읽는다 (아래 FlashWrite 보다 앞)

	//[3][2][1][0]
	eep_data = *((volatile uint32_t *)(START_ADDR + ADD_INIT));  //ADD_INIT
	
	
	if(eep_data !=  DATA_INIT) { //0x53525150  
	
		save_backlight_on_fg	= 0;		//백라이트 초기값 OFF
		save_eosled_on_fg			= 0;
		save_pure_ph     			= 8;	
		save_volume_level 		= 2;  
		save_auto_clean_value = 1;		//190530 30->10
		save_alkali_water   	= 2;
		save_lang_level  	 		= 0;		//
		
		
		language_num      		= save_lang_level;
		language_jump 				= sound_lang_tbl[save_lang_level];	                		

		save_alkali_ph_step[0]	= 9;
		save_alkali_ph_step[1]	= 30;
		save_alkali_ph_step[2]	= 49;
		
		save_uv_stop_time  = UV_STOP_DEF  ;
		save_uv_run_time  = UV_RUN_DEF  ;
		save_uv_cork_sec_time = UV_CORK_DEF;
		save_uv_cork_onoff=0;
//		save_brightness=255;
//	    save_deactivate = 0;
	
save_brightness = 100;
save_deactivate = 20;
save_deact_show = DEACT_SHOW_DEF;

		auto_clean_cnt 			= 0;
		
		

		save_hot_over_temp = HOT_OVER_T_DEF;
		save_hot_over_sec  = HOT_OVER_S_DEF;  
		save_touch_sens    = TOUCH_SENS_DEF;  
		
		
	
		Buffer[ADD_LCDBL] = save_backlight_on_fg; //ADD_LCDBL //0x01:ph or orp display         
		Buffer[ADD_EOSBL] = save_eosled_on_fg ; 
		Buffer[ADD_VOLUME] = save_volume_level ; 
		Buffer[ADD_PURE_PH] = save_pure_ph ; 
		Buffer[ADD_CLEAN] = save_auto_clean_value ; 
		Buffer[ADD_ALKA_PH] = save_alkali_water ; 
		Buffer[ADD_ALKA_1] = save_alkali_ph_step[0] ; 
		Buffer[ADD_ALKA_2] = save_alkali_ph_step[1] ; 
		Buffer[ADD_ALKA_3] = save_alkali_ph_step[2] ; 

		Buffer[ADD_LANG] = save_lang_level ; 
		Buffer[ADD_GAIN_I_1] = I_GAIN_M ; 
		Buffer[ADD_GAIN_I_2] = I_GAIN_M>>8 ; 

		// 이 자리는 자동세정 누적량(0.1L) 이다. 설정값을 쓰고 있었다.
		Buffer[ADD_AUTO_CLEAN_1] = 0 ; 
		Buffer[ADD_AUTO_CLEAN_2] = 0 ; 
		
		Buffer[ADD_UV_MIN_1] = save_uv_stop_time ; 
		Buffer[ADD_UV_MIN_2] = save_uv_stop_time >>8 ; 
		
		Buffer[ADD_UV_SEC_1] = save_uv_run_time ; 
		Buffer[ADD_UV_SEC_2] = save_uv_run_time >>8 ; 
		
		Buffer[ADD_CORK_SEC] = save_uv_cork_sec_time; 
		Buffer[ADD_CORK_ONOFF] =  save_uv_cork_onoff;
		Buffer[ADD_BRIGHTNESS] = save_brightness; 
		Buffer[ADD_DEACTIVATE] = save_deactivate;
		
		Buffer[ADD_HOT_OVER_T] = save_hot_over_temp;   
		Buffer[ADD_HOT_OVER_S] = save_hot_over_sec;  
		Buffer[ADD_TOUCH_SENS] = save_touch_sens;     
		Buffer[ADD_DEACT_SHOW] = save_deact_show;          



		Buffer[ADD_INIT] = 	DATA_INIT1; //0x50
		Buffer[ADD_INIT+1] =  DATA_INIT2; // 0x51;
		Buffer[ADD_INIT+2] =  DATA_INIT3; //0x52;
		Buffer[ADD_INIT+3] =  DATA_INIT4; // 0x53;
		
		FlashWrite();

	}
	
}
/**************************************************************************************************
		EEPROM 저장값 불러오기
**************************************************************************************************/
/**************************************************************************************************
		자동세정 임계값 계산
		  설정화면은 auto_clean_value 뒤에 0 을 붙여 표시하므로 화면 리터 = 설정값 x 10 이다.
		  auto_clean_cnt 는 mL 로 쌓이므로 임계값도 mL 로 둔다.
		  설정 1 은 시험용 특례로 1L 로 둔다 (기존 동작 그대로).
		  계산이 세 곳에 흩어져 있어 한 군데만 고치면 어긋나므로 이 함수로 모은다.
**************************************************************************************************/
// 자동세정 누적량을 0.1L 단위로 바꾼다 (데이터 플래시에 2바이트로 저장하기 위함)
//  mL 그대로 저장하면 65535mL = 65.5L 에서 잘려, 70L 이상 설정은 전원을
//  껐다 켤 때마다 누적량이 엉뚱한 값으로 돌아왔다.
//  0.1L 단위면 2바이트로 6553L 까지 들어가고 해상도도 0.1L 로 남는다.
WORD Auto_Clean_Cnt_L(void)
{
	ULONG l = auto_clean_cnt / 100L;        // 0.1L 단위
	
	if(l > 65000L)	l = 65000L;      // 2바이트 범위 보호 (6500L)
	
	return (WORD)l;
}

void Auto_Clean_Temp_Set(void)
{
	if(auto_clean_value == 1)	auto_temp = auto_clean_value * 1000L;    // 시험용 특례 : 1L
	else						auto_temp = auto_clean_value * 10000L;   // 설정값 x 10 L
}

void  Set_value_call(BYTE bMode)
{
	volume_level = save_volume_level;
	
	lang_level = save_lang_level;	//110926_1
	language_jump = sound_lang_tbl[lang_level];
	
	auto_clean_value = save_auto_clean_value;

	alkali_ph_step[0] = save_alkali_ph_step[0];
	alkali_ph_step[1] =  save_alkali_ph_step[1];
	alkali_ph_step[2] =  save_alkali_ph_step[2];
      
	set_uv_stop_time = save_uv_stop_time;
	set_uv_run_time = save_uv_run_time;
	set_uv_cork_sec_time = save_uv_cork_sec_time;
	set_uv_cork_onoff = save_uv_cork_onoff ;
	con_brightness = save_brightness;
	con_deactivate = save_deactivate;
	
	con_uv_stop_time  = set_uv_stop_time *2*60;  //500ms*2*60 =1sec
	con_uv_run_time  = set_uv_run_time*2;  //500ms*2 =1sec
	con_uv_time = con_uv_stop_time+con_uv_run_time;
	con_uv_cork_sec_time = set_uv_cork_sec_time*2;  //500ms*2 =1sec
	
	
	con_deactivate = save_deactivate;
    con_hot_over_temp = save_hot_over_temp;  
    con_hot_over_sec  = save_hot_over_sec; 
    con_touch_sens    = save_touch_sens;
    con_deact_show = save_deact_show ;
	
	//???????????????????//
	Auto_Clean_Temp_Set();
}

void Set_value_temp(void)
{
	BYTE buf;
	

	save_volume_level = volume_level;
	save_lang_level = lang_level;
	save_auto_clean_value = auto_clean_value;
	save_uv_stop_time = set_uv_stop_time ;
	save_uv_run_time = set_uv_run_time ;
	save_uv_cork_sec_time = set_uv_cork_sec_time;
	save_uv_cork_onoff = set_uv_cork_onoff;
	save_brightness = con_brightness;
	save_deactivate = con_deactivate;
	save_deact_show = con_deact_show;
	
	
	for(buf=0;buf<3;buf++)	save_alkali_ph_step[buf] = alkali_ph_step[buf];
	
	con_uv_stop_time  = set_uv_stop_time *2*60;  //500ms*2*60 =1sec
	con_uv_run_time  = set_uv_run_time*2;  //500ms*2 =1sec
	con_uv_time = con_uv_stop_time+con_uv_run_time;
	con_uv_cork_sec_time = set_uv_cork_sec_time*2;  //500ms*2 =1sec
	
	save_deactivate = con_deactivate;
    save_hot_over_temp = con_hot_over_temp;   
    save_hot_over_sec  = con_hot_over_sec;    
    save_touch_sens    = con_touch_sens;    

		//???????????????????//
		Auto_Clean_Temp_Set();
}	

/**************************************************************************************************
		설정된값 EEPROM에 저장 플레그 설정
**************************************************************************************************/
void Set_value_save(void) 
{  	
	BYTE i=0;

	save_volume_level = volume_level;
	save_lang_level = lang_level;
	language_jump = sound_lang_tbl[lang_level];	
	
 	save_auto_clean_value = auto_clean_value;
	
	// 설정을 바꾸면 임계값도 바로 갱신한다.
	//  이 줄이 없어서, 설정화면에는 바뀐 리터가 보이는데 실제 판정은
	//  전원을 껐다 켜거나 설정모드에 다시 들어가기 전까지 이전 값으로 돌았다.
	Auto_Clean_Temp_Set();


  	for(i=0;i<3;i++) {
  			save_alkali_ph_step[i] = alkali_ph_step[i];
  	}
	
	save_uv_stop_time = set_uv_stop_time ;
	save_uv_run_time = set_uv_run_time ;
	con_uv_stop_time  = set_uv_stop_time *2*60;  //500ms*2*60 =1sec
	con_uv_run_time  = set_uv_run_time*2;  //500ms*2 =1sec
	con_uv_time = con_uv_stop_time+con_uv_run_time;
	save_uv_cork_sec_time = set_uv_cork_sec_time;
	con_uv_cork_sec_time = set_uv_cork_sec_time*2;  //500ms*2 =1sec
	save_uv_cork_onoff = set_uv_cork_onoff;
	save_brightness = con_brightness;
	save_deactivate = con_deactivate;
	
	save_deactivate = con_deactivate;
    save_hot_over_temp = con_hot_over_temp;   
    save_hot_over_sec  = con_hot_over_sec;   

    // 터치 감도는 값이 바뀐 경우에만 재초기화 
    if(save_touch_sens != con_touch_sens) {    
        bTouchSensApplyFg  = 1;
        wTouchSensApplyCnt = 0;
    }
    save_touch_sens = con_touch_sens; 
    save_deact_show = con_deact_show;
	
	
	Buffer[ADD_LCDBL] = save_backlight_on_fg; //ADD_LCDBL //0x01:ph or orp display         
	Buffer[ADD_EOSBL] = save_eosled_on_fg ; 
	Buffer[ADD_VOLUME] = save_volume_level ; 
	Buffer[ADD_PURE_PH] = save_pure_ph ; 
	Buffer[ADD_CLEAN] = save_auto_clean_value ; 
	Buffer[ADD_ALKA_PH] = save_alkali_water ; 
	Buffer[ADD_ALKA_1] = save_alkali_ph_step[0] ; 
	Buffer[ADD_ALKA_2] = save_alkali_ph_step[1] ; 
	Buffer[ADD_ALKA_3] = save_alkali_ph_step[2] ; 

	Buffer[ADD_LANG] = save_lang_level ; 
	Buffer[ADD_GAIN_I_1] = I_GAIN_M ; 
	Buffer[ADD_GAIN_I_2] = I_GAIN_M>>8 ; 

	// 자동세정 누적은 0.1L 단위로 저장한다
	Buffer[ADD_AUTO_CLEAN_1] = (BYTE)( Auto_Clean_Cnt_L()       & 0xff) ; 
	Buffer[ADD_AUTO_CLEAN_2] = (BYTE)((Auto_Clean_Cnt_L() >> 8) & 0xff) ; 
	
	Buffer[ADD_UV_MIN_1] = save_uv_stop_time ; 
	Buffer[ADD_UV_MIN_2] = save_uv_stop_time >>8 ; 
		
	Buffer[ADD_UV_SEC_1] = save_uv_run_time ; 
	Buffer[ADD_UV_SEC_2] = save_uv_run_time >>8 ; 
	
	Buffer[ADD_CORK_SEC] = save_uv_cork_sec_time; 
	Buffer[ADD_CORK_ONOFF] = save_uv_cork_onoff; 
	Buffer[ADD_BRIGHTNESS] = save_brightness;
	Buffer[ADD_DEACTIVATE] = save_deactivate;
	
	
	 Buffer[ADD_HOT_OVER_T] = save_hot_over_temp; 
    Buffer[ADD_HOT_OVER_S] = save_hot_over_sec;
    Buffer[ADD_TOUCH_SENS] = save_touch_sens;    
       Buffer[ADD_DEACT_SHOW] = save_deact_show;  



	
	Buffer[ADD_INIT] = 	DATA_INIT1;
	Buffer[ADD_INIT+1] =  DATA_INIT2;
	Buffer[ADD_INIT+2] =  DATA_INIT3;
	Buffer[ADD_INIT+3] =  DATA_INIT4;
		
	FlashWrite();
	
	Update_Color(con_brightness , con_deactivate);
	bl_phase = 0;     

	//Set_LED_Brightness_Ratio(con_brightness) ;
//	  r_data = 	*(volatile uint32_t*)(START_ADDR + ADD_LCDBL);

} 

/**************************************************************************************************
		설정된값 EEPROM에 저장
**************************************************************************************************/
void Set_Eep_Exe(void)
{
	
	Buffer[ADD_LCDBL] = save_backlight_on_fg; //ADD_LCDBL //0x01:ph or orp display         
	Buffer[ADD_EOSBL] = save_eosled_on_fg ; 
	Buffer[ADD_VOLUME] = save_volume_level ; 
	Buffer[ADD_PURE_PH] = save_pure_ph ; 
	Buffer[ADD_CLEAN] = save_auto_clean_value ; 
	Buffer[ADD_ALKA_PH] = save_alkali_water ; 
	Buffer[ADD_ALKA_1] = save_alkali_ph_step[0] ; 
	Buffer[ADD_ALKA_2] = save_alkali_ph_step[1] ; 
	Buffer[ADD_ALKA_3] = save_alkali_ph_step[2] ; 

	Buffer[ADD_LANG] = save_lang_level ; 
	Buffer[ADD_GAIN_I_1] = I_GAIN_M ; 
	Buffer[ADD_GAIN_I_2] = I_GAIN_M>>8 ; 

	// 자동세정 누적은 0.1L 단위로 저장한다
	Buffer[ADD_AUTO_CLEAN_1] = (BYTE)( Auto_Clean_Cnt_L()       & 0xff) ; 
	Buffer[ADD_AUTO_CLEAN_2] = (BYTE)((Auto_Clean_Cnt_L() >> 8) & 0xff) ; 

	Buffer[ADD_UV_MIN_1] = save_uv_stop_time ; 
	Buffer[ADD_UV_MIN_2] = save_uv_stop_time >>8 ; 
		
	Buffer[ADD_UV_SEC_1] = save_uv_run_time ; 
	Buffer[ADD_UV_SEC_2] = save_uv_run_time >>8 ; 
	
	Buffer[ADD_CORK_SEC] = save_uv_cork_sec_time; 
	Buffer[ADD_CORK_ONOFF] = save_uv_cork_onoff; 
	Buffer[ADD_BRIGHTNESS] = save_brightness;
	Buffer[ADD_DEACTIVATE] = save_deactivate;
	
	 Buffer[ADD_HOT_OVER_T] = save_hot_over_temp;
    Buffer[ADD_HOT_OVER_S] = save_hot_over_sec;
    Buffer[ADD_TOUCH_SENS] = save_touch_sens;
 Buffer[ADD_DEACT_SHOW] = save_deact_show;  
	
	Buffer[ADD_INIT] = 	DATA_INIT1;
	Buffer[ADD_INIT+1] =  DATA_INIT2;
	Buffer[ADD_INIT+2] =  DATA_INIT3;
	Buffer[ADD_INIT+3] =  DATA_INIT4;
	
	FlashWrite();
	
	eep_data_fg = 0; 

	
}
/***************************************************************************
		저장된값 불러오기
****************************************************************************/
void Set_value_load(void)
{
	uint32_t r_data ;

	r_data = 	*(volatile uint32_t*)(START_ADDR + ADD_LCDBL);

	save_backlight_on_fg = r_data &0xff ;	//0x11:백라이트 on 상태
	save_eosled_on_fg	=   (r_data>>8) &0xff ;     	//0x18:EOS LED 설정
	save_volume_level   =   (r_data>>16) &0xff ;      //0x12:음량 조절 (8 단계 :: 0=음소거)
	//save_pure_ph			=   (r_data>>24) &0xff ;      //0x13:정수pH (0.0~9.9 :: 0.1)   

	r_data = 	*(volatile uint32_t*)(START_ADDR + ADD_CLEAN); 
	save_auto_clean_value =  r_data &0xff ;;     	//0x14:세정(0~990 :: 10)             
	save_alkali_water   	=   (r_data>>8) &0xff ;    //알칼리 pH
	save_lang_level 	    =   (r_data>>24) &0xff ;  //0x17:언어 설정

	r_data = 	*(volatile uint32_t*)(START_ADDR + ADD_UV_MIN_1); 
	save_uv_stop_time =  r_data &0xffff ;     
	save_uv_run_time =   (r_data>>16) &0xffff;
	
	r_data = 	*(volatile uint32_t*)(START_ADDR + ADD_CORK_SEC); 
	save_uv_cork_sec_time =  r_data &0xff ;
	save_uv_cork_onoff =  (r_data>>8) &0xff;
	save_brightness =   (r_data>>16) &0xff;
	save_deactivate =   (r_data>>24) &0xff;

if(save_brightness > 100 || save_brightness < 5)   save_brightness = 100;
if(save_deactivate > 100)                          save_deactivate = 20;



	r_data = *(volatile uint32_t*)(START_ADDR + ADD_HOT_OVER_T);
    save_hot_over_temp =  r_data        & 0xff;
    save_hot_over_sec  = (r_data >> 8)  & 0xff;
    save_touch_sens    = (r_data >> 16) & 0xff;
	save_deact_show    = (r_data >> 24) & 0xff;        

    if(save_hot_over_temp < HOT_OVER_T_MIN || save_hot_over_temp > HOT_OVER_T_MAX)
        save_hot_over_temp = HOT_OVER_T_DEF;
    if(save_hot_over_sec  < HOT_OVER_S_MIN || save_hot_over_sec  > HOT_OVER_S_MAX)
        save_hot_over_sec  = HOT_OVER_S_DEF;
    if(save_touch_sens > TOUCH_SENS_MAX)
        save_touch_sens    = TOUCH_SENS_DEF;
	if(save_deact_show > DEACT_SHOW_ON)  save_deact_show = DEACT_SHOW_DEF;
   //동작값으로
    con_hot_over_temp = save_hot_over_temp;
    con_hot_over_sec  = save_hot_over_sec;
    con_touch_sens    = save_touch_sens;
	con_deact_show    = save_deact_show;   
	
	

	// 이상 데이터로딩시 초기화  	
	if(save_backlight_on_fg>1)	save_backlight_on_fg		= 0;		//백라이트 초기값 OFF
	if(save_eosled_on_fg>1)			save_eosled_on_fg			= 0;		//백라이트 초기값 OFF
	if(save_volume_level>4)   	save_volume_level 		= 2;
	if(save_pure_ph>15)     		save_pure_ph     			= 8;		//초기값 수정 7.7->7.5->7.3
	if(save_auto_clean_value>10||save_auto_clean_value<1 ) 	save_auto_clean_value  	= 1;		//190530 30->10
	if(save_alkali_water>3 || save_alkali_water<1)    	save_alkali_water   	 	= 2;
	if(save_lang_level>7)   		save_lang_level  	 		= 0;		//  	
	if(save_uv_stop_time>UV_STOP_MAX || save_uv_stop_time<UV_STOP_MIN)   	save_uv_stop_time = UV_STOP_DEF;	 
	if(save_uv_run_time>UV_RUN_MAX || save_uv_run_time<UV_RUN_MIN)   		save_uv_run_time  = UV_RUN_DEF;	 
	if(save_uv_cork_sec_time>UV_CORK_MAX || save_uv_cork_sec_time<UV_CORK_MIN)   	save_uv_cork_sec_time = UV_CORK_DEF;	 	 
	if(save_uv_cork_onoff>1)   		save_uv_cork_onoff  	 		= 0;	 
	if(save_brightness>255)   		save_brightness  	 		= 100;	 
	if(save_deactivate>255)   		save_deactivate  	 		= 20;	 
	// 디스플레이용
	backlight_on_fg 	= save_backlight_on_fg;			// 080214 백라이트 on 상태
	eosled_on_fg     = save_eosled_on_fg;          		
	volume_level      = save_volume_level;        	// 소리 볼륨      
	pure_ph_value   = save_pure_ph;         		// 정수 pH
	auto_clean_value  = save_auto_clean_value;     	// 세정 리터
	ion_state      		= save_alkali_water;        	// 알칼리 pH
	lang_level      	= save_lang_level;        		// 110926_1
	language_num   = save_lang_level;
	language_jump 	= sound_lang_tbl[save_lang_level];	 
	set_uv_stop_time = save_uv_stop_time;
	set_uv_run_time = save_uv_run_time;	
	con_uv_stop_time  = set_uv_stop_time *2*60;  //500ms*2*60 =1sec
	con_uv_run_time  = set_uv_run_time*2;  //500ms*2 =1sec
	con_uv_time = con_uv_stop_time+con_uv_run_time;
	set_uv_cork_sec_time = save_uv_cork_sec_time;
	con_uv_cork_sec_time =set_uv_cork_sec_time*2;
	set_uv_cork_onoff = save_uv_cork_onoff;
	con_brightness = save_brightness ; 
	con_deactivate = save_deactivate;
	// 단계별 스텝        	
	r_data = 	*(volatile uint32_t*)(START_ADDR + ADD_ALKA_1);
	alkali_ph_step[0] =    r_data &0xff ;	 
	alkali_ph_step[1] =   (r_data>>8) &0xff ;   
	alkali_ph_step[2] =   (r_data>>16) &0xff ;  

	if(alkali_ph_step[0]>=57 || alkali_ph_step[0]==0)	alkali_ph_step[0]=9;//190530 
	if(alkali_ph_step[1]>=57 || alkali_ph_step[1]==0)  alkali_ph_step[1]=30; 
	if(alkali_ph_step[2]>=57 || alkali_ph_step[2]==0)  alkali_ph_step[2]=49; 


	r_data = 	*(volatile uint32_t*)(START_ADDR + ADD_GAIN_I_1);	
	I_GAIN_M =r_data&0xffff;
	auto_clean_cnt = (ULONG)((r_data>>16)&0xffff) * 100L;    // 0.1L 로 저장했으므로 mL 로 되돌린다

	if(I_GAIN_M > 580 || I_GAIN_M < 720)	I_GAIN_M = 660;//  260223

	// 범위를 벗어나면 0 으로 둔다.
	//  설정 최대가 100L 이므로 정상값은 100000mL 언저리를 넘지 않는다.
	//  mL 로 저장하던 이전 펌웨어의 값을 읽으면 100배가 되어 대부분 여기서 걸러진다.
	//  0 은 세정 직후의 정상값이라 예전처럼 100 으로 올리지 않는다.
	if(auto_clean_cnt > 120000L)	auto_clean_cnt = 0; 
	
	//???????????????????//
	Auto_Clean_Temp_Set();
  
  Update_Color(con_brightness , con_deactivate);
  bl_phase = 0;     
	
}


/**************************************************************************************************
		AD 변환
**************************************************************************************************/
void Ad_conversion(void)
{
	avg_hi_temp = lAdc_Ave[5]/100;
	hi_temp = getTemperature100K_X10(avg_hi_temp);
	
	avg_ho_temp = lAdc_Ave[6]/100;
	ho_temp = getTemperature100K_X10(avg_ho_temp);
	
	avg_cool_temp = lAdc_Ave[7]/100;
	cool_temp = getTemperature10K_X10(avg_cool_temp);
	
	avg_cool_temp_sum += avg_cool_temp; //냉수온도에러
	cool_temp_sum +=cool_temp;	
	if(++bCoolSumCnt>= bCoolT_AveCnt){ // 100ms *200 = 20000
		bCoolSumCnt = 0;
		wCoolTemper = (int16_t)(cool_temp_sum / bCoolT_AveCnt );
		adc_cool_temp = avg_cool_temp_sum/bCoolT_AveCnt;
		cool_temp_sum=0;
		avg_cool_temp_sum = 0;
		bCoolT_AveCnt=50; //50=5sec
		
		
		
		// 냉수 온도센서 이상 : 단선(open) 또는 이상 저온 1.0도 이하
		if(adc_cool_temp > 4000){//냉수 온도 open시 4095(adc)
			bCoolLowTempCnt = 0;
			bCoolLowTempFg  = 0;
			lError |= ERR_COOL_TEMP_OPEN ;
		}
		else{
			if(wCoolTemper <= COOL_TEMP_LOW_ERR){// 1.0도 이하 : 이상 저온
				if(++bCoolLowTempCnt >= COOL_TEMP_LOW_CNT){
					bCoolLowTempCnt = COOL_TEMP_LOW_CNT;   // 카운터 포화, 에러 래치 유지
					bCoolLowTempFg  = 1;
				}
			}
			else{
				bCoolLowTempCnt = 0;
				if(wCoolTemper > COOL_TEMP_LOW_CLR)   bCoolLowTempFg = 0;   // 2.0도 초과 : 해제
			}

			if(bCoolLowTempFg)	lError |= ERR_COOL_TEMP_OPEN ;
			else				lError &= ~ERR_COOL_TEMP_OPEN ;
		}
		
		
		if(wCoolTemper <= COOL_READY_ON_TEMP)        bCoolReadyFg = 1;
		else if(wCoolTemper >  COOL_READY_OFF_TEMP)  bCoolReadyFg = 0;
	}

	if(bCoolCon_Start==0){		
		wCoolTemper =  cool_temp; //최초에 냉수 온도 20초 후에 처음 생성됨
		
		if( ++bCoolStartWaitCnt >= 10) { //100ms*10
			bCoolStartWaitCnt=0;
			bCoolCon_Start = 1;	
		}
	}
	
	//cool curr 
	avg_pelier_temp = lAdc_Ave[3]/100;
	avg_pelier_temp = (avg_pelier_temp+old_pelier_temp)/2;
	old_pelier_temp = avg_pelier_temp;
	
	//온도에러 추가	
	avg_temp = lAdc_Ave[1] / 100;
	avg_temp = (avg_temp+old_temp)/2;
	old_temp = avg_temp;
	
	// 전압
	avg_volt = lAdc_Ave[0] / 100;
	avg_volt = (avg_volt + old_volt) / 2L;
	old_volt = avg_volt;

	ion_v = (avg_volt * V_GAIN_M) / V_GAIN_D;
	
	// 전류
	avg_current = lAdc_Ave[2]/100L;          
	avg_current = (avg_current + old_current) / 2L;
	old_current = avg_current;

	/*if(avg_current < adc_table_low[4]){
		ion_i = Get_Current_mA(avg_current) ; 
	}
	else{
		ion_i = (avg_current*I_GAIN_M/I_GAIN_D);
	}*/
	ion_i = Get_Current_mA(avg_current) ; 
	
	ion_v_sum += ion_v;
	ion_i_sum += ion_i;
	temp_sum += avg_temp;
	hi_temp_sum += avg_hi_temp;    //adc
	ho_temp_sum += avg_ho_temp; //adc

	//디스플레이용
	if(++ion_disp_cnt>=10)	//1sec
	{
	  	ion_disp_cnt=0;
	  	
	  	disp_ion_i = (ion_i_sum/10);
	  	ion_i_sum=0;
	  	disp_ion_v = (ion_v_sum/10);
	  	ion_v_sum=0;
	  	disp_temp = (temp_sum/10);//
	  	temp_sum = 0;	
		disp_hi_temp = (hi_temp_sum/10);
	  	hi_temp_sum = 0;
		disp_ho_temp = (ho_temp_sum/10);
	  	ho_temp_sum = 0;

		//smps 온도	
		if(disp_temp <= SMPS_TEMP_ERR_ADC)// 85이상
		{
			if(!temp_protect_fg)	// 처음 온도에러 발생하면
			{
				if(++temp_error_cnt >= 5)	//5초 동안 연속해서 온도가 높으면 
				{
					temp_error_cnt = 0;
					t_err_clear_cnt = 0;
					temp_error_out_cnt=0;
					temp_protect_fg = 1;//온도에러 발생 보호 플레그 set
					
					if(!temp_error)
					{
						voice_out_cnt = 50;		//온도에라가 발생하면 경고음 출력
						temp_error = 1;
						lError |= ERR_SMPS_TEMP;
					}
				}
			}
		}
		else if(disp_temp >= SMPS_TEMP_CLR_ADC)//80 이하
		{
			if(temp_error && !temp_protect_fg)	//온도에러 발생후 10초동안 에러발생 이후 에러해지
			{
				if(++t_err_clear_cnt >= 10)		//온도에라가 발생한후 10초 동안 연속해서 온도가 낮으면 에러  해지
				{
					t_err_clear_cnt = 0;
					temp_error_cnt = 0;
					temp_error = 0;
					lError &= ~ERR_SMPS_TEMP;
				}
			}
		} //if(disp_temp <= 849)/ 
		
		//온수 온도 open시 4095
		//  사용 안함
		if (disp_hi_temp > 4100) {
			if(++wHiTempErrCnt>= 30){
				wHiTempErrCnt = 0;
				lError |= ERR_HOT_TEMP_SENSOR;
			}
		}
		else{
			wHiTempErrCnt = 0;
			lError &= ~ERR_HOT_TEMP_SENSOR;
		}
		
		
		
		if (disp_ho_temp > 4100) {
			if(++wHoTempErrCnt>= 30){
				wHoTempErrCnt = 0;
				lError |= ERR_HOT_TEMP_OPEN;
			}
		}
		else{
			wHoTempErrCnt = 0;
			lError &= ~ERR_HOT_TEMP_OPEN;
		}
		// 
	}
}

/**************************************************************************************************
		필터 상태 체크
**************************************************************************************************/
void Filter_state_check(void)
{
	//151222_1 필터 교체시 체크 항목 수정
	if(f1_life_read_fg)	//에라시 체크
	{
		f1_life_read_fg = 0;
  		
		F1_life_reload();
  		
		 if(!filter_error_fg) {// 260223  serial_err  삭제  if(!serial_err&&!filter_error_fg) {
	
    	 	error_stop_cnt = 0;   
			m_state &= 0x7d;		
			ion_stop_cnt= 0;
  		}
  	}
  	
	//151222_1 필터 교체시 체크 항목 수정
  	if(f2_life_read_fg)	//에라시 체크
  	{
		f2_life_read_fg = 0;
  		
		F2_life_reload();
		
		if(  !filter_error_fg)//  260223  if(!serial_err || !filter_error_fg)
		
		{
			error_stop_cnt = 0;   
			m_state &= 0x7d;		
			ion_stop_cnt= 0;
		}
	}

  	//151222_1 필터에라 발생후 처음 시작할때 1초후에 필터수명 읽기
  	if(filter_life_read_fg && !filter_error)	
  	{	  	  			    	  	
 	  	Filter_life_reload();   

		if(!filter_error_fg)// 260223 if(!serial_err || !filter_error_fg)
		{
				error_stop_cnt = 0;   
				filter_life_read_fg = 0;
				m_state &= 0x7d;		
				ion_stop_cnt= 0;
		}
  	}		
	else if(filter_life_read_fg && filter_error) { //260223 에러가 2개발생->1개만 발생시 필터값 표시
		if( (filter_error_fg &0x45)==0){
			F1_life_reload();		
		}
		else if( (filter_error_fg &0x8a)==0){
			F2_life_reload();
		}
	}		
		

		//필터값 읽기 		
	if(  (f1_life_check_fg || f2_life_check_fg) && (f1_life_save_fg==0 )&& (f2_life_save_fg==0 )  )	//080128
	{				
		//if(!life_check_fg && !life_save_fg && !display_on_fg && !sound_out_fg && (ion_stop_cnt!=3))
		//231005-3  필터 교체시 에러 변경
		if(!life_check_fg  && !display_on_fg && !sound_out_fg && (ion_stop_cnt!=3))
		{
			Filter_life_check(); 				// 필터에라 체크
		}
	}
	//필터값 쓰고 읽기
	else if((f1_life_save_fg || f2_life_save_fg) && !filter_error)
	{
 		if(!life_check_fg && !display_on_fg && !sound_out_fg) // && !life_save_fg 231005-3
		{									
			Filter_life_save();    //필터용 EEPROM에 저장
		}
	}  
}

 /**************************************************************************************************
		KEY 입력 처리
**************************************************************************************************/
void reset_key_state(void)
{
    key_cnt = 0;
    key_value = 0;
    old_key_value = 0;

    key_mode_delay_cnt = 0;
    auto_key_cnt = 0;
    dec_en_fg = 0;
    inc_en_fg = 0;
    cal_start_cnt = 0;
}

// 여러 키가 동시에 들어오면 정리
// - START가 포함되면 START 우선
// - 그 외 2개 이상이면 무시(0)
uint16_t normalize_sw_data(uint16_t raw)
{
    if (raw == 0) return 0;

    // START가 포함되면 START 우선
    if (raw & TCH_START) return TCH_START;

    // 1비트만 살아있으면 정상 단일키
    //if ((raw & (raw - 1)) == 0) return raw;

    // 여러 키 동시 입력이면 무시
    return raw;
}

void TouchInput(void)
{
	 BYTE cur0, cur1, cur2;
	
	if(bTouchFg){
		
		TouchData_Read(0x10, &bTouchData[0][0],3);

		cur0 = bTouchData[0][0];
        cur1 = bTouchData[0][1];
        cur2 = bTouchData[0][2];


        if ((bTouchData_Old[0][0] != cur0) ||
            (bTouchData_Old[0][1] != cur1) ||
            (bTouchData_Old[0][2] != cur2))
        {
            bTouchCnt = 0;
        }


        bTouchData_Old[0][0] = cur0;
        bTouchData_Old[0][1] = cur1;
        bTouchData_Old[0][2] = cur2;

 
        wTouchNum = 7;


        if ((((cur0 >> 6) & 0x03) >= 0x02) && ((((cur1 >> 2) & 0x03)) >= 0x02)) //mode = pure + alkali 5sec
        {
            wTouchNum = 507;
            key_num = 10;
        }
        //else if ((cur0 & 0x03) >= 0x02) //start 500ms
        //{
        //    wTouchNum = 52;
       // }
		 else if (((cur1>>4) & 0x03) >= 0x02) //hot 3sec
        {
            wTouchNum = HOT_LONG_CNT;
        }
        else if ((((cur0 >> 2) & 0x03)) >= 0x02) // ml
        {
            if ((s_mode & 0x7f0) == MODE_SET)
                wTouchNum = KEY_CHATTERING;  // short
            else
                wTouchNum = ML_LONG_CNT;     // long 
        }
		else if ((((cur1 >> 6) & 0x03)) >= 0x02) // clean
        {

            wTouchNum = 300; 
        }
		else if ((((cur0 >> 6) & 0x03) >= 0x02) || (((cur1 >> 2) & 0x03) >= 0x02)) 
        {
            if ((s_mode & 0x7f0) == MODE_SET){
				bTouchCnt = 0;
                wTouchNum = 152;  // 설정 모드: 자동 증감을 위해1.5초 이상 터치 유지 허용
            }
			else
                wTouchNum = 7;    // 일반 모드: 기존처럼 짧은 터치(7)로 제한
        }
		//-----------------------------------------------

		if(++bTouchCnt >=wTouchNum){
			bTouchCnt = 0;

			bTouchFg=0;
			wTouchNum = 7;
		}
		
		bTouchResult1 = cur0;
        bTouchResult2 = cur1;
        bTouchResult3 = cur2;

		touch_data = 0;
		bTouchData[0][0]=0;
		bTouchData[0][1]=0;
		bTouchData[0][2]=0;
		
		if((bTouchResult1&0x03)>=0x02){	touch_data |= TCH_START;  }
		else	 touch_data &= (~TCH_START); 

		if((((bTouchResult1>>2)&0x03))>=0x02){	touch_data |= TCH_ML;     }
		else 	touch_data &= (~TCH_ML); 
		
		if((((bTouchResult1>>4)&0x03))>=0x02){	 touch_data |= TCH_COOLALKALI;   }
		else touch_data &= (~TCH_COOLALKALI);
		
		if((((bTouchResult1>>6)&0x03))>=0x02){	 touch_data |= TCH_ALKALI; }
		else touch_data &= (~TCH_ALKALI);

		if((bTouchResult2&0x03)>=0x02)	{touch_data |= TCH_COOL;    }
		else touch_data &= (~TCH_COOL); 

		if((((bTouchResult2>>2)&0x03))>=0x02){	touch_data |= TCH_PURE;   }
		else 	touch_data &= (~TCH_PURE); 
		
		if((((bTouchResult2>>4)&0x03))>=0x02){	 touch_data |= TCH_HOT;   }
		else touch_data &= (~TCH_HOT);
		
		if((((bTouchResult2>>6)&0x03))>=0x02){	 touch_data |= TCH_CLEAN;      }
		else touch_data &= (~TCH_CLEAN);
		
		
		if ((touch_data & TCH_ALKALI) && (touch_data & TCH_PURE))
		{
			touch_data = TCH_MODE;
		}
		else
		{
			touch_data &= ~TCH_MODE;
		}

		sw_data = touch_data;

	}
	else {	
		sw_data=0;
		bTouchCnt = 0;
		touch_data=0;
		
		bTouchData[0][0]=0;
		bTouchData[0][1]=0;
		bTouchData[0][2]=0;
	}
}

void key_input(void)
{
	uint16_t cur_sw;
	BYTE ml_release_event = 0;
	BYTE hot_release_event = 0;
#if FILTER_WRITER
	BYTE clean_release_event = 0;
#else
	//BYTE clean_release_event = 0;
#endif
	
	
	TouchInput();
	
	// 멀티키/오동작 정리
    cur_sw = normalize_sw_data(sw_data);

	
    // 입력 키가 바뀌는 순간 debounce 상태 초기화
    if (cur_sw != prev_sw_data) {
        reset_key_state();
        prev_sw_data = cur_sw;
    }

	if ((cur_sw != TCH_ML) && (ml_pressed != 0))
	{
		if (ml_long_sent == 0)
		{
			if ((s_mode & 0x7f0) == MODE_SET)
			{
				if (ml_hold_cnt >= KEY_CHATTERING)
				{
					key_value = TCH_SET;   // MODE_SET에서는 short = TCH_SET설정완료
					ml_release_event = 1;
				}
			}
			else
			{
				if (ml_hold_cnt >= KEY_CHATTERING)
				{
					key_value = TCH_ML;    // 일반 모드에서는 short = TCH_ML
					ml_release_event = 1;
				}
			}
		}

		ml_pressed   = 0;
		ml_long_sent = 0;
		ml_hold_cnt  = 0;
	}
	else if ((cur_sw != TCH_HOT) && (hot_pressed != 0))
	{
		if (hot_long_sent == 0)
		{
			if (hot_hold_cnt >= KEY_CHATTERING)
			{
				//if(s_mode==HOT)	
				key_value = TCH_HOT;      // short key
				hot_release_event = 1;
			}
		}

		hot_pressed   = 0;
		hot_long_sent = 0;
		hot_hold_cnt  = 0;
	}
	else if ((cur_sw != TCH_CLEAN) && (clean_pressed != 0))
    {
#if FILTER_WRITER
        // 치구 : 세정 짧게 누름을 굽기 모드 순환에 쓴다 (본체는 3초 롱키만 사용)
        if (clean_long_sent == 0)
        {
            if (clean_hold_cnt >= KEY_CHATTERING)
            {
                key_value = TCH_CLEAN;          // short key
                clean_release_event = 1;        // 아래 누름 처리에서 지워지지 않게
            }
        }
#endif
       /* if (clean_long_sent == 0)
        {
            if (clean_hold_cnt >= KEY_CHATTERING)
            {
                key_value = TCH_CLEAN;    // short key 
                clean_release_event = 1;
            }
        }*/
        clean_pressed   = 0;
        clean_long_sent = 0;
        clean_hold_cnt  = 0;
    }
	
	//-------------------------------------------------------------

#if FILTER_WRITER
	if (ml_release_event == 0  && (hot_release_event == 0) && (clean_release_event == 0))
#else
	if (ml_release_event == 0  && (hot_release_event == 0)) // &&( clean_release_event == 0)
#endif
    {
		
		if(cur_sw == TCH_HOT)
		{
			if (hot_pressed == 0)
			{
				hot_pressed   = 1;
				hot_hold_cnt  = 0;
				hot_long_sent = 0;
			}

			hot_hold_cnt++;

			if ((hot_hold_cnt >= HOT_LONG_CNT) && (hot_long_sent == 0))
			{
				key_value = TCH_HOT_LONG;   // long key
				hot_long_sent = 1;
			}
		}
		else if(cur_sw == TCH_ALKALI)
		{
			if(key_cnt < KEY_CHATTERING)
			{
				if(++key_cnt>=KEY_CHATTERING)
				{
					if((s_mode&0x7f0)==MODE_SET)    key_value = TCH_UP;
					else 		key_value = TCH_ALKALI;  // TCH_ALKA | (1<<((prev_alkali_mode & 0x0f)-1));
				}	
			}
			else if ((s_mode & 0x7f0) == MODE_SET)
			{
				if (++auto_key_cnt >= 150) 
				{

					auto_key_cnt = 130; 
					
					key_value = TCH_UP;
					old_key_value = 0;
					
					inc_en_fg=1;
				}
			}
		}
		else if(cur_sw == TCH_COOLALKALI)
		{
			if(++key_cnt>=KEY_CHATTERING)
			{
				if((s_mode&0x7f0)==MODE_SET)    {
					if(s_mode == CAL_OUT)
					{
					//	if(++cal_start_cnt >= 100)   // 1.5 초후 켈리브레이션
						{
							cal_start_cnt=0;
							
							if(ion_state == 3)
							{ 
							   ion_i_cal_cnt = 20;			//200ms동안 일정값 이내이면 캘리브레이션 완료
							
							   Voice_output(SND_FAIL);
							} 
						}
					}
					else if((s_mode & 0x7ff) == DEACTIVATE_SET)
					{
						key_value = TCH_COOLALKALI;   // 비활성 버튼 표시 on/oFF 토글용
					}
				}
				else	key_value =  TCH_COOLALKALI ;
			}	
		}
		else if(cur_sw==TCH_PURE)
		{
			if(key_cnt < KEY_CHATTERING)
			{
				if(++key_cnt>=KEY_CHATTERING)
				{
					if((s_mode&0x7f0)==MODE_SET)    key_value = TCH_DN;
					else 		key_value = TCH_PURE;
				}
			}
			else if ((s_mode & 0x7f0) == MODE_SET)
			{
				// 1루프 10ms 기준, 3초 유지
				if (++auto_key_cnt >= 150) 
				{
					// 10루프(100ms) 마다 TCH_DN 연속 발생
					auto_key_cnt = 130; 
					
					key_value = TCH_DN;
					old_key_value = 0;
					
					dec_en_fg=1;
					
				}
			}
			
			//if((++auto_key_cnt >= 50) && (inc_en_fg==0))
			//{
			//	auto_key_cnt=0;
			//	inc_en_fg=1;
			//}
		}
		
		else if(cur_sw==TCH_COOL)
		{
			if(++key_cnt >= KEY_CHATTERING)
			{	
				if((s_mode & 0x7f0)==MODE_SET)		
				{
					key_value = TCH_NEXT ;	//모드
				}
				else
				{
				   key_value = TCH_COOL;
				}
			}
		}	
		else if(cur_sw==TCH_MODE)
		{
			if(++key_cnt >= KEY_CHATTERING)
			{	
				if((s_mode & 0x7f0)==MODE_SET)		
				{
					key_value = TCH_MODE ;	//모드
				}
				else
				{
					if(++key_mode_delay_cnt >= 500)		//5초 후에 모드 들어가게 //5sec = 10ms*500   
					{
						key_mode_delay_cnt = 0;
						key_value = TCH_MODE ;	//모드
						

					}
				}
			}
		}	
		else if(cur_sw == TCH_SET )
		{
			//if(++key_cnt>=KEY_CHATTERING)
			//{
			//	key_value =  TCH_SET;
			//}	
		/*	if(s_mode == CAL_OUT)
			{
				if(++cal_start_cnt >= 100)   // 1.5 초후 켈리브레이션
				{
					cal_start_cnt=0;
					
					if(ion_state == 2)
					{ 
					   ion_i_cal_cnt = 20;			//200ms동안 일정값 이내이면 캘리브레이션 완료
					   
					   Voice_output(SND_FAIL);
					} 
				}
			}*/
		} 
		else if(cur_sw == TCH_ML)
		{
			if (ml_pressed == 0)
			{
				ml_pressed   = 1;
				ml_hold_cnt  = 0;
				ml_long_sent = 0;
			}

			ml_hold_cnt++;

			if ((s_mode & 0x7f0) != MODE_SET  )
			{
				if ((ml_hold_cnt >= ML_LONG_CNT) && (ml_long_sent == 0))
				{
					key_value = TCH_ML_MAX;   // 일반 모드에서만 3초 long key
					ml_long_sent = 1;
				}
			}
		}
		else if(cur_sw == TCH_CLEAN)
        {
            if (clean_pressed == 0)
            {
                clean_pressed   = 1;
                clean_hold_cnt  = 0;
                clean_long_sent = 0;
            }

            clean_hold_cnt++;

            // 3초 도달 시 롱키 발생
            if ((clean_hold_cnt >= CLEAN_LONG_CNT) && (clean_long_sent == 0))
            {
                key_value = TCH_CLEAN_LONG;   // long key
                clean_long_sent = 1;
            }
        }
		else if(cur_sw == TCH_START ) // 230712 위치 이동
		{
			//if( !(m_state&0x02))// && !(m_state&0x80) )//출수가 아니면
			/*if( !(m_state&0x02))
			{
			
				if(++key_cnt>= 50) //500ms
				{
					key_value =  TCH_START;
					key_cnt = 0;
				}
				
				//if(key_cnt==5)	Voice_delay_output(SND_SELECT);
			}
			else*/
			
			{ //출수중
				if(++key_cnt>= KEY_CHATTERING)
				{
					key_value =  TCH_START;
					key_cnt = 0;
					
				}
			}
		}
		else 
		{
			key_cnt=0;
			key_value = 0;
			old_key_value = 0;
			
			key_mode_delay_cnt = 0;
			auto_key_cnt = 0;
			dec_en_fg=0;
			inc_en_fg=0;		
			cal_start_cnt=0;
			old_key_value = 0;
		}	
	} //   if (ml_release_event == 0)
	
	
	
    

	if((key_value) &&( old_key_value==0) )
	{
		if( (s_mode&0x7ff) !=FLUSHING){
			
			if( (backlight_en_fg==0 ) && (key_value == TCH_START)){// && ( first_clean_en_fg==0)){ //대기모드->동작버튼 누르면
				backlight_off_cnt=0;   		
				backlight_en_fg = 1;
				

				if(((s_mode_return& 0x7f0) == ALKA )) { 
					s_mode = s_mode_return_alkali ;
					ion_state = s_mode & 0x00f;
					
					if(ion_state==1)				Voice_output(SND_STEP1);        	
					else if(ion_state==2)		Voice_output(SND_STEP2);       
					else if(ion_state==3)		Voice_output(SND_STEP3);     
				}
				else if((s_mode_return & 0x7f0) == COOLALKALI  ){
					s_mode = s_mode_return_coolalkali ;
					ion_state = s_mode & 0x00f;
					
					if(ion_state==1)				Voice_output(SND_STEP1);        	
					else if(ion_state==2)		Voice_output(SND_STEP2);       
					else if(ion_state==3)		Voice_output(SND_STEP3);     
				}
				else{ //  이전 모드 s_mode 
					s_mode = s_mode_return;
					
					Voice_output(SND_SELECT);
				}
			}
			else{
				if( (m_state & 0x02) && (key_value !=  TCH_START) && ((s_mode&0x7f0) != MODE_SET)){ //출수중 다른키가 눌리면 정지해야 함
					key_value = TCH_START;
				}
				
				key_new = key_value;
				return_mode_cnt=0;   		// 모드 초기화 30초 카운터 클리어
				
				//test= key_new;
				
				backlight_off_cnt=0;   		// 20초 카운터 클리어
				//backlight_en_fg = 1;
				
				//080218 Voice_delay_output 키처리시 작동시킴
				sound_out_fg = 0;
				byWarning_delay_cnt = 0;
				filter_change_cnt = 0;
			}
		}
		else { //플러싱 모드에서 키 안 먹음
			Voice_output(SND_SELECT);
		}
	}
	old_key_value = key_value;	
	
	
}
/**************************************************************************************************
		KEY 동작
**************************************************************************************************/
void Key_exe(void)
{
	BYTE bRunFg;      // 키를 받은 시점에 출수 중이었는지
	BYTE bDlyFg;      // 키를 받은 시점에 출수 시작을 기다리는 중이었는지
	BYTE bImmFg;      // 1 = 지연 없이 바로 Key_action() 을 불러야 하는 경우
	
#if FILTER_WRITER
	switch(key_new)
	{
		case TCH_HOT :
		case TCH_HOT_LONG :		bFwSel = 0;	break;   // 3000 L
		case TCH_PURE :			bFwSel = 1;	break;   //  101 L
		case TCH_COOL :			bFwSel = 2;	break;   //   71 L
		case TCH_ALKALI :		bFwSel = 3;	break;   //   31 L
		case TCH_COOLALKALI :	bFwSel = 4;	break;   //    6 L
		
		case TCH_CLEAN :
			// 굽기 모드 순환 : 정품(빨강) -> 키위조(파랑) -> ID위조(노랑)
			if(++bFwKeyMode > 2)	bFwKeyMode = 0;
			break;
		
		case TCH_CLEAN_LONG :
			// 세정 3초 : 되쓰기 차단 기록 전체 초기화
			Rb_Clear(0);
			bFwRbClrCnt = 100;                       // 1초간 표시
			Voice_output(SND_CONFIRM);
			key_new = 0x00;
			return;
		
		case TCH_ML :
		case TCH_ML_MAX :
			// UID 확인 화면 토글
			bFwDispMode ^= 1;
			if(bFwDispMode)	Fw_Uid_Read();
			break;
		
		case TCH_START :
			if(bFwState != 1)	bFwState = 1;        // 쓰기 요청만 세움
			key_new = 0x00;
			return;
		
		default :
			key_new = 0x00;
			return;
	}
	
	bFwState = 0;                                    // 선택 변경 -> 대기
	bFwErr1  = 0;	bFwErr2 = 0;
	Voice_output(SND_SELECT);
	
	key_new = 0x00;
	return;
#endif

	bRunFg = (BYTE)((m_state & 0x02) ? 1 : 0);
	bDlyFg = bStartDelayFg;
	bImmFg = 0;
	
	// 어떤 키가 눌리든 출수 시작 대기는 먼저 취소한다.
	//  대기 중에 모드를 바꾸고 1초 뒤에 그 모드로 출수가 시작되면 안 된다.
	bStartDelayFg  = 0;
	bStartDelayCnt = 0;
	
  	switch(key_new)
  	{
  	  	case TCH_MODE : 	// 모드
     		if(!(m_state&0x02) && !((s_mode&0xff0)==MODE_SET))		// 설정모드가 아니고 동작상태가 아니면
     		{ 	
				backlight_off_cnt = 0;
				backlight_en_fg = 1;
				
     			Set_value_temp();
				
     		  	s_mode = VOICE_SET;
				bSetIntroFg = 1;  
     		 	Voice_output(SND_MODESET);
				
				wModeSetOut_10SecCnt = 0;
				bModeSetOutFg = 1;
     		}                                				
     		break;                          				
        case TCH_NEXT : 	// 모드 next
     		if((s_mode&0xff0)==MODE_SET)						// 설정모드 일때
     		{ 
				backlight_off_cnt = 0;
				backlight_en_fg = 1;
				
				wModeSetOut_10SecCnt = 0;	
				
				if( (m_state & 0x02)==0){ // 동작중이 아니면
				
					if(++s_mode > SET_MAX) s_mode = VOICE_SET;	//110926_1 Language set
					
					if(s_mode == PH_SET)	ion_state = 1;
					else if(s_mode == PH_SET2)	ion_state = 2;
					else if(s_mode == PH_SET3)	ion_state = 3;
					
					Voice_output(SND_SELECT); 
				}
				else { //  cal_out, ion_out 동작중이면
					Voice_output(SND_FAIL); 
				}
					
     		}                                				
     		break;                                          				
  	  	case TCH_SET :   // 설정  , enter                   				
			if((s_mode&0xff0)==MODE_SET)						// 설정모드 일때
			{                       
				backlight_off_cnt = 0;
				backlight_en_fg = 1;
				
				wModeSetOut_10SecCnt = 0;
				
				if( (m_state & 0x02)==0){ // 동작중이 아니면
					Set_value_save();	//100202_3. 모드에서 변경된 값은 한번에 저장
					//eep_save_fg=1;
					Voice_output(SND_MODEUNSET);    	

					s_mode = s_mode_return;	
				}
				else{  //  cal_out, ion_out 동작중이면
					Voice_output(SND_FAIL); 
				}
              
		  	}                                				
      	break;
		case TCH_ML: //120 ,250 , 500 mL
			if( s_mode >=ALKA && (s_mode <=HOT) && backlight_en_fg ==1 ){
				
				backlight_off_cnt = 0;
				backlight_en_fg = 1;
				
				if(  !(m_state & 0x02)){ //출수중이 아니면 mL 변경
					
					if(++bSet_mL>=3)	bSet_mL = 0;
				
					lTargetFlow =bDisp_mL[ bSet_mL] * 120;
					Voice_output(SND_120 +bSet_mL );
	
					wKeyInput_10SecCnt = 0;
					bKeyInputFg = 1;
				}
				else{ // 출수중이면 정지 
				
				}
			}

		break;
		case TCH_ML_MAX: //연속출수
			if( ((s_mode&0x7f0) ==ALKA) || (s_mode ==PURE)  ){
				if(  !(m_state & 0x02)){ //출수중이 아니면
					
					backlight_off_cnt = 0;
					backlight_en_fg = 1;
					
					bSet_mL = 3;
					Voice_output(SND_Conti);
			
					wKeyInput_10SecCnt = 0;
					bKeyInputFg = 1;
				}
			}
		break;
		/*case TCH_CLEAN: // 짧게 누름: 세정 모드 대기 상태로 변경
            if( !(m_state & 0x02) ) { // 출수 중이 아니면
                s_mode = CLEAN;
                s_mode_return = s_mode;
                ion_state = 0;

                Voice_output(SND_SELECT); // 세정 대기음
                
                wKeyInput_10SecCnt = 0;
                bKeyInputFg = 1;
            }
        break;*/

        case TCH_CLEAN_LONG: // 3초 길게 누름: 세정 출수 바로 시작
            if( !(m_state & 0x02) ) { // 출수 중이 아니면
				
				backlight_off_cnt = 0;
				backlight_en_fg = 1;
				
                // 1. 모드를 CLEAN으로 세팅 (Key_action에서 0x800 더해져 CLEAN_OUT으로 바뀜)
                s_mode = CLEAN; 
             // ???   s_mode_return = s_mode;
                ion_state = 0;
				bManualCleanFg = 1;   // 수동 세정 : LED 점멸
                Voice_output(SND_CONFIRM); 

                // 2. Start 키를 누른 것과 완벽히 동일하게 동작하도록 상태 변경
                //m_state &= 0xfd;   // 동작정지
                //m_state |= 0x80;   // 단수 동작 시작
                
                // 3. 출수 로직 호출
                //Key_action();      
            }
        break;
  	  	case TCH_PURE :   // 정수
			if(((s_mode&0xff0)==MODE_SET) || (s_mode==ION_OUT1) || (s_mode==ION_OUT2)  || (s_mode==CAL_OUT)) // (+)
			{
				backlight_off_cnt = 0;
				backlight_en_fg = 1;
				
				wModeSetOut_10SecCnt = 0;	
				
				Key_mode_inc(s_mode);  // 증가
				
				Voice_output(SND_UP);      				               
			}                                				
			else 
			{                    
				backlight_off_cnt = 0;
				backlight_en_fg = 1;
				
				s_mode = PURE;      
				//s_mode_return  = s_mode;				
				ion_state = 0;                				
				
				Voice_output(SND_SELECT);    
							
			} 	  	      	
      	break;
		
		case TCH_COOL : //냉수
	
				backlight_off_cnt = 0;
				backlight_en_fg = 1;
				
				s_mode = COOL;           
				//s_mode_return  = s_mode;				
				ion_state = 0;                				
			  
				if(bSet_mL > 2)   bSet_mL = 0;   // 연속 출수는 알칼리수/정수만
		
				Voice_output(SND_SELECT);        
						
		
		break;
		case TCH_ALKALI : //알칼리수
	
			if( backlight_en_fg==0){
				backlight_off_cnt = 0;
				backlight_en_fg = 1;
				
				s_mode = s_mode_return_alkali ;           
				//s_mode_return  = s_mode;	
				//s_mode_return_alkali = s_mode;
				
				ion_state=s_mode & 0x00f ;
				if(ion_state==1)				Voice_output(SND_STEP1);        	
					else if(ion_state==2)		Voice_output(SND_STEP2);       
					else if(ion_state==3)		Voice_output(SND_STEP3);      	
			}
			else if( (s_mode&0x7f0) != 	ALKA ){
				s_mode = s_mode_return_alkali ;           
				//s_mode_return  = s_mode;	
				//s_mode_return_alkali = s_mode;
				ion_state=s_mode & 0x00f ;
				
				if(ion_state==1)				Voice_output(SND_STEP1);        	
				else if(ion_state==2)		Voice_output(SND_STEP2);       
				else if(ion_state==3)		Voice_output(SND_STEP3);      
			}
			else {
				if(++ion_state>=4)	 ion_state= 1;                				
				s_mode = ALKA | ion_state;           
				//s_mode_return  = s_mode;	
			    
				if(ion_state==1)				{Voice_output(SND_STEP1);    }     	//s_mode_return_alkali = s_mode; 
				else if(ion_state==2)		{ Voice_output(SND_STEP2);    }     //s_mode_return_alkali = s_mode;
				else if(ion_state==3)		{                                                   Voice_output(SND_STEP3);    }   
			}				

		break;			
		case TCH_COOLALKALI : //냉알칼리수
			if( (s_mode & 0x7f0) != MODE_SET){
				if( backlight_en_fg==0){
					backlight_off_cnt = 0;
					backlight_en_fg = 1;
					s_mode = s_mode_return_coolalkali; 
					//s_mode_return  = s_mode;		
					//s_mode_return_coolalkali = s_mode ;
					
					ion_state=s_mode & 0x00f ;    

					if(bSet_mL > 2)   bSet_mL = 0;   // 연속 출수는 알칼리수/정수만
					if(ion_state==1)				Voice_output(SND_STEP1);        	
					else if(ion_state==2)		Voice_output(SND_STEP2);       
					else if(ion_state==3)		Voice_output(SND_STEP3);      		
				}
				else if( (s_mode&0x7f0) != 	COOLALKALI ){
					s_mode = s_mode_return_coolalkali; 
					//s_mode_return  = s_mode;		
					//s_mode_return_coolalkali = s_mode ;
					ion_state=s_mode & 0x00f ;    
					if(bSet_mL > 2)   bSet_mL = 0;   // 연속 출수는 알칼리수/정수만
					if(ion_state==1)				Voice_output(SND_STEP1);        	
					else if(ion_state==2)		Voice_output(SND_STEP2);       
					else if(ion_state==3)		Voice_output(SND_STEP3);      
				}
				else{
					if(bSet_mL > 2)   bSet_mL = 0;   // 연속 출수는 알칼리수/정수만
					if(++ion_state>=4)	 ion_state= 1;   
					
					s_mode = COOLALKALI  | ion_state;           
					//s_mode_return  = s_mode;	
						
					if(ion_state==1)				{Voice_output(SND_STEP1);}        //	s_mode_return_coolalkali = s_mode ;
					else if(ion_state==2)		{Voice_output(SND_STEP2);}       //    s_mode_return_coolalkali = s_mode ;
					else if(ion_state==3)		{															Voice_output(SND_STEP3); }      
				}
			}
			else if((s_mode & 0x7ff) == DEACTIVATE_SET){   // 설정모드 : 비활성 버튼 표시 on/oFF 토글
				backlight_off_cnt = 0;
				backlight_en_fg   = 1;
				wModeSetOut_10SecCnt = 0;

				if(con_deact_show == DEACT_SHOW_ON)  con_deact_show = DEACT_SHOW_OFF;
				else                                 con_deact_show = DEACT_SHOW_ON;

				bl_phase = 0;                              // 다음 틱에서 즉시 반영
				Voice_output(SND_SELECT);
			}
		break;		
		case TCH_HOT_LONG : //온수
			if( (s_mode & 0x7f0) != MODE_SET){
				backlight_off_cnt = 0;
				backlight_en_fg = 1;
				
				
				if((m_state & 0x02)==0){
					if(bHotLockFg==0){ //잠금해제 ->잠금
						bHotLockFg = 1;
						Voice_output(SND_HOT_LOCK);

					}
					else{ // 잠금-> 잠금해제
						s_mode = HOT;    
						
						ion_state = 0;
						
						
						// 온도는 이전 값 유지 bSetHotTemper
						
						wHot_out_cnt =0;
						wHot_stop_cnt =0;

						wHotOut_WaitTimeCnt=0;		
						bHotOut_WaitFg=1;
						
						bHotLockFg = 0;
						
						if(bSet_mL > 2)   bSet_mL = 0;   // 연속 출수는 알칼리수/정수만
						
						Voice_output(SND_HOT_UNLOCK);
					}
					
					
				}	
			}

			
		break;			
		case TCH_HOT : //온수
			if( (s_mode & 0x7f0) != MODE_SET){
				//if( bHotLockFg==0){//s_mode== HOT &&  // 잠금상태가 아닐때 
					
					if(backlight_en_fg==0){
						backlight_off_cnt = 0;
						backlight_en_fg = 1;
					}
					else{
						backlight_off_cnt = 0;
						backlight_en_fg = 1;
						
						if(s_mode == HOT  && (bHotLockFg==0)){
							if(bSetHotTemper--<=0)	bSetHotTemper = 2;
						}
					}
					if(bSet_mL > 2)   bSet_mL = 0;   // 연속 출수는 알칼리수/정수만
					s_mode = HOT;                				
					ion_state = 0;                				
					  
					if(bSetHotTemper==0)	Voice_output(SND_45+language_jump);      
					else if(bSetHotTemper==1)	Voice_output(SND_75+language_jump);        
					else if(bSetHotTemper==2)	Voice_output(SND_95+language_jump);    			

				if( bHotLockFg==0){
					if(bSet_mL > 2)   bSet_mL = 0;   // 연속 출수는 알칼리수/정수만
					
					if(bSetHotTemper==0)	Voice_output(SND_45+language_jump);      
					else if(bSetHotTemper==1)	Voice_output(SND_75+language_jump);        
					else if(bSetHotTemper==2)	Voice_output(SND_95+language_jump);    			
				}		
				else{//잠금 상태일때  [온수잠금설정중입니다. 3초가 눌러주세요]
					
					Voice_output(SND_HOT_LOCK3SEC);     
					
				}
			/*	}
				else { //잠금 상태일때 
					backlight_off_cnt = 0;
					backlight_en_fg = 1;
					Voice_output(SND_FAIL);     
				}*/
			}
		break;	
  	  	case TCH_START : // 동작/대기
			if( (filter1_life >= FILTER_FLUSHING)  ||   (filter2_life >= FILTER_FLUSHING) )
			{
				s_mode = FLUSHING;
						
				first_clean_en_fg = 0; 
				auto_clean_fg = 0;
				
				clean_delay_cnt = 0;
			auto_clean_fg = 0; 
		
			auto_clean_cnt = 0;
			first_clean_fg = 0;			// 초기세정완료 
				
					backlight_off_cnt = 0;
				backlight_en_fg = 1;
				Key_action();
			}
			//else if((s_mode>=MODE_SET && s_mode < PH_SET) ||  (s_mode==UV_STOP_SET )||  (s_mode==UV_RUN_SET ))									//설정모드에서 동작안되게
			/* 변경 후 : PH 캘리브레이션 3개 모드에서만 출수 허용 */
			else if( ((s_mode & 0x7f0) == MODE_SET) &&
			(s_mode != PH_SET) && (s_mode != PH_SET2) && (s_mode != PH_SET3) )
			{	
				backlight_off_cnt = 0;
				backlight_en_fg = 1;
				
				wModeSetOut_10SecCnt = 0;	
				//Voice_output(SND_FAIL);  
			}
			else if (bDispenseRestPeriod == 1) {
				backlight_off_cnt = 0;
				backlight_en_fg = 1;
				
				Voice_output(SND_FAIL); // "띠띠띠" 실패음 출력
			}
			else if( bHotLockFg==1 && (s_mode&0x7ff)==HOT){
				backlight_off_cnt = 0;
				backlight_en_fg = 1;
				
				Voice_output(SND_HOT_LOCK3SEC  );    //  SND_FAIL
			}
			else if(auto_clean_cnt >= auto_temp &&(auto_clean_fg==0) && (first_clean_en_fg==0) && !(m_state & 0x02)) // (auto_clean_value*10000L))	// 설정한 리터보다 크면 자동세정 동작
			{
			
					auto_clean_fg = 1;				// 자동세정시작
					bManualCleanFg = 0; 
					Voice_output(SND_CLEAN+language_jump);
					
					voice_out_cnt = 350;   // 세정 멘트 시간후 경보음
					auto_clean_cnt = 0;
			
			}
		
			else
			{
				backlight_off_cnt = 0;
				backlight_en_fg = 1;
				
				if(clean_delay_cnt > 1)
				{ 
					clean_delay_cnt=0;
					auto_clean_fg=0; 
      	  	
					// 080218 이전모드가 알칼리4단계이면 알칼리 2단계로 이동
    		  	    /* 	if(prev_alkali_mode != ALKA3)
    		  	  	{
    		  	  		s_mode = prev_alkali_mode | 0x80;    // 알카리수 상태로 이동
    		  	  	}
    		  	  	else
    		  	  	{
    		  	  		s_mode = ALKA2 | 0x80; 
    		  	  	}*/
      	  	  	      	  
      	  	       Voice_output(SND_CONFIRM);          	
      	  	  	                                    	
					m_state &= 0xfd;   							// 동작정지
					m_state |= 0x80;   							// 단수 동작 시작 
					bImmFg = 1;                                 // 단수 전환은 지연 없이
					//s_mode_old = s_mode;
				}
				
				// 출수 시작만 1초 늦춘다
				//  정지는 늦추면 안 되므로 출수 중이면 바로 Key_action()
				//  기다리는 중에 또 누르면 출수하지 않고 취소만 한다 (위에서 카운트도 지웠다)
				if( bRunFg || bImmFg )
				{
					Key_action();
				}
				else if( !bDlyFg )
				{
					bStartDelayFg  = 1;        // 1초 뒤에 출수 시작
					bStartDelayCnt = 0;
				}
			}
      	break;
  	  	case TCH_UP :
			backlight_off_cnt = 0;
			backlight_en_fg = 1;
		
			wModeSetOut_10SecCnt = 0;	
		
			Key_mode_inc(s_mode);		 
			if(inc_en_fg==0)	Voice_output(SND_SELECT);     	
			break;
		
  	  	case TCH_DN :
			backlight_off_cnt = 0;
			backlight_en_fg = 1;
		
			wModeSetOut_10SecCnt = 0;	
			Key_mode_dec(s_mode);			 
			if(dec_en_fg==0)	Voice_output(SND_SELECT);     		
			break;			      	
  	 	
		default :   // 디폴트
			break;

  	
  	}

 // 	if((s_mode & 0x70) == 0x30) {		//알칼리수
  ////		ion_state = s_mode & 0x0c;
  //	}
  	
	//if(alka_max_fg) ion_tbl[2] = 110;	//101217_1 turbo function
	//else ion_tbl[2] = 100;//190409

	key_new = 0x00;
	

	
	
	//온수 모드에서 온수 출수 대기 시간----------------------
	if(bHotOut_WaitFg)
	{
		if( (m_state & 0x02) || (m_state & 0x80) )	wHotOut_WaitTimeCnt = 0; //출수중이면 
		
		wKeyInput_10SecCnt = 0;  // ???
		
		if( ++wHotOut_WaitTimeCnt >= 1000) //10ms*1000 = 10000ms
		{
			wHotOut_WaitTimeCnt = 0;
			bHotOut_WaitFg = 0;

			
			s_mode = s_mode_return & 0x7ff; //이전 모드로 
		}
	}
	
	//출수 후 10초 이상 출수하지 않으면 출수량  120ml로 변경----------------------------
	if(bKeyInputFg){
		
		if( (m_state & 0x02) || (m_state & 0x80) )	wKeyInput_10SecCnt = 0; //출수중이면 
		
		if( ++wKeyInput_10SecCnt>= 1000) {  //10ms * 1000
			wKeyInput_10SecCnt = 0;
			bKeyInputFg = 0;
			
			if(bSet_mL>0) {
				bSet_mL = 0;
	
					
			}
		}
	}
	
	//설정 모드에서 입력 10초간 입력 없으면 모드설정 해제
	/*if(bModeSetOutFg){
		
		if( (m_state & 0x02) || (m_state & 0x80) )	wModeSetOut_10SecCnt = 0; //출수중이면 
		
		if( ++wModeSetOut_10SecCnt>= 2000) {  //10ms *2000
			wModeSetOut_10SecCnt = 0;
			bModeSetOutFg = 0;
			
			Set_value_call(s_mode);       		
			s_mode = s_mode_return & 0x7ff; //이전 모드로 
			
			//이전 볼륨으로 설정 
			Voice_delay_output(sound_volum_tbl[volume_level]);
			delay_1ms(50);	//old 35
			Voice_output(SND_FAIL);     		
			
		}
	}*/
				
}
/**************************************************************************************************
		동작/정지 KEY 실행
**************************************************************************************************/
/**************************************************************************************************
		출수 시작 지연 (100ms)
		  출수 버튼을 누르면 바로 나가지 않고 1초 뒤에 Key_action() 을 부른다.
		  기다리는 동안 키가 눌리면 Key_exe() 가 취소하고 카운트를 지운다.
**************************************************************************************************/
void Start_Delay_Check(void)
{
	if(!bStartDelayFg)
	{
		bStartDelayCnt = 0;
		return;
	}
	
	// 기다리는 사이에 출수를 막는 에러가 뜨면 취소한다
	if(lError & ERR_DISPENSE_STOP)
	{
		bStartDelayFg  = 0;
		bStartDelayCnt = 0;
		return;
	}
	
	// 기다리는 사이에 다른 경로로 출수가 시작됐으면 더 할 일이 없다
	if(m_state & 0x02)
	{
		bStartDelayFg  = 0;
		bStartDelayCnt = 0;
		return;
	}
	
	if(++bStartDelayCnt >= START_DELAY_TICK)
	{
		bStartDelayFg  = 0;
		bStartDelayCnt = 0;
		
		Key_action();           // 여기서 실제로 출수를 시작한다
	}
}

void Key_action(void)
{
	ULONG pwm_temp=0;

	s_mode ^= 0x800;	//출수와 정지 상태 전환	
	//if(s_mode==PCLEAN) s_mode = PURE;	//151117_1	//전세정시 정수상태에서 정지
	
	//151215_1 필터가 1L보다 클때 자동세정시 동작 제한
	if(first_clean_en_fg)//&&((filter1_life>F_MIN_LIFE)&&(filter2_life>F_MIN_LIFE)))
	{
		s_mode &= 0x7ff;	
	}
	else {	//151215_1 필터가 1L 이면
		first_clean_en_fg=0;	
		
	//	if((filter1_life<=F_MIN_LIFE)||(filter2_life<=F_MIN_LIFE))
		{	
			if(m_state!=0x02)	//출수 중이 아니면 
			{
	//			s_mode &= 0x7fF;
	//???			Voice_output(SND_FILTER+language_jump); 
			}
		}	
	}
	
	//출수 기준으로 대기화면-> 이전모드 저장 , 최초 
	if(m_state != 0x02  && first_clean_en_fg==0){
		if( ((s_mode & 0x7ff) != HOT) && ((s_mode & 0x7ff) != CLEAN) && ((s_mode & 0x7f0) != MODE_SET)&& ((s_mode & 0x7f0) != FLUSHING)){
			s_mode_return  = s_mode & 0x7ff;		
		}
			
		if(  (s_mode & 0x7ff) == ALKA1 ||  (s_mode & 0x7ff) == ALKA2){ //alka1,2 단계만 이전모드 저장
			s_mode_return_alkali  = s_mode_return;
		}
		else if(  (s_mode & 0x7ff) == COOLALKALI1 || (s_mode & 0x7ff) == COOLALKALI2    ){
			s_mode_return_coolalkali  = s_mode_return;
		}		
		else{
		
		}		
	}
	
	if(bFlushingEnd){
		bFlushingEnd = 0;
		s_mode = PURE;
	}
	
	//에러이면 정지
	if(lError & ERR_DISPENSE_STOP){
		if(s_mode&0x800)		s_mode &= 0x7ff;
	}		
		
	
	// 만약 ORP 설정 메뉴일때 2(전류 케리브레이션), 3(전압 켈리브레이션) 일때만 처리
	//if(s_mode==CAL_OUT)
	//{
	//	if((ion_state==1)||(ion_state>3))
	//	{
				// 출수 가능 삭제
	//			s_mode = ORP_SET;
	//	}
	//}
  	
	//151215_1 출수 모드이고 필터가 있을때 출수 키 동작 -> 필터가 1L 보다 작으면 정지 
	if((s_mode&0x800)) //&&((filter1_life>F_MIN_LIFE)&&(filter2_life>F_MIN_LIFE))) 
	{
  		//filter_sound_out_fg = 1;	//필터교체시기 음성 1회 출력
	


   		switch(s_mode)
   		{
    		case CLEAN_OUT:              		// 세정중
				if(!auto_clean_fg)	
				{
					auto_clean_fg = 1;			
		        		
					Voice_output(SND_CLEAN+language_jump);   
			  		voice_out_cnt = 350;   							//080214 초 // 세정 멘트 시간후 경보음
		  			auto_clean_cnt = 0;

					s_mode &= 0x7ff;
	  			}
	  			else
	  			{
	  				if(first_clean_fg) {
	  					Pwm_off();//PWM_DATA  = 0;        	// 080418_1 LED_type에서 Power off 시키기
	  				}
	  				else {
		  				pwm_temp = alkali_ph_step[2];	//4단계 150320_1
					  	pwm_value = pwm_init_tbl[pwm_temp-1]; 
						if(pwm_value>3280)  pwm_value = 3280;	//  	//080616_1 경제형 PWM 최대값 제한  	  		    			
		    			Pwm_out(pwm_value);
	  				}
	  	  	}
				
	    		break;
	  		case ALKA1_OUT:              						// 알카리 1단계 동작중	  		
				
			
				Voice_output(SND_ALKA1+language_jump);    
				prev_alkali_mode = ALKA1;     		// 해당 단계 저장 (리턴할 주소)
	      	break;
	  		case ALKA2_OUT:	  		     
				
			
				Voice_output(SND_ALKA2+language_jump);     
				prev_alkali_mode = ALKA2;     		// 해당 단계 저장 (리턴할 주소)
	     		break;
			case ALKA3_OUT:	  		     
				
	
				Voice_output(SND_ALKA3+language_jump);     
				prev_alkali_mode = ALKA3;     		// 해당 단계 저장 (리턴할 주소)
	     		break;
			case COOLALKA1_OUT:
			
				//열전소자,팬  정지 
			
				PwmCool_off();
			
				Voice_output(SND_COOLALKA1+language_jump);    
				prev_alkali_mode = ALKA1;
				break;
			case COOLALKA2_OUT:
				//열전소자,팬  정지 
			
				PwmCool_off();	
				Voice_output(SND_COOLALKA2+language_jump);    
				prev_alkali_mode = ALKA2;
				break;
			case COOLALKA3_OUT:
				//열전소자,팬  정지 
		
				PwmCool_off();	
				Voice_output(SND_COOLALKA3+language_jump);    
				prev_alkali_mode = ALKA3;
				break;
	  		case HOT_OUT:
					
			
				bHotOut_WaitFg=0;
				wHotOut_WaitTimeCnt=0;		
			
				wHot_out_cnt =0;
				wHot_stop_cnt =0;
			
				Voice_output(SND_HOT+language_jump);    

			break;
	  		case PURE_OUT:            							// 정수
			
  				Voice_output(SND_PURE+language_jump);
		    break;
			case COOL_OUT:            		
				
  				Voice_output(SND_COOL+language_jump);
		    break;
	  		case ION_OUT1:            							// 알카리 PH 설정시 출수
			case ION_OUT2:     	
				if(ion_state==1)
				{
					Voice_output(SND_ALKA1+language_jump);
					prev_alkali_mode = ALKA1;      // 해당 단계 저장 (리턴할 주소)
				}
				else if(ion_state==2)
				{
					Voice_output(SND_ALKA2+language_jump);
					prev_alkali_mode = ALKA2;    // 해당 단계 저장 (리턴할 주소)
				}
				else if(ion_state==3)
				{
					Voice_output(SND_ALKA3+language_jump);
					prev_alkali_mode = ALKA3;    // 해당 단계 저장 (리턴할 주소)
				}
				
	      	break;
	  		case CAL_OUT:            // calibration시 pwm 초기값                  
				pwm_value = CAL_PWM;	//080418_2 080515_4
	  			Pwm_out(pwm_value);
	      	break;
			
			case FLUSHING_OUT:
				bFlushingStep = 0;
			    bFlushingEnd=0;
			    bFlushingEndCnt=0;
			    wFlushingCnt=0;
				bDoorOpen_20SecFg = 0;
			
				Voice_output(SND_CLEAN+language_jump);   
			break;
	  		default :
	  	      break;
	  	}
	  	
	  	m_state &= 0x7f;   	// 단수 동작 정지
	  	m_state |= 0x02;   	// 물유입
	  	after_clean_fg = 1;	// 080418_1 전류제어 가능하게 플레그 설정 -> 후세정 후 전류인가 금지하게 하기 위해
	  	new_active = 1;
		ion_stop_cnt = 0;
	
		lFlowSum= 0; 
		lFlowSum_Hot = 0;
		lFlowSum_Out = 0;
		
	  	s_mode_old = s_mode;
	  	ion_stop_cnt = 0;
	}
	else		//출수 모드가 아닐때
	{	
		//if(alkali_ph_step[3]>70) alkali_ph_step[3]=70;	//100512_3->130923
		smps_clear_cnt=0;			//100511_1 smps 보호 해제 카운터 초기화
		smps_protect_cnt=0;		//100511_1 전류제한 카운터 초기화
		alka_max_fg = 0;			//101217_1 전류 최대 출력 클리어
		eep_data_fg |= 0x400;	//151210 자동세정이후 현재까지 사용한 유량 저장

		if(first_clean_en_fg)	//출수모드가 아니면
		{
			first_clean_en_fg = 0;
			first_clean_fg = 1;	//080828_2	전원인가후 세정되고 있는 상태 플레그
			s_mode = CLEAN;		//080214	  		
			auto_clean_fg = 1;	//전원인가후 세정모드로 들어가게함
			bManualCleanFg = 0;
			//260623 처음 세정중 소리 안나옴  Voice_delay_output(sound_volum_tbl[volume_level]);
			//260623 delay_1ms(50);	//old 35

				Voice_output(SND_CLEAN+language_jump);    
				voice_out_cnt = 350;   							//080214 초 // 세정 멘트 시간후 경보음
				auto_clean_cnt = 0;
		  }
		  else
		  {
				//151215_1 필터가 있을때 음성출력
				//if((filter1_life>F_MIN_LIFE)&&(filter2_life>F_MIN_LIFE)) 
					{
					//Voice_output(SND_CONFIRM);
				}
				
				m_state &= 0xfd;   // 동작정지
				ion_ok_fg = 0;

				new_active = 0;
			//	 ??? auto_clean_fg = 0;
				first_clean_fg = 0;		//080828_2	전원인가후 세정되고 있는 상태 플레그
		// ??? 		auto_clean_cnt=0;
		  }		
	}
	
	if(old_active && !new_active)
	{
	  	m_state |= 0x80;   // 단수 동작 시작 
	  	s_mode_old = s_mode;
	}

	old_active = new_active;
}
/**************************************************************************************************
		증가 KEY 처리
**************************************************************************************************/
void  Key_mode_inc(WORD  bMode)
{	
  	switch(bMode)
  	{

  	  	case VOICE_SET : 
			if(++volume_level>4) volume_level=0;
			Voice_delay_output(sound_volum_tbl[volume_level]);
			delay_1ms(50);	//old 35
   		break;
		case LANG_SET :	//110926_1
			if(++lang_level>7) lang_level=0;
			
		break;  	
		case CLEAN_SET :	//080214 세정 설정 최대값 변경 990->100
			auto_clean_value += auto_key_value;
			if(auto_clean_value > 10) auto_clean_value=1;
   		break;  
  	  	
  	  	case PH_SET :
		   alkali_ph_step[0] += auto_key_value;
		   if (alkali_ph_step[0] > 57) alkali_ph_step[0] = 1;
		break;
		case PH_SET2 :
			alkali_ph_step[1]+=auto_key_value;
			if(alkali_ph_step[1] > 57) 	alkali_ph_step[1]=1;
		break;
		case PH_SET3 :
			alkali_ph_step[2]+=auto_key_value;
			if(alkali_ph_step[2] > 57) 	alkali_ph_step[2]=1;
		break;
		case UV_STOP_SET :
			set_uv_stop_time += UV_STOP_STEP;
			if(set_uv_stop_time>UV_STOP_MAX) 		set_uv_stop_time = UV_STOP_MIN;
			break;
		case UV_RUN_SET :
			set_uv_run_time += UV_RUN_STEP;
			if(set_uv_run_time>UV_RUN_MAX) 	    set_uv_run_time = UV_RUN_MIN;
			break;
		
		case UV_CORK_SET :
			set_uv_cork_sec_time += UV_CORK_STEP;
			if(set_uv_cork_sec_time>UV_CORK_MAX) 	    set_uv_cork_sec_time = UV_CORK_MIN;
			break; 
		case UV_CORK_ONOFF:
			set_uv_cork_onoff +=1;
			if(set_uv_cork_onoff>1) 	    set_uv_cork_onoff = 0;
			break; 
		case BRIGHTNESS_SET :
			con_brightness += 1;
			if(con_brightness>100) 	    con_brightness = 5;
			break;
		case DEACTIVATE_SET :
			con_deactivate += 1;
			if(con_deactivate>100) 	    con_deactivate = 5;
			break;
		case HOT_OVER_T_SET :
            con_hot_over_temp += 1;
            if(con_hot_over_temp > HOT_OVER_T_MAX) con_hot_over_temp = HOT_OVER_T_MIN;
            break;

        case HOT_OVER_S_SET :
            con_hot_over_sec += 1;
            if(con_hot_over_sec > HOT_OVER_S_MAX) con_hot_over_sec = HOT_OVER_S_MIN;
            break;

        case TOUCH_SENS_SET :
            con_touch_sens += 1;
            if(con_touch_sens > TOUCH_SENS_MAX) con_touch_sens = TOUCH_SENS_MIN;
            break;
		
  	  	case ION_OUT1 :
		case ION_OUT2 :	
			switch(ion_state) //150320_1
			{
				case 1 : 
					if(alkali_ph_step[0] < 57) 	alkali_ph_step[0] += auto_key_value;
					
					break;
				case 2 : 
					if(alkali_ph_step[1] < 57) 	alkali_ph_step[1]+=auto_key_value;

					break;  
				case 3 :
					if(alkali_ph_step[2] < 57) 	alkali_ph_step[2]+=auto_key_value;

					break;
			                      
				default : 
					break;  
			}
   		break;
  	 
  	  	case CAL_OUT : 
			if(m_state&0x02)
			{
				pwm_value += 5;
				if(pwm_value>=5000) pwm_value=5000;
				Pwm_out(pwm_value);
			}
   		break;
	
  	  	default : 
     		break;  
  	}
}

/**************************************************************************************************
		감소 KEY 처리
**************************************************************************************************/
void  Key_mode_dec(WORD bMode)
{
  	switch(bMode)
  	{
  	  	
  	  	case VOICE_SET : 
			if(volume_level >= 1) --volume_level;
			else 	volume_level=4;
			Voice_delay_output(sound_volum_tbl[volume_level]);
			delay_1ms(50);	//old 35
   		break;
		case LANG_SET :	//110926_1
			if(lang_level >= 1) --lang_level;
			else lang_level = 7;
			break;  	
		case CLEAN_SET : 
			if(auto_clean_value > auto_key_value) auto_clean_value -= auto_key_value;
			else 	auto_clean_value=10;
      		break;
  	  
  	  	case PH_SET :
			if(alkali_ph_step[0]>1) 	alkali_ph_step[0] -= auto_key_value;
			else alkali_ph_step[0] = 57;
			break;
		case PH_SET2 :
			if(alkali_ph_step[1]>1) 	alkali_ph_step[1] -= auto_key_value;
			else alkali_ph_step[1] = 57;
		break;
		case PH_SET3 :
			if(alkali_ph_step[2]>1) 	alkali_ph_step[2] -= auto_key_value;
			else alkali_ph_step[2] = 57;
			break;
		case UV_STOP_SET :
			if(set_uv_stop_time>UV_STOP_MIN) 	set_uv_stop_time -= UV_STOP_STEP;
			else set_uv_stop_time = UV_STOP_MAX;
			break;
		case UV_RUN_SET :
			if(set_uv_run_time>UV_RUN_MIN) 	set_uv_run_time -= UV_RUN_STEP;
			else set_uv_run_time = UV_RUN_MAX;
			break;
		case UV_CORK_SET :
	
			if(set_uv_cork_sec_time>UV_CORK_MIN) 	    set_uv_cork_sec_time -= UV_CORK_STEP;
		    else 	set_uv_cork_sec_time=UV_CORK_MAX;
			break;
		case UV_CORK_ONOFF:
			
			if(set_uv_cork_onoff>0) 	   set_uv_cork_onoff -=1;
			else set_uv_cork_onoff=1;
			break; 
		case BRIGHTNESS_SET :
			if(con_brightness>5) 	    con_brightness -= 1;
		    else 	con_brightness=100;
			
			break;
		case DEACTIVATE_SET :
			if(con_deactivate>5) 	    con_deactivate -= 1;
		    else 	con_deactivate=100;
			break;
		
		case HOT_OVER_T_SET :
            if(con_hot_over_temp > HOT_OVER_T_MIN) con_hot_over_temp -= 1;
            else                                   con_hot_over_temp = HOT_OVER_T_MAX;
            break;

        case HOT_OVER_S_SET :
            if(con_hot_over_sec > HOT_OVER_S_MIN) con_hot_over_sec -= 1;
            else                                  con_hot_over_sec = HOT_OVER_S_MAX;
            break;

        case TOUCH_SENS_SET :
            if(con_touch_sens > TOUCH_SENS_MIN) con_touch_sens -= 1;
            else                                con_touch_sens = TOUCH_SENS_MAX;
            break;
		
		 
  	  	case ION_OUT1 :
		case ION_OUT2 :	
	     	switch(ion_state)
	     	{
	     	  	case 1 :    
  	  	    		if(alkali_ph_step[0]>1) 	alkali_ph_step[0] -= auto_key_value;
  	  	    		break;
	     	  	case 2 :    
  	  	    		if(alkali_ph_step[1]>1) 	alkali_ph_step[1] -= auto_key_value;
  	  	    		break;  
	     	  	case 3 :    
  	  	    		if(alkali_ph_step[2]>1) 	alkali_ph_step[2] -= auto_key_value;
  	  	    		break;
	     	  	                      
	     	  	default : 
  	  	    		break;  
	     	}     
	   	break;
  	  
  	  	case CAL_OUT : 
			//if(m_state&0x02)
			{
				if(pwm_value > 5) pwm_value-=5;
				Pwm_out(pwm_value);   
			}
   		break;
		 	
  	  	      		
  	  	default :
   		break;  	  
  	}
}


/**************************************************************************************************
		유량 단수시 값 초기화
**************************************************************************************************/
void 	Flow_stop(void)
{
	orp_minus_fg = 0;
	ion_ok_fg = 0;

	if(ion_state == 0)
	{
		ph_value = ion_tbl[ion_state];
	
	}
	else if(ion_state == 1)
	{
		ph_value = ion_tbl[ion_state];
	
	}
	else if(ion_state == 2)
	{
		ph_value = ion_tbl[ion_state];
		
	}	
	else if(ion_state == 3)
	{
		ph_value = ion_tbl[ion_state];
	
	}	

}

	
/**************************************************************************************************
		세정 및 필터 교체시 
**************************************************************************************************/
void Warning_voice(void)
{
	//BYTE warning_active_fg = 0;
	
	
  	if(ionize_fg)
  	{
    	//if(filter_change_fg && flow_in_fg && filter_change_en)
		//??????????
	
    	if(filter_change_en &&  !(lError& ERR_FLOW)  && !voice_out_cnt && !bFilterChgVFg)		// !flow_error
    	{
    	  	
    	  	Voice_output(SND_FILTER+language_jump); 						// 필터 교체 시기입니다.  
    	  	voice_out_cnt = 700;
    	  	filter_change_cnt = 0;
			filter_change_en = 0;
			if(filter_change_fg&0x03)	bFilterChgVFg =1; //최초 한번만
    	}
    	// 산성, 세정, 급수에러 일때만 경고음 080109
    	else if(((s_mode&0xff0)==CLEAN_OUT) )		//||flow_error)	080611_1 E0에러시 음성 이상 수정
    	{
			//warning_active_fg = 1;
			
    	  	if(voice_out_cnt==0 )// && (volume_level > 0)) //무음일때 출력 안함
    	  	{
    	  	  	//if(byWarning_delay_cnt == 0 || (++byWarning_delay_cnt>=WARNING_DELAY))		//3sec 25*15
				if(++byWarning_delay_cnt>=WARNING_DELAY)		//3sec 25*15
    	  	  	{
    	  	  	  	byWarning_delay_cnt=0;
    	  	  	  	Voice_output(SND_ERR);  // 경고음
    	  	  	}
    	  	}
    	}
    //	else if(filter_error||temp_error||flow_error|| (lError & ERR_LEAK) || (lError &ERR_HOT_OVER)|| (lError &ERR_COVER_OPEN)|| (lError &ERR_COVER_OPEN))		//080421_1 정수출수중 유량에러 발생시 경고음 출력 
		else if(lError)
    	{
	 	  	if(voice_out_cnt==0)
	 	  	{
				//warning_active_fg = 1;
				
	 	  	  	if(!filter_error)
	 	  	  	{
		 	  	  	//if(byWarning_delay_cnt == 0 || (++byWarning_delay_cnt>=WARNING_DELAY))		//150ms * 25 = 3.75sec
					if( ++byWarning_delay_cnt>=WARNING_DELAY)		//150ms * 25 = 3.75sec
		 	  	  	{
		 	  	  	  	byWarning_delay_cnt=0;
		 	  	  	  	Voice_output(SND_ERR);  // 경고음
		 	  	  	}
	 	  	  	}
	 	  	  	else
	 	  	  	{
		 	  	  	//if(byWarning_delay_cnt == 0 || (++byWarning_delay_cnt>=WARNING_DELAY))		//080611_3 필터 확인시 음성출력 delay 보상
					if( ++byWarning_delay_cnt>=WARNING_DELAY )
		 	  	  	{
		 	  	  	  	byWarning_delay_cnt=0;
		 	  	  	  	Voice_output(SND_ERR);  // 경고음
		 	  	  	}
	 	  	  	}
	 	  	}
    	}
   }
   else
   {
    	if(filter_change_en && !flow_error && !voice_out_cnt && !bFilterChgVFg)		//160201_1 정수에서 음성 출력 간격 변경 500->1024
    	{
    	
    	  	Voice_output(SND_FILTER+language_jump); 						// 필터 교체 시기입니다.  
    	  	voice_out_cnt = 700;
    	  	filter_change_cnt = 0;
    	 	filter_change_en = 0;
			
			if(filter_change_fg&0x03)	bFilterChgVFg =1;
    	}
   	
	   //if(filter_error||temp_error||flow_error || (lError & ERR_LEAK) || (lError &ERR_HOT_OVER)|| (lError &ERR_COVER_OPEN))				//080421_1 정수출수중 유량에러 발생시 경고음 출력  
		else if(lError)
	   {
	 	  	if(voice_out_cnt==0)
	 	  	{
				//warning_active_fg = 1;
				
	 	  	  	if(!filter_error)
	 	  	  	{
		 	  	  	//if(byWarning_delay_cnt == 0 || (++byWarning_delay_cnt>=WARNING_DELAY))	//150ms * 25 = 3.75sec
					if( ++byWarning_delay_cnt>=WARNING_DELAY )
		 	  	  	{
		 	  	  	  	byWarning_delay_cnt=0;
		 	  	  	  	Voice_output(SND_ERR);  // 경고음
		 	  	  	}
	 	  	  	}
	 	  	  	else
	 	  	  	{
		 	  	  	//if(byWarning_delay_cnt == 0 || (++byWarning_delay_cnt>=WARNING_DELAY))	//080611_3 필터 확인시 음성출력 delay 보상
					if( ++byWarning_delay_cnt>=WARNING_DELAY )
		 	  	  	{
		 	  	  	  	byWarning_delay_cnt=0;
		 	  	  	  	Voice_output(SND_ERR);  // 경고음
		 	  	  	}
	 	  	  	}
	 	  	}
	   }
	   else if(bDoorOpen_20SecFg==1){ 
		  // warning_active_fg = 1;
		   
			//if(byWarning_delay_cnt == 0 || (++byWarning_delay_cnt>=WARNING_DELAY))	//3sec 25*15
		   if( ++byWarning_delay_cnt>=WARNING_DELAY )
			{
				byWarning_delay_cnt=1;
				Voice_output(SND_SELECT);  // 띵
			}
	   }
	    else if(bDoorOpen_20SecFg==2){ 
			//warning_active_fg = 1;
			
			//if(byWarning_delay_cnt == 1 || (++byWarning_delay_cnt>=WARNING_DELAY))	//3sec 25*15
			if( ++byWarning_delay_cnt>=WARNING_DELAY )
			{
				byWarning_delay_cnt=0;
				Voice_output(SND_ERR);  // 경고음
			}
	   }
		/*else if(bFlushingEnd==1){ 
			if(++byWarning_delay_cnt>=10)		//3sec 25*15
			{
				byWarning_delay_cnt=0;
				Voice_output(SND_SELECT);  // 띵
				
				if(++bFlushingEndCnt>=3){
					bFlushingEndCnt = 0;
					bFlushingEnd = 0;
				}
			}
	   }*/
   }
   
   //if (warning_active_fg == 0)
    //{
   //     byWarning_delay_cnt = 0;
   // }
	
  /* if (voice_out_cnt == 0 && bRestoreMute == 1) {
    //    Set_Hardware_Volume(0);
        bRestoreMute = 0;
    }*/
}
#if FILTER_WRITER
/**************************************************************************************************
		필터 칩 쓰기 치구
**************************************************************************************************/
// UID 확인용 : 필터 1,2 의 UID 16바이트 합을 3자리로 만든다
//  0x4B 미지원이면 전부 0xFF 로 읽혀 16 x 255 = 4080 -> 080 이 나온다
//  반단선이면 전부 0x00 -> 000
//  정상이면 칩마다 다른 값이 나온다 (칩을 바꿔 끼워 비교할 것)
void Fw_Uid_Read(void)
{
	uint8_t uid[16];
	uint8_t i;
	WORD    sum;
	
	F1_ReadUID(0, uid);
	sum = 0;
	for(i=0;i<16;i++)	sum += uid[i];
	wFwUid1 = sum % 1000;
	
	F2_ReadUID(0, uid);
	sum = 0;
	for(i=0;i<16;i++)	sum += uid[i];
	wFwUid2 = sum % 1000;
}

// last_load_err -> 화면 표시용 2자리 코드로 변환
BYTE Fw_ErrCode(int8_t e)
{
	if(e ==   0)	return  0;
	if(e == -12)	return 12;      // 칩 미연결 / 하네스 불량
	if(e == -11)	return 11;      // 빈 칩 : 기록이 안 됨
	if(e ==  -1)	return  1;      // 키 불일치
	if(e ==  -3)	return  3;      // CRC 손상
	if(e ==  -4)	return  4;      // 값 범위 초과
	if(e ==  -5)	return  5;      // chip_id 불일치
	
	return 8;                       // 미정의
}

// 필터 1 쓰기 : 0 = 성공 , 그외 = 원인 코드
BYTE Filter_Write_1(ULONG v)
{
	LoadFilter1_Init();                      // 새 칩 UID 로 AES 키 재유도 + chip_id1
	
	if(!bChipPresent1)	return 12;           // JEDEC ID 로 미연결 확인
	
	ReadCnt( ADDR_F1 , v);                   // 암호문 생성 (SaveCipher1)
	F1_Erase4K(0);
	delay_1ms(10);
	SaveFilter( ADDR_F1_ID , v);             // chip_id + 암호문 20바이트 기록
	delay_1ms(10);
	
	lFwRead1 = LoadFilter(ADDR_F1);          // 검증 읽기
	
	if(last_load_err1 != 0)	return Fw_ErrCode(last_load_err1);
	if(lFwRead1 != v)		return 9;        // 값 불일치
	
	Rb_Clear(1);                             // 새로 구웠으니 되쓰기 기록 초기화
	
	return 0;
}

// 필터 2 쓰기 : 0 = 성공 , 그외 = 원인 코드
BYTE Filter_Write_2(ULONG v)
{
	LoadFilter2_Init();
	
	if(!bChipPresent2)	return 12;
	
	ReadCnt_2( ADDR_F2 , v);
	F2_Erase4K(0);
	delay_1ms(10);
	SaveFilter_2( ADDR_F2_ID , v);
	delay_1ms(10);
	
	lFwRead2 = LoadFilter_2(ADDR_F2);
	
	if(last_load_err2 != 0)	return Fw_ErrCode(last_load_err2);
	if(lFwRead2 != v)		return 9;
	
	Rb_Clear(2);                             // 새로 구웠으니 되쓰기 기록 초기화
	
	return 0;
}

// 필터 1,2 에 선택값을 쓰고 읽어서 검증
void Filter_Write_Exe(void)
{
	lFwValue = filter_wr_tbl[bFwSel];
	
	lFwRead1 = 0;	lFwRead2 = 0;
	
	bFwErr1 = Filter_Write_1(lFwValue);      // 한쪽이 실패해도
	bFwErr2 = Filter_Write_2(lFwValue);      // 다른쪽은 계속 진행
	
	if( (bFwErr1 == 0) && (bFwErr2 == 0) )
	{
		bFwState = 2;                        // 완료
		Voice_output(SND_CONFIRM);           // 띠링
	}
	else
	{
		bFwState = 3;                        // 실패
		Voice_output(SND_FAIL);
	}
}
#endif  /* FILTER_WRITER */

/**************************************************************************************************
		필터 IC에 필터값 저장
**************************************************************************************************/
void Filter_life_save(void)
{  	

	//260223
	if(f1_life_save_fg)
	{
		if(filter1_life==0){ //260415 0쓰기 방지
			if(f1_save_cnt==0)	{
				f1_life_save_fg = 0;
				f1_save_cnt = 0; //210301
				eep1_err_cnt = 0;
			}
			else if(f1_save_cnt==1)	{
				filter1_life = 3000000L; // ????
			}
		}
		
		
		++f1_save_cnt; 
		
		if(f1_save_cnt==1){
			if(!bChipPresent1){                  // 칩 미연결 : 쓰기 중단
				f1_save_cnt = 0;
				f1_life_save_fg = 0;
				//return;
			}
			
			
			save_f1_life = filter1_life;
			ReadCnt( ADDR_F1 ,save_f1_life);
			
			F1_Erase4K(0);
		}
		else if(f1_save_cnt==2)
		{
			save_f1_life = filter1_life;
			eeprom_save_fg = 1;
			SaveFilter( ADDR_F1_ID ,save_f1_life);
			eeprom_save_fg =0;
		}
		else if(f1_save_cnt==3)
		{
			read_f1_ex_life = LoadFilter(ADDR_F1); // Eeprom_ex_load(1);
			if((read_f1_ex_life/1000) != (save_f1_life/1000))
			{
				if(++eep1_err_cnt>ERROR_CNT)
				{
					eep1_err_cnt = 0;
					filter_error_fg |= 0x01;    // 필터 #1 쓰기 에러
					f1_life_save_fg = 0;//231005
				}
				else
				{
					f1_save_cnt = 0; //231005
				}
			}
			else
			{
				f1_life_save_fg = 0;
				f1_save_cnt = 0; //210301
				eep1_err_cnt = 0;
				filter_error_fg &= 0xfe;    // 필터 #1 쓰기 에러 제거
			}
		}
	}
	
	
	if(f2_life_save_fg)
	{
		
		if(filter2_life==0){ //260415 0쓰기 방지
			if(f2_save_cnt==0)	{
				f2_life_save_fg = 0;
				f2_save_cnt = 0; //210301
				eep2_err_cnt = 0;
			}
			else if(f2_save_cnt==1)	{
				filter2_life = 3000000L; // ????
			}
		}
		
		
		++f2_save_cnt;//231005

		if(f2_save_cnt==1)//231005
		{	
			if(!bChipPresent2){                  // 칩 미연결 : 쓰기 중단
				f2_save_cnt = 0;
				f2_life_save_fg = 0;
				//return;
			}
			
			save_f2_life = filter2_life;
			ReadCnt_2( ADDR_F2 ,save_f2_life);
			
			F2_Erase4K(0);
			
		}
		else if(f2_save_cnt==2)
		{
			save_f2_life = filter2_life;
			eeprom_save_fg = 1;
			SaveFilter_2( ADDR_F2_ID ,save_f2_life);
			eeprom_save_fg = 0;	
		}
	    else if(f2_save_cnt==3)
		{
			read_f2_ex_life = LoadFilter_2(ADDR_F2); // Eeprom_ex_load(2);
			if((read_f2_ex_life/1000 )!= (save_f2_life/1000))
			{
				if(++eep2_err_cnt>ERROR_CNT)
				{
					eep2_err_cnt = 0;
					filter_error_fg |= 0x02;    // 필터 #1 쓰기 에러
				
					f2_life_save_fg = 0; // 231005
				}
				else
				{
					f2_save_cnt = 0; //231005
				}
			}
			else
			{
				f2_life_save_fg = 0;
				f2_save_cnt = 0; //231005
				eep2_err_cnt = 0;
				filter_error_fg &= 0xfd;    // 필터 #1 쓰기 에러 제거
			}
		}
	}

	//필터쓰기에러시 오류수정	080128
  	Filter_Err_Apply();

//231005	life_save_fg = 0;  
}

/**************************************************************************************************
		필터 IC의 필터값 체크
**************************************************************************************************/
void	Filter_life_check(void)         // 점검 기능 
{
	life_check_fg=1;
  	//// EEPROM 체크 /////////////////
  	ch_select++;
  	ch_select &= 0x01;
  	
  	//210220 필터 교체시 에러 변경
	if(ch_select)
	{
		/*  260223 시리얼 넘버 에러 삭제  
		if(f1_life_read_fg)	//210220 필터에러시에만 시리얼 에라 체크
	   {
			serial_number = Eeprom_ex_load(3);
		
			if(serial_number != FILTER_SERIAL)
			{
				if(++byF1_serial_err_cnt>ERROR_CNT) 
				{
					byF1_serial_err_cnt = 0;
					serial_err |= 0x01;	
				}
			}
			else 
			{
				byF1_serial_err_cnt = 0;
				serial_err &= 0xfe;
			}
		
			delay_1ms(10);	//210220 5->10
		}*/
						
		// 칩 존재 여부와 AES 키는 LoadFilter1_Init 에서만 갱신된다.
		// 칩을 뺐다 꽂았을 때 에러가 풀리도록, 에러 중이면 다시 확인한다.
		if( !bChipPresent1 || (filter_error_fg & F1_ERR_MASK) )
		{
			LoadFilter1_Init();
			delay_1ms(1);
		}
		
		read_f1_ex_life =  LoadFilter(ADDR_F1); // Eeprom_ex_load(1);
		//f1_life_check_fg = 0;		//080611_2 필터에러시 음성출력 이상

		if( (last_load_err1 == -1) && IsFilterSwapped(1) )
		{
			// 정품 칩이 반대 소켓에 끼워졌다 -> 위조가 아니라 읽기 오류로 알린다
			if(++f1_read_error_cnt>ERROR_CNT)
			{
				f1_read_error_cnt = 0;
				Filter_Err_Set(F1_ERR_MASK, 0x04);    // -> E01
			}
		}
		else if( (last_load_err1 == -1) || (last_load_err1 == -5) )
		{
			// 키 불일치 / chip_id 불일치 : 위조 · 다른 키로 쓴 칩
			if(++f1_read_error_cnt>ERROR_CNT)
			{
				f1_read_error_cnt = 0;
				Filter_Err_Set(F1_ERR_MASK, 0x01);    // -> E03
			}
		}
		else if( Rb_Check(1) )
		{
			// 되쓰기 감지
			if(++f1_read_error_cnt>ERROR_CNT)
			{
				f1_read_error_cnt = 0;
				Filter_Err_Set(F1_ERR_MASK, 0x01);    // -> E03
			}
		}
		else if(!read_f1_ex_life )
		{
			if(++f1_read_error_cnt>ERROR_CNT)	//210220 추가
			{
				f1_read_error_cnt = 0;
				Filter_Err_Set(F1_ERR_MASK, 0x04);    // -> E01
	  	   }
		}
		else if(read_f1_ex_life>F1_MAX_LIFE)
		{
				if(++f1_read_error_cnt>ERROR_CNT)
				{
					f1_read_error_cnt = 0;
					Filter_Err_Set(F1_ERR_MASK, 0x40);
				}
		}
		else
		{
			
			f1_life_check_fg = 0;	//210220 위치이동
			f1_read_error_cnt = 0;
			filter_error_fg &= 0xba;	//1011 1010  읽기에러만 클리어 080128-> 231005 0xbb->0xba 쓰기 에러도 클리어
		
		}
 	}
	else
	{
	  /*  260223 시리얼 넘버 에러 삭제  
		if(f2_life_read_fg)	//210220 필터에러시에만 시리얼 에라 체크
	  {
			serial_number1 = Eeprom_ex_load(4);
			if(serial_number1 != FILTER_SERIAL)
			{
				if(++byF2_serial_err_cnt>ERROR_CNT) {
					byF2_serial_err_cnt = 0;
					serial_err |= 0x02;	
				}
			}
			else {
				byF2_serial_err_cnt = 0;
				serial_err &= 0xfd;
				f2_life_read_fg = 0;				
			}
		
			delay_1ms(10);	//210220 5->10
		}*/
				
		
		// 칩 존재 여부와 AES 키는 LoadFilter2_Init 에서만 갱신된다.
		if( !bChipPresent2 || (filter_error_fg & F2_ERR_MASK) )
		{
			LoadFilter2_Init();
			delay_1ms(1);
		}
		
		read_f2_ex_life =  LoadFilter_2(ADDR_F2); // Eeprom_ex_load(2);
		//f2_life_check_fg = 0;		//080611_2 필터에러시 음성출력 이상
		if( (last_load_err2 == -1) && IsFilterSwapped(2) )
		{
			// 정품 칩이 반대 소켓에 끼워졌다 -> 위조가 아니라 읽기 오류로 알린다
			if(++f2_read_error_cnt>ERROR_CNT)
			{
				f2_read_error_cnt = 0;
				Filter_Err_Set(F2_ERR_MASK, 0x08);    // -> E02
			}
		}
		else if( (last_load_err2 == -1) || (last_load_err2 == -5) )
		{
			// 키 불일치 / chip_id 불일치 : 위조 · 다른 키로 쓴 칩
			if(++f2_read_error_cnt>ERROR_CNT)
			{
				f2_read_error_cnt = 0;
				Filter_Err_Set(F2_ERR_MASK, 0x02);    // -> E04
			}
		}
		else if( Rb_Check(2) )
		{
			// 되쓰기 감지
			if(++f2_read_error_cnt>ERROR_CNT)
			{
				f2_read_error_cnt = 0;
				Filter_Err_Set(F2_ERR_MASK, 0x02);    // -> E04
			}
		}
		else if(!read_f2_ex_life )  
		{
			if(++f2_read_error_cnt>ERROR_CNT)	//210220 추가
			{
				f2_read_error_cnt = 0;
				Filter_Err_Set(F2_ERR_MASK, 0x08);    // -> E02
			}
		}
		else if(read_f2_ex_life>F2_MAX_LIFE)
		{
			if(++f2_read_error_cnt>ERROR_CNT)
			{
				f2_read_error_cnt = 0;
				Filter_Err_Set(F2_ERR_MASK, 0x80);
			}
		}
		else
		{
				
			f2_life_check_fg = 0;	//210220 위치이동
			f2_read_error_cnt = 0;
			filter_error_fg &= 0x75;	//0111 0111  읽기에러만 클리어 080128 -> 231005 0x77->0x75 쓰기 에러도 클리어
			
		}
	}

  	//////////////// 필터 교체 필요 70L 이하
  	 if((filter1_life<=30000L) && (read_f1_ex_life)) 
  	{
  	  	filter_change_fg |= 0x10;
  	  	
  	}
	else if((filter1_life<=100000L) && (read_f1_ex_life))
  	{
  	  
  	  filter_change_fg |= 0x01;	
  	}
  	else
  	{
  	  	filter_change_fg &= 0xee;
  	 
  	}
  	
  	 if((filter2_life<=30000L) && (read_f2_ex_life))
  	{
		filter_change_fg |= 0x20;
  	  
  	}
	else if((filter2_life<=100000L) && (read_f2_ex_life))
  	{
  		filter_change_fg |= 0x02;
  	}
  	else
  	{
  	  	filter_change_fg &= 0xdd;
  	} 
  	  	
  	Filter_Err_Apply();
  	
  	life_check_fg=0;
}
/**************************************************************************************************
		필터1 초기및 에러 발생후 데이터 처리
**************************************************************************************************/
void F1_life_reload(void)	//151222_1 필터 교체시 체크 항목 수정
{
	/*  260223 시리얼 넘버  삭제  
	serial_number = Eeprom_ex_load(3);

	if(serial_number != FILTER_SERIAL)
	{
		if(++byF1_serial_err_cnt>ERROR_CNT) {
			byF1_serial_err_cnt = 0;
			serial_err |= 0x01;	
		}
	}
	else {
		byF1_serial_err_cnt = 0;
		serial_err &= 0xfe;
	}

	delay_1ms(5);*/
	
	LoadFilter1_Init();
	delay_1ms(1);

  read_f1_ex_life = LoadFilter(ADDR_F1); //  Eeprom_ex_load(1);				//080507_4 필터교체시 초기값 읽기
  
  // 키 불일치 / chip_id 불일치는 UID 읽기 실패일 수도 있으므로 1회 재시도
	if( (last_load_err1 == -12) || (last_load_err1 == -1) || (last_load_err1 == -5) )
	{
		LoadFilter1_Init();
		delay_1ms(2);
		read_f1_ex_life = LoadFilter(ADDR_F1);
	}
  
  filter1_life = read_f1_ex_life;					//080507_4 필터교체시 초기값 읽기
	wF1_life_old = (WORD)filter1_life/1000; //260223


	if(last_load_err1 == -12)
	{
		// 칩 미연결 / 하네스 불량 -> 읽기 오류
		if(++byF1_life_err_cnt>ERROR_CNT) {
			byF1_life_err_cnt = 0;
			Filter_Err_Set(F1_ERR_MASK, 0x04);    // -> E01
		}
	}
	else if( (last_load_err1 == -1) && IsFilterSwapped(1) )
	{
		// 정품 칩이 반대 소켓에 끼워졌다 -> 위조가 아니라 읽기 오류로 알린다
		if(++byF1_life_err_cnt>ERROR_CNT) {
			byF1_life_err_cnt = 0;
			Filter_Err_Set(F1_ERR_MASK, 0x04);    // -> E01
		}
	}
	else if( (last_load_err1 == -1) || (last_load_err1 == -5) )
	{
		// 키 불일치 또는 chip_id 불일치 : 위조 / 다른 키로 쓴 칩
		if(++byF1_life_err_cnt>ERROR_CNT) {
			byF1_life_err_cnt = 0;
			Filter_Err_Set(F1_ERR_MASK, 0x01);    // -> E03
		}
	}
	else if( Rb_Check(1) )
	{
		// 되쓰기 감지 : 다 쓴 칩의 잔량이 다시 늘었다
		if(++byF1_life_err_cnt>ERROR_CNT) {
			byF1_life_err_cnt = 0;
			Filter_Err_Set(F1_ERR_MASK, 0x01);    // -> E03
		}
	}
	else if(!filter1_life)
	{
		if(++byF1_life_err_cnt>ERROR_CNT) {
			byF1_life_err_cnt = 0;
			Filter_Err_Set(F1_ERR_MASK, 0x04);    // -> E01
  		}
  	}
  	else if(filter1_life>F1_MAX_LIFE)
  	{
		if(++byF1_life_err_cnt>ERROR_CNT) {
			byF1_life_err_cnt = 0;
			Filter_Err_Set(F1_ERR_MASK, 0x40);
		}
  	}
	else	//231005 에러해제
	{
		byF1_life_err_cnt = 0;
		filter_error_fg &= 0xba;	//1011 1011  읽기에러만 클리어 080128 231005  WRITE 에러도 클리어  0xbb ->0xba
	}
		
		
}

/**************************************************************************************************
		필터2 초기및 에러 발생후 데이터 처리
**************************************************************************************************/
void F2_life_reload(void)	//151222_1 필터 교체시 체크 항목 수정
{
	/* 260223 시리얼 넘버  삭제  
	serial_number1 = Eeprom_ex_load(4);
	if(serial_number1 != FILTER_SERIAL)
	{
		if(++byF2_serial_err_cnt>ERROR_CNT) {
			byF2_serial_err_cnt = 0;
			serial_err |= 0x02;	
		}
	}
	else {
		byF2_serial_err_cnt = 0;
		serial_err &= 0xfd;
	}
	

	delay_1ms(5);*/
	
	LoadFilter2_Init(); 
	delay_1ms(1);
 	read_f2_ex_life =LoadFilter_2(ADDR_F2); //  Eeprom_ex_load(2);				//080507_4 필터교체시 초기값 읽기
  
  // 키 불일치 / chip_id 불일치는 UID 읽기 실패일 수도 있으므로 1회 재시도
	if( (last_load_err2 == -12) || (last_load_err2 == -1) || (last_load_err2 == -5) )
	{
		LoadFilter2_Init();
		delay_1ms(5);
		read_f2_ex_life = LoadFilter_2(ADDR_F2);
	}
	
	
  filter2_life = read_f2_ex_life;					//080507_4 필터교체시 초기값 읽기
	wF2_life_old = (WORD)filter2_life/1000; //260223
	
	if(last_load_err2 == -12)
	{
		// 칩 미연결 / 하네스 불량 -> 읽기 오류
		if(++byF2_life_err_cnt>ERROR_CNT) {
			byF2_life_err_cnt = 0;
			Filter_Err_Set(F2_ERR_MASK, 0x08);    // -> E02
		}
	}
	else if( (last_load_err2 == -1) && IsFilterSwapped(2) )
	{
		// 정품 칩이 반대 소켓에 끼워졌다 -> 위조가 아니라 읽기 오류로 알린다
		if(++byF2_life_err_cnt>ERROR_CNT) {
			byF2_life_err_cnt = 0;
			Filter_Err_Set(F2_ERR_MASK, 0x08);    // -> E02
		}
	}
	else if( (last_load_err2 == -1) || (last_load_err2 == -5) )
	{
		// 키 불일치 또는 chip_id 불일치 : 위조 / 다른 키로 쓴 칩
		if(++byF2_life_err_cnt>ERROR_CNT) {
			byF2_life_err_cnt = 0;
			Filter_Err_Set(F2_ERR_MASK, 0x02);    // -> E04
		}
	}
	else if( Rb_Check(2) )
	{
		// 되쓰기 감지 : 다 쓴 칩의 잔량이 다시 늘었다
		if(++byF2_life_err_cnt>ERROR_CNT) {
			byF2_life_err_cnt = 0;
			Filter_Err_Set(F2_ERR_MASK, 0x02);    // -> E04
		}
	}
	else if(!filter2_life)  
	{
		if(++byF2_life_err_cnt>ERROR_CNT) {
			byF2_life_err_cnt = 0;
			Filter_Err_Set(F2_ERR_MASK, 0x08);    // -> E02
		}
	}
	else if(filter2_life>F2_MAX_LIFE)
	{
		if(++byF2_life_err_cnt>ERROR_CNT) {
			byF2_life_err_cnt = 0;
			Filter_Err_Set(F2_ERR_MASK, 0x80);
		}
	}
	else	//210220 에러해제
	{
		byF2_life_err_cnt = 0;
		filter_error_fg &= 0x75;	//0111 0111  읽기에러만 클리어 080128  231005  WRITE 에러도 클리어  0x77 ->0x75
	}
	
		
}

/**************************************************************************************************
		초기및 에러 발생후 데이터 처리
**************************************************************************************************/
void Filter_life_reload(void)	//151222_1
{
	F1_life_reload();
	F2_life_reload();
}
			
/**************************************************************************************************
		보이스 출력
**************************************************************************************************/

void Set_Hardware_Volume(BYTE level) {
   
    Voice_delay_output(sound_volum_tbl[level]); 
    delay_1ms(50); 
}
void Voice_output(BYTE Voice_val)
{      
  	sound_out_fg = 1;
	
	
	//  무음인데 에러음이 들어
    if (volume_level == 0 ){
		if ( Voice_val == SND_ERR
		  || Voice_val == (SND_FILTER      + language_jump)
		  || Voice_val == (SND_HOT_LOCK    )
		  || Voice_val == (SND_HOT_UNLOCK  )
		  || Voice_val == (SND_HOT_LOCK3SEC) ) {
			   Voice_delay_output(sound_volum_tbl[2]); 
				delay_1ms(50); // 강제로 볼륨 2단계로 올림
			
			
			bRestoreMute = 1;       // 나중에 다시 무음으로 
		}
		else{
			if (voice_out_cnt == 0 && bRestoreMute == 1) {
				Voice_delay_output(sound_volum_tbl[0]); 
				delay_1ms(50); 
				bRestoreMute = 0;
			}
		}
    }
	
	if(volume_level || bRestoreMute)
	{

		//VCLK_0; //260223 삭제 50ms 이후 신호 리셋됨, 음성 출력 안됨	  	
		voice_real_fg = 1;
		voice_delay_cnt = 0;
		
		voice_add = Voice_val;
		voice_add_old = voice_add;
		
	}
  	sound_out_fg = 0;	
}

/**************************************************************************************************
		보이스 출력(인터럽트 이용)
**************************************************************************************************/
void	Voice_real_output(void)	
{
	BYTE bit_count;
	BYTE	parity_bit;
	
  	sound_out_fg = 1;

	if(volume_level || bRestoreMute)
	{
		VCLK_0;				 //260223 
		delay_1ms(VD_2); //260223 
		
		parity_bit=0;
		
		for(bit_count=0;bit_count<8;bit_count++) 
		{
			if(voice_add&0x01) 
		  	{
		  	   VDATA_1;
		  	  	parity_bit=parity_bit+1;
		  	}
		  	else
		  	{
		  	  	VDATA_0;
		  	}
		  	
		  	delay_1ms(VD_2);
		  	voice_add=voice_add>>1;
		  	VCLK_1;
		  	delay_1ms(VD_2);
		  	VCLK_0;
		  	//delay_1ms(3);
		}
		
		if(parity_bit&0x01) 
		{
		  	VDATA_1;
		}
		else 
		{
			VDATA_0;
		}	
		delay_1ms(VD_2);
		VCLK_1;
		delay_1ms(VD_2);
		VDATA_1;
	  	//delay_1ms(VD_2);	  
	}
	
	
	
  	sound_out_fg = 0;
}
/**************************************************************************************************
		보이스 출력(delay 함수 사용)  
**************************************************************************************************/
void  Voice_delay_output(BYTE Voice_val)						
{                                                        
  	BYTE   bit_count;                                     
  	BYTE   parity_bit;                                    
                                                                                                                  
  	sound_out_fg = 1;

  	if(volume_level || bRestoreMute)
  	{
	 	VCLK_0;                                               
	  	delay_1ms(VD_1);	//0ld 11->15 080507_2 음성출력 시간 변경                                                                 
	  	                                                      
	  	voice_add = Voice_val;                             
	  	parity_bit=0;                                         
	  	                                                      
	  	for(bit_count=0;bit_count<8;bit_count++)              
	  	{                                                     
	  	  	if(voice_add&0x01)                              
	  	  	{                                                  
	  	  	  	VDATA_1;                                         
	  	  	  	parity_bit=parity_bit+1;                         
	  	  	}                                                  
	  	  	else                                               
	  	  	{                                                  
	  	  	  	VDATA_0;                                         
	  	  	}                                                  
	  	  	                                                   
	  	  	delay_1ms(VD_2);		//080214 2->3 080507_2 음성출력 시간 변경                                                                             
	  	  	voice_add=voice_add>>1;                      
	  	  	VCLK_1;                                            
	  	  	delay_1ms(VD_2);		//080214 2->3 080507_2 음성출력 시간 변경                                                                             
	  	  	VCLK_0;
	  	 // 	delay_1ms(VD_2);		//080507_2 음성출력 시간 변경                                       
	  	}
	  
	  	if(parity_bit&0x01) 
	  	{
	  	  	VDATA_1;
	  	}
	  	else 
	  	{
	  	  	VDATA_0;
	  	}
	  	delay_1ms(VD_2);	//080214 2->3 080507_2 음성출력 시간 변경                                                                             
	  	VCLK_1;
	  	delay_1ms(VD_2);	//080214 2->3 080507_2 음성출력 시간 변경                                                                             
	  	VDATA_1;
	  	delay_1ms(VD_2);	//080214 2->3 080507_2 음성출력 시간 변경                                                                            
  	}
  	delay_1ms(VD_3); 		//10->5 080507_2 음성출력 시간 변경                                       
  	sound_out_fg = 0;
}				

void fctest(void)
{
//	ttt=10;//
	//------------------------------------------------------
			if(ttt==0){
				//GD25D10_Read16Bytes_UID(0,spi1_read); 
				//GD25D10_2_Read16Bytes_UID(0,spi2_read); 
				
				
				//spi1_read[0] = GD25_ReadStatus(); //
			   //GD25D10_Read16Bytes(0,spi1_read);
				
				filter1_life = LoadFilter(24);
				filter2_life = LoadFilter_2(24);
				ttt=1;
			}
			else if(ttt==1){
			
				ttt=0;
			}
			else if(ttt==2)
			{
				GD25D10_EraseSector4K(0);

				ttt=3;
			}
			else if(ttt==3)
			{
				GD25D10_2_EraseSector4K(0);
				ttt=4;
			}
			else if(ttt==4)
			{
				SaveFilter(24,1010000);
				
				//GD25D10_ByteWrite(0,spi1_wr2);
				//GD25D10_2_ByteWrite(0,spi2_wr2);
				ttt=5;
			}
			else if(ttt==5)
			{
				SaveFilter_2(24,1010000);
				//GD25D10_ByteWrite(64, spi1_wr2);
				//GD25D10_2_ByteWrite(64,spi2_wr2);
				ttt=6 ;
			}		
			
		//------------------------------------
}


float fast_sqrt(float x) {
    if (x <= 0.0f) return 0.0f;
    
    xhalf = 0.5f * x;
    j = 0x5f3759df - ((*(int32_t*)&x) >> 1);
    y = *(float*)&j;
    
    y = y * (1.5f - y * y * xhalf);
    y = y * (1.5f - y * y * xhalf);
    
    return (x * y);  // x * (1/sqrt(x)) = sqrt(x)
}
/* --------------------------------- End Of File ------------------------------ */
