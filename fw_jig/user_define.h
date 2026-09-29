//===================================================================
// File Name : pMain.h
// Function  : pMain
// Program   : B.H. Im & Kits   --  Boo-Ree Multimedia Inc.
// Date      : April, 20, 2010
// Version   : 0.0
// Mail      : support@boo-ree.com
// Web       : www.boo-ree.com
// History
//===================================================================

#ifndef __USER_H__

#define __USER_H__
#include "A31G22x_hal_pcu.h"
#include "A31G22x_hal_scu.h"

//#define	DEBUG_FLOW		1

/***************************************************************************
   빌드 선택
     0 = 이온수기 본체 프로그램
     1 = 필터 칩 쓰기 치구 프로그램
   나머지 소스는 두 빌드가 완전히 동일하다. 이 줄만 바꾼다.
****************************************************************************/
#define FILTER_WRITER       1

/* 치구에서 쓸 필터 값 (mL 단위 , 본체는 filter1_life 를 mL 로 다룬다)
   온수 / 정수 / 냉수 / 알칼리수 / 냉알칼리수 버튼 순 */
#define FW_VAL_0         3000000    /* 온수       버튼 : 3000 L */
#define FW_VAL_1          101000    /* 정수       버튼 :  101 L */
#define FW_VAL_2           71000    /* 냉수       버튼 :   71 L */
#define FW_VAL_3           31000    /* 알칼리수   버튼 :   31 L */
#define FW_VAL_4            6000    /* 냉알칼리수 버튼 :    6 L */


#define BL_IDLE_CNT      6000u //6000u   // 1분   : 무입력 판정    
#define BL_FULL_CNT      3000u//3000u  // 30초  : 100% 유지       
#define BL_DIM_CNT       1000u   // 10초  : 20% 유지        
      



#define BL_T_FULL_END   (BL_IDLE_CNT + BL_FULL_CNT)   /// 9000 =  90초 
#define BL_T_DIM_END    (BL_T_FULL_END + BL_DIM_CNT)  //10000 = 100초 


#define HOT_FLOW_WIN        5    // 100ms x 5 = 500ms 이동창 (분해능 46 ml/min)
#define HOT_PID_I_LIMIT  3000    // 적분 클램프 (Ki/10 적용시 +-300 PWM )
#define HOT_T_OFFSET        0    // ho_temp 와 실제 출수 온도 차 보정 (0.1도)
// 온수 출수 : 목표 온도 기준 히터 듀티 제어
#define HOT_DUTY_PERIOD    10    // 100ms x 10 = 1초 저속 PWM 주기
#define HOT_T_KP           20    // /10 적용 -> 오차 1.0도당 2%
#define HOT_T_KI            5    // /1000 적용
#define HOT_T_I_LIMIT   20000    // 적분 클램프

#define COOL_READY_ON_TEMP    100    // 10.0도 이하 -> ON
#define COOL_READY_OFF_TEMP   110    // 11.0도 초과 -> OFF
// 냉수 온도센서 이상 저온 판정 (E12)
#define COOL_TEMP_LOW_ERR      10    // 1.0도 이하 -> 에러
#define COOL_TEMP_LOW_CLR      20    // 2.0도 초과 -> 해제
#define COOL_TEMP_LOW_CNT       3    // 5초 평균 x 3회 = 15초 연속시 에러 확정

// 플러싱 단계별 용량 (mL)
#define FLUSH_ML_FILTER      3000    // 0단계 : 필터 세정
#define FLUSH_ML_COOL        1000    // 1단계 : 냉수라인 물채우기
#define FLUSH_ML_HOT          300    // 2단계 : 온수라인 물채우기
#define FLUSH_ML_PURE         200    // 3단계 : 정수라인 물채우기
#define FLUSH_ML_HOT_TEST     250    // 4단계 : 온수관로 배수 + 히터 60도 테스트
#define FLUSH_ML_HOT_COOL     300    // 5단계 : 온수관로 잔열 배수

#define FLUSH_HOT_TEST_ON     550    // 히터 ON  : 55.0도 미만 (0.1도 단위)
#define FLUSH_HOT_TEST_OFF    600    // 히터 OFF : 60.0도 이상
#define FLUSH_HOT_LED_CNT      30    // 온수 LED 순차 점등 주기 10ms x 30 = 300ms

// 냉수모듈 UV 살균 설정 범위
#define UV_STOP_MIN        5    // 미동작 시간 최소 5분
#define UV_STOP_MAX      180    // 미동작 시간 최대 180분 (3시간)
#define UV_STOP_STEP       5    // 5분씩 변경
#define UV_STOP_DEF       50    // 기본 50분

#define UV_RUN_MIN        30    // 살균 시간 최소 30초
#define UV_RUN_MAX       600    // 살균 시간 최대 600초 (10분)
#define UV_RUN_STEP       30    // 30초씩 변경
#define UV_RUN_DEF       600    // 기본 600초 (10분)

#define SET_INTRO_CNT     20    // 설정모드 진입 애니메이션 단계 시간 10ms x 30 = 300ms


// 콕크 UV 살균 시간 설정 범위
#define UV_CORK_MIN        1    // 최소 1초
#define UV_CORK_MAX       14    // 최대 14초
#define UV_CORK_STEP       1    // 1초씩 변경
#define UV_CORK_DEF        5    // 기본 5초


#define  VDATA_1						HAL_GPIO_SetPin((PORT_Type *)PF,_BIT(8))	//VDATA=1;
#define  VDATA_0						HAL_GPIO_ClearPin((PORT_Type *)PF,_BIT(8))//VDATA=0;
                        		
#define  VCLK_1						HAL_GPIO_SetPin((PORT_Type *)PF,_BIT(9))	//VCLK=1;
#define  VCLK_0						HAL_GPIO_ClearPin((PORT_Type *)PF,_BIT(9))//VCLK=0;
              
#define  SOL1_CLOSE					HAL_GPIO_SetPin(PA,_BIT(9))	  	//SOL1_ON
#define  SOL1_OPEN					HAL_GPIO_ClearPin(PA,_BIT(9))   //SOL1_OFF

#define  SOL2_ON					HAL_GPIO_SetPin(PA,_BIT(10))	
#define  SOL2_OFF					HAL_GPIO_ClearPin(PA,_BIT(10))

#define  SOL3_ON					HAL_GPIO_SetPin(PA,_BIT(11))	
#define  SOL3_OFF					HAL_GPIO_ClearPin(PA,_BIT(11))

#define  SOL4_ON					HAL_GPIO_SetPin(PB,_BIT(11))	
#define  SOL4_OFF					HAL_GPIO_ClearPin(PB,_BIT(11))

#define  SOL5_ON					HAL_GPIO_SetPin(PB,_BIT(12))	
#define  SOL5_OFF					HAL_GPIO_ClearPin(PB,_BIT(12))

#define  SOL6_ON					HAL_GPIO_SetPin(PB,_BIT(13))	
#define  SOL6_OFF					HAL_GPIO_ClearPin(PB,_BIT(13))

#define  SOL7_ON					HAL_GPIO_SetPin(PB,_BIT(14))	
#define  SOL7_OFF					HAL_GPIO_ClearPin(PB,_BIT(14))

#define  SOL8_ON					HAL_GPIO_SetPin(PB,_BIT(15))	
#define  SOL8_OFF					HAL_GPIO_ClearPin(PB,_BIT(15))

#define  UVC1_ON					HAL_GPIO_SetPin(PE,_BIT(12))	
#define  UVC1_OFF					HAL_GPIO_ClearPin(PE,_BIT(12))

#define  UVC2_ON					HAL_GPIO_SetPin(PE,_BIT(13))	
#define  UVC2_OFF					HAL_GPIO_ClearPin(PE,_BIT(13))

#define  HEATER_OFF				HAL_GPIO_SetPin(PE,_BIT(2))	
#define  HEATER_ON				HAL_GPIO_ClearPin(PE,_BIT(2))


#define  RELAY_ON					HAL_GPIO_ClearPin((PORT_Type *)PF,_BIT(3))
#define  RELAY_OFF					HAL_GPIO_SetPin((PORT_Type *)PF,_BIT(3))//  HAL_GPIO_SetPin((PORT_Type *)PF,_BIT(3))	

#define  POWER_C_OFF			HAL_GPIO_ClearPin((PORT_Type *)PF,_BIT(2))
#define  POWER_C_ON			HAL_GPIO_SetPin((PORT_Type *)PF,_BIT(2))



//error
/*
#define	ERR_FLOW					0x0001  //E00
#define	ERR_FILTER_RD1			0x0002  //E01
#define	ERR_FILTER_RD2			0x0004  //E02
#define	ERR_FILTER_WR1		0x0008  //E03
#define	ERR_FILTER_WR2		0x0010  //E04
#define	ERR_SMPS_TEMP		0x0020  //E05
#define	ERR_LEAK					0x0040  //E06
#define	ERR_COVER_OPEN		0x0080  //E07
#define	ERR_HOT_OVER			0x0100  //E08
#define	ERR_HOT_TEMP_OPEN	0x0200  //E09
#define	ERR_COOL_TEMP_OPEN	0x0400  //E10
#define	ERR_PELTIER_TEMP		0x0800  //E11
*/


/* 필터 칩 JEDEC 제조사 확인
   0x9F 로 읽은 Manufacturer ID 가 다르면 정품 칩이 아닌 것으로 본다.
   양산 전에 실제 칩의 값을 확인할 것. 값이 다르면 매 부팅마다 E01 / E02 가
   뜨므로 바로 알 수 있고, GD25_MID_CHECK 를 0 으로 두면 확인을 건너뛴다. */
#define GD25_MID_CHECK		   1     /* 0 = 제조사 확인 안 함 */
#define GD25_MID_EXPECT		0xC8     /* GigaDevice */

#define SMPS_TEMP_ERR_ADC     944    // 85.0도 이상 -> 에러 (5초 연속)
#define SMPS_TEMP_CLR_ADC    1050    // 80.0도 이하 -> 해제 (10초 연속)


//error
#define	ERR_FLOW					0x0001  //E00 원수 유입
#define	ERR_FILTER_RD1			0x0002  //E01 필터1 라이프 읽기 오류
#define	ERR_FILTER_RD2			0x0004  //E02 필터2 라이프 읽기 오류
#define	ERR_FILTER_WR1		0x0008  //E03 필터1 위조/암호 오류
#define	ERR_FILTER_WR2		0x0010  //E04 필터2 위조/암호 오류
#define	ERR_SMPS_TEMP		0x0020  //E05 Main PCB 고온 85도
#define	ERR_SMPS_HEAT			0x0040  //E06 SMPS 발열체크 (미구현, 협의중)
#define	ERR_LEAK					0x0080  //E07 누수
#define	ERR_COVER_OPEN		0x0100  //E08 필터커버 오픈
#define	ERR_HOT_OVER			0x0200  //E09 온수 바이메탈 고온가열
#define	ERR_HOT_TEMP_SENSOR	0x0400  //E10 온수(입수) 온도센서
#define	ERR_HOT_TEMP_OPEN	0x0800  //E11 온수(출수) 온도센서
#define	ERR_COOL_TEMP_OPEN	0x1000  //E12 냉수 온도센서
#define	ERR_PELTIER_TEMP		0x2000  //E13 냉수모듈 냉각불량

// NOS 밸브는 열어둔 채 출수만 정지 : 필터 에러 + 기판 발열
#define	ERR_STOP_ONLY		(ERR_FILTER_RD1|ERR_FILTER_RD2|ERR_FILTER_WR1|ERR_FILTER_WR2 \
							|ERR_SMPS_TEMP|ERR_SMPS_HEAT)

// NOS 밸브 닫고 원수 차단 : 누수 / 커버 / 온수과열 / 센서 / 냉각 불량
#define	ERR_NOS_CLOSE		(ERR_LEAK|ERR_COVER_OPEN|ERR_HOT_OVER \
							|ERR_HOT_TEMP_SENSOR|ERR_HOT_TEMP_OPEN \
							|ERR_COOL_TEMP_OPEN|ERR_PELTIER_TEMP)

// 출수 중 에러 발생 시 출수 모드 해제 (원수 유입 E00 은 별도 처리)
#define	ERR_DISPENSE_STOP	(ERR_STOP_ONLY|ERR_NOS_CLOSE)

#define TICKS_5_MINUTES    (uint32_t)(5 * 60 * 2)  // 600 Ticks
#define TICKS_2_MINUTES    (uint32_t)(2 * 60 * 2 * 5)  // 240 Ticks

// cool pwm
#define PWM_PELTIER_24V  5000  // (24 / 24) * 5000
#define PWM_PELTIER_21V  4300  // (21 / 24) * 5000
#define PWM_PELTIER_14V  2500  //2916  // (14 / 24) * 5000
#define PWM_PELTIER_11V  1900   // 2291  // (11 / 24) * 5000
#define PWM_PELTIER_8V     1300//1666  // ( 8 / 24) * 5000


#define TARGET_ADC_130    2500 //2500 // 2800   // 4.30A
#define TARGET_ADC_071    1600 // 1720  //  2.8A
#define TARGET_ADC_056    1250 // 1350   // 2.28A
#define TARGET_ADC_040     700// 970    // 1.70A

#define TICKS_4_HOURS    		(4 * 60 * 60 * 2 ) // 36,000 ticks
#define TICKS_5_HOURS    		(5 * 60 * 60 * 2 ) // 36,000 ticks
#define TICKS_30_MINUTES 	(30 * 60 * 2 )      // 3,600 ticks


#define CLEAN_LONG_CNT   300     // 3초 (10ms * 300)

#define  KEY_CHATTERING			4 // 260223   15  //230712 10->15
#define  ML_LONG_CNT   		300    // 10ms 3초
#define  HOT_LONG_CNT   		300  

#define  WATER_PURE				0
#define  WATER_HOT				1
#define  WATER_COOL			2
#define  WATER_ALKALI			3
#define  WATER_COOLALKALI	4

/***************************************************************************
                              모드
****************************************************************************/
#define	STANDBY		     	 0x00

#define	MODE_SET		     0x010		//모드    
#define	VOICE_SET	         0x011		//음량 조절 (8 단계 :: 0=음소거)
#define	LANG_SET			 0x012		//언어 설정
#define	CLEAN_SET	         0x013		//세정(0~990 :: 10)             
#define	PH_SET		         0x014		//알칼리성pH (19 단계)    
#define	PH_SET2		         0x015	
#define	PH_SET3		         0x016	
#define	UV_STOP_SET			 0x017
#define	UV_RUN_SET			 0x018
#define	UV_CORK_SET		 0x019
#define	UV_CORK_ONOFF		 0x01a
#define	BRIGHTNESS_SET		 0x01b
#define	DEACTIVATE_SET		 0x01c
#define    HOT_OVER_T_SET     0x01d   /* 0x1C : 온수 과열 감지 온도 */
#define    HOT_OVER_S_SET   0x01e   /* 0x1D : 온수 과열 감지 시간 */

#define   TOUCH_SENS_SET   0x01f  /* 0x1E : 터치 감도          */

#define    SET_MAX	TOUCH_SENS_SET

#define	ION_SET				 0x810		//이온 설정
#define	ION_OUT1		     0x814	
#define	ION_OUT2		     0x815		//알칼리성pH (19 단계)          
#define	CAL_OUT		         0x816		//알칼리성 L (13 단계)   
											
#define	MANUAL	             0x019		//version 
                                  			
#define	CLEAN		             0x020		//세정                          
#define	CLEAN_OUT		     0x820		//세정 출수                         
                                  			
#define	ALKA		         	 0x030		//알칼리 
#define	ALKA1		         	 0x031		//알칼리 1단계  ==> 평상모드    
#define	ALKA2		         	 0x032		//알칼리 2단계                  
#define	ALKA3		         	 0x033		//알칼리 3단계                  
                                  			
#define	ALKA_OUT             0x830		//알칼리 출수             
#define	ALKA1_OUT           0x831		//알칼리 1단계 출수             
#define	ALKA2_OUT           0x832		//알칼리 2단계 출수             
#define	ALKA3_OUT           0x833		//알칼리 3단계 출수             


#define	PURE			 		0x050		//정수
#define	PURE_OUT		 	0x850		//정수 출수

#define	PCLEAN		 		0x060		//정수 동작전 세정
#define	PCLEAN_OUT		0x860		//정수 동작전 세정 출수

#define	COOL			 		0x040		//냉수
#define	COOL_OUT	 		0x840	


#define	COOLALKALI				0x70					
#define	COOLALKALI1			0x71			
#define	COOLALKALI2			0x72			
#define	COOLALKALI3			0x73			
#define	COOLALKA1_OUT		0x871			//알칼리 1단계  ==> 평상모드    
#define	COOLALKA2_OUT		0x872			//알칼리 2단계                  
#define	COOLALKA3_OUT		0x873			//알칼리 3단계        

#define	COOLALKALI_OUT	0x870	

#define	HOT			 				0x100				
#define	HOT_OUT			 		0x900	

#define     FLUSHING				0x200
#define     FLUSHING_OUT		0xa00

/***************************************************************************
                              키입력
****************************************************************************/
#define	TCH_CLEAN			    0x0010
#define	TCH_CLEAN_LONG   0x0011
#define	TCH_HOT					0x0020
#define 	TCH_HOT_LONG   	0x0021
#define	TCH_PURE				0x0040
#define	TCH_COOL				0x0080
#define	TCH_ALKALI				0x0100
#define	TCH_COOLALKALI	0x0200

#define 	TCH_SET					0x0400
#define 	TCH_UP					0x0401
#define 	TCH_DN					0x0402
#define 	TCH_CAL					0x0404
#define 	TCH_VERSION			0x0408

#define 	TCH_MODE				0x0800
#define 	TCH_NEXT				0x0801
#define 	TCH_ML					0x4000
#define 	TCH_ML_MAX			0x4100

#define 	TCH_START				0x8000

#define 	TCH_ALKA				0x1000  
#define 	TCH_STEP1				0x0001
#define 	TCH_STEP2				0x0002
#define 	TCH_STEP3				0x0004
#define 	TCH_STEP4				0x0008
#define 	TCH_ACID				0x2000
//#define 	TCH_PURE			0x0010
//#define 	TCH_START			0x0020

        	
#define 	TCH_ALKA1				0x1001
#define 	TCH_ALKA2				0x1002
#define 	TCH_ALKA3				0x1004
#define 	TCH_ALKA4				0x1008


#define CORK_LED_OFF      0
#define CORK_LED_ON       1
#define CORK_LED_DEF      CORK_LED_OFF  

/***************************************************************************
                              시간제어
****************************************************************************/
#define 	STOP_TIME				62	//5초후 정수 출수 완료
#define 	CLEAN_TIME				80		//8초후 후세정 완료
#define 	SAVE_TIME				3
#define 	OFF_TIME					1		//키동작 1초후 SOL1 Off
#define 	LOW_LIMIT_LITER 		200	// 100125 저유량 에러 1.0 -> 0.7L 이상 일때 장비 구동
#define 	HIGH_LIMIT_LITER		3500 	// 3.5L 100512_4
#define 	DIS_LIMIT_LITTER 		650	// 0.7L 이하면 0 표시
#define 	CAL_PWM					445 //// 260223  1300//경제형 SMPS


#define 	DOOR_OPEN_20SEC_CNT			2000 //10ms*2000=20000ms



//filter
#define ADDR_F1_ID			20
#define ADDR_F2_ID			20

#define ADDR_F1			24
#define ADDR_F2			24

#define	F1_MAX_LIFE				3000000L
#define	F1_MAX					3000L
	#define	F2_MAX_LIFE				3000000L
	#define	F2_MAX					3000L

#define FILTER_FLUSHING	  3000000L 

#define FILTER_60P				  2000000L //  ~2000
#define FILTER_30P				  1000000L // ~1000
#define FILTER_0P				    100000L //70

//#define PURE_LITER			2000  
//#define F_MIN_LIFE				1000	

#define  MAIN						0x0000
/**************************************************************************************************
                              TIME SET
**************************************************************************************************/
#define	FLICKER					10	//1sec
#define	FLICKER_ON				50	//500ms
#define  FLIKER					100	//2초
   
/***************************************************************************
                              키 음향 해당 주소                         
****************************************************************************/
#define 	SND_FAIL					24
#define 	SND_SELECT			25
#define 	SND_DN					26
#define 	SND_UP					27
#define 	SND_CONFIRM			28
#define 	SND_ERR					29
        
#define 	SND_STEP1					1
#define 	SND_STEP2					2
#define 	SND_STEP3					4
#define 	SND_ALKA1					5
#define 	SND_ALKA2					5
#define 	SND_ALKA3					5
#define 	SND_COOLALKA1			6
#define 	SND_COOLALKA2			6
#define 	SND_COOLALKA3			6
#define 	SND_PURE					7
#define 	SND_95						8
#define 	SND_75						9
#define 	SND_45						10
#define 	SND_HOT						11
#define 	SND_HOT_LOCK3SEC 	12
#define 	SND_HOT_LOCK			13
#define 	SND_HOT_UNLOCK	 	14
#define 	SND_COOL					15
#define 	SND_120						16
#define 	SND_250						17
#define 	SND_500						18
#define 	SND_Conti					19
#define 	SND_CLEAN				    20
#define 	SND_FILTER				   3

#define 	SND_ERROR				21
#define 	SND_MODESET			22
#define 	SND_MODEUNSET		23
     	
#define 	FLASH_SIZE				32	//짝수 설정 

/*-------------------------------------------------------------------------*/
/*                              키패트 LED                                 */
/*-------------------------------------------------------------------------*/	                  			
#define 	LED7_ON						0
#define 	LED7_OFF					1

/*-------------------------------------------------------------------------*/
/*                              바탕색 선언                                */
/*-------------------------------------------------------------------------*/                        	

#define	RUN_LED_OFF				HAL_GPIO_SetPin(PC,_BIT(12))
#define	RUN_LED_ON				HAL_GPIO_ClearPin(PC,_BIT(12))

#define	EOS_LED_UP_ON  		HAL_GPIO_SetPin(PA,_BIT(3))
#define	EOS_LED_DN_ON  		HAL_GPIO_SetPin(PA,_BIT(4))
#define	EOS_LED_UP_OFF  		HAL_GPIO_ClearPin(PA,_BIT(3))
#define	EOS_LED_DN_OFF		HAL_GPIO_ClearPin(PA,_BIT(4))


#define	BLUE_OFF 				HAL_GPIO_ClearPin(PA,_BIT(5))
#define	GREEN_OFF				HAL_GPIO_ClearPin(PA,_BIT(6))
#define	RED_OFF					HAL_GPIO_ClearPin(PA,_BIT(7))
#define	BLUE_ON					HAL_GPIO_SetPin(PA,_BIT(5))
#define	GREEN_ON				HAL_GPIO_SetPin(PA,_BIT(6))
#define	RED_ON					HAL_GPIO_SetPin(PA,_BIT(7))

											//B						G						R
#define 	BLACK						{BLUE_OFF; 	GREEN_OFF; 	RED_OFF;}		//000
#define 	DARK_RED					{BLUE_OFF; 	GREEN_OFF; 	RED_ON;}		//001
#define 	DARK_GREEN				{BLUE_OFF; 	GREEN_ON; 	RED_OFF;}		//010
#define 	LIGHT_GREEN				{BLUE_OFF; 	GREEN_ON; 	RED_ON;}		//011
#define 	DARK_BLUE				{BLUE_ON; 	GREEN_OFF; 	RED_OFF;}		//100
#define 	LIGHT_VIOLET			{BLUE_ON; 	GREEN_OFF; 	RED_ON;}		//101
#define 	BLUE						{BLUE_ON; 	GREEN_ON; 	RED_OFF;}		//110
#define 	LIGHT_BLUE				{BLUE_ON; 	GREEN_ON; 	RED_ON;}		//111
 	
/*-------------------------------------------------------------------------*/
/*                               설정 값 주소                              */
/*-------------------------------------------------------------------------*/

#define 	ADD_LCDBL		 		0			//0x01:ph or orp display         
#define	ADD_EOSBL		 		1			//0x02:EOS LED 설정
#define	ADD_VOLUME	         2			//0x04:음량 조절 (8 단계 :: 0=음소거)
#define	ADD_PURE_PH		      3			//0x08:정수pH (0.0~9.9 :: 0.1)       

#define	ADD_CLEAN	         	4			//0x10:세정(0~990 :: 10)             
#define	ADD_ALKA_PH	         5			//0x20:알칼리성pH (19 단계)          
#define	ADD_ORP		         	6			//0x40:알칼리성 L (13 단계) 
#define	ADD_LANG			 	7			//0x80:언어 설정

/*-------------------------------------------------------------------------*/
/*                               단계별 pH 값 주소                         */
/*-------------------------------------------------------------------------*/                        	
#define 	ADD_ALKA_1				8
#define 	ADD_ALKA_2				9
#define 	ADD_ALKA_3				10
#define 	ADD_ALKA_4 				11
        	
#define 	ADD_UV_MIN_1			12
#define 	ADD_UV_MIN_2			13
#define 	ADD_UV_SEC_1			14
#define 	ADD_UV_SEC_2			15
#define 	ADD_CORK_SEC			16
#define 	ADD_CORK_ONOFF		17
#define 	ADD_BRIGHTNESS		18
#define 	ADD_DEACTIVATE		19
#define 	ADD_HOT_OVER_T     20  /* byte0 */
#define 	ADD_HOT_OVER_S     21  /* byte1 */
#define 	ADD_TOUCH_SENS     22  /* byte2 */

#define 	ADD_DEACT_SHOW   23   // byte3 : 비활성 버튼 표시 여부
#define DEACT_SHOW_OFF    0
#define DEACT_SHOW_ON     1
#define DEACT_SHOW_DEF    DEACT_SHOW_ON


/*-------------------------------------------------------------------------*/
/*                               단계별 ORP 주소                           */
/*-------------------------------------------------------------------------*/                        	

/*-------------------------------------------------------------------------*/
/*                               자동 세정시점 저장 주소                   */
/*-------------------------------------------------------------------------*/
#define 	ADD_CLEAN_L_1  		24
#define 	ADD_CLEAN_L_2  		25
#define 	ADD_CLEAN_L_3  		26
#define 	ADD_CLEAN_L_4 			27
/*-------------------------------------------------------------------------*/
/*                               게인 값 주소                              */
/*-------------------------------------------------------------------------*/
#define 	ADD_GAIN_I_1				28
#define 	ADD_GAIN_I_2				29
#define 	ADD_AUTO_CLEAN_1	30
#define 	ADD_AUTO_CLEAN_2	31

#define 	ADD_INIT					    32   /* 32~35 : DATA_INIT 매직 4바이트 */

/*-------------------------------------------------------------------------*/
/*        되쓰기(롤백) 차단 기록  -  데이터 플래시 36~87번지               */
/*        0~31 설정값 , 32~35 DATA_INIT 매직 -> 33번지는 사용 불가          */
/*        FlashWrite() 가 256바이트 페이지를 통째로 쓰므로 36~255 는 여유   */
/*-------------------------------------------------------------------------*/
#define 	ADD_RB_MAGIC		36   /* 2바이트 (36,37) 기록 유효 표식        */
#define 	ADD_RB_IDX1			38   /* 1바이트 필터1 다음 덮어쓸 칸          */
#define 	ADD_RB_IDX2			39   /* 1바이트 필터2 다음 덮어쓸 칸          */
#define 	ADD_RB_TBL1			40   /* 필터1 (chip_id 4B + 잔량L 2B) x 4 = 24B (40~63) */
#define 	ADD_RB_TBL2			64   /* 필터2 24B (64~87)                     */

#define 	RB_MAGIC		0xA55A   /* 기록 유효 표식                        */
#define 	RB_HIST				 4   /* 슬롯당 기억하는 칩 개수               */
#define 	RB_MARGIN_L			20   /* 잔량이 기록보다 20L 이상 늘면 되쓰기   */
#define 	RB_STEP_L			10   /* 10L 줄어들 때마다 기록 갱신 (플래시 수명) */


#define 	DATA_INIT	0x53525190
#define 	DATA_INIT1	0x90
#define 	DATA_INIT2	0x51
#define 	DATA_INIT3	0x52
#define 	DATA_INIT4	0x53
/*-------------------------------------------------------------------------*/
/*                               필터 #1 주소                              */
/*-------------------------------------------------------------------------*/
#define 	ADD_FILTER_HH			0x00	// 외부 EEPROM
#define 	ADD_FILTER_HL			0x01	// 외부 EEPROM
        	
#define 	ADD_SERIAL1				0x02  // serial number 1
#define 	ADD_SERIAL2				0x03  // serial number 2
#define 	ADD_SERIAL3				0x04  // serial number 3
#define 	ADD_SERIAL4				0x05  // serial number 4
//    데이터 주소                         
#define 	PARAM_DATA_ADDR			0x10000


#define SAMPLE_CNT 100  // 1ms 샘플링 * 100개 = 100ms (60Hz의  6주기)
#define DC_OFFSET 2048  // 1.65V 기준 (4096 / 2)


#define HOT_OVER_T_MIN    90
#define HOT_OVER_T_MAX   110
#define HOT_OVER_T_DEF   105

#define HOT_OVER_S_MIN     1
#define HOT_OVER_S_MAX    15
#define HOT_OVER_S_DEF     5

#define TOUCH_SENS_MIN     0
#define TOUCH_SENS_MAX    15
#define TOUCH_SENS_DEF     0

#define HOT_OVER_HYST    250 



const int16_t SinX[SAMPLE_CNT] = {
      0,  377,  701,  927, 1022,  974,  789,  494,  128, -255, 
   -602, -865, -1006,-1016, -897, -652, -316,   64,  437,  746, 
    952, 1024,  952,  746,  437,   64, -316, -652, -897,-1016, 
  -1006, -865, -602, -255,  128,  494,  789,  974, 1022,  927, 
    701,  377,    0, -377, -701, -927,-1022, -974, -789, -494, 
   -128,  255,  602,  865, 1006, 1016,  897,  652,  316,  -64, 
   -437, -746, -952,-1024, -952, -746, -437,  -64,  316,  652, 
    897, 1016, 1006,  865,  602,  255, -128, -494, -789, -974, 
  -1022, -927, -701, -377,    0,  377,  701,  927, 1022,  974, 
    789,  494,  128, -255, -602, -865,-1006,-1016, -897, -652
};

const int16_t CosX[SAMPLE_CNT] = {
   1024,  952,  746,  437,   64, -316, -652, -897,-1016,-1006, 
   -865, -602, -255,  128,  494,  789,  974, 1022,  927,  701, 
    377,    0, -377, -701, -927,-1022, -974, -789, -494, -128, 
    255,  602,  865, 1006, 1016,  897,  652,  316,  -64, -437, 
   -746, -952,-1024, -952, -746, -437,  -64,  316,  652,  897, 
   1016, 1006,  865,  602,  255, -128, -494, -789, -974,-1022, 
   -927, -701, -377,    0,  377,  701,  927, 1022,  974,  789, 
    494,  128, -255, -602, -865,-1006,-1016, -897, -652, -316, 
     64,  437,  746,  952, 1024,  952,  746,  437,   64, -316, 
   -652, -897,-1016,-1006, -865, -602, -255,  128,  494,  789
};

/***************************************************************************
// system default I/O setting
****************************************************************************/
void SysTick_Handler (void); 	// SysTick Interrupt Handler @ 1000Hz
void TIMER3_IRQHandler(void); //pwm timer
void GPIODE_IRQHandler(void);
void ADC2_IRQHandler(void);
void Voice_Init(void);
void I2C1_Eep_init(void);

void mainloop(void);
void Ad_conversion(void);
void Filter_state_check(void);
void PCU_Init(void);
void Timer0_Init(void);
void ADC0_Init(void);
void Time_PWM_Init(void);
void key_input(void);
void Key_exe(void);
void Pwm_off(void);
void Pwm_out(WORD pwm_data);
void Key_mode_dec(WORD bMode);
void Key_mode_inc(WORD bMode);
void Key_action(void);
void Flow_stop(void);
void Ph_value_updn(BYTE select_mode);
void Ph_value_con(BYTE select_mode);
void Orp_value_updn(BYTE select_mode);
void Orp_value_con(BYTE select_mode);
void Led_control(void);
void Voice_output(BYTE  Voice_val);
void Voice_real_output(void)	;
void Warning_voice(void);
void delay_1ms(WORD delay);
void Output_control(void);
void Flow_in_check(void);

void Filter_life_reload(void);
void F1_life_reload(void);
void F2_life_reload(void);
void Filter_life_check(void)   ;
void Filter_life_save(void);
void Rb_Load(void);
void Rb_Sync_Buffer(void);
BYTE Rb_Check(BYTE ch);
void Rb_Clear(BYTE ch);

/* 필터 칩 쓰기 치구 (FILTER_WRITER 1 에서만 동작) */
void Filter_Write_Exe(void);
BYTE Filter_Write_1(ULONG v);
BYTE Filter_Write_2(ULONG v);
BYTE Fw_ErrCode(int8_t e);
void Fw_Uid_Read(void);
void Current_pid(void);
void Cal_ion_i(void);


void Set_eep_init(void);
void Set_value_call(BYTE bMode);
void Set_value_load(void);
void Set_value_save(void);
void Auto_clean(void);
void Set_value_temp(void);
void Voice_delay_output(BYTE Voice_val);	
void Set_Eep_Exe(void);

void TouchInput(void);
void TouchReg_Init(void);
void fctest(void);


void HT16K33_Init(void);

void Flow_Hot_Pid(void);
void Cool_Control(void);
void Door_Check(void);

void Heater_Control(void);
void Uvc_Control(void);
void fcError(void);
float fast_sqrt(float x) ;

void Heater_Current(void);
 extern void fcDisplay(void);
 
 void Set_Hardware_Volume(BYTE level) ;
 
#endif
