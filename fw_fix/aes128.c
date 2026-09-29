
#include "user_define.h"


extern ULONG filter1_life;
extern uint32_t test_data;
/* =========================
   Plaintext block format (16B)
   ========================= */
#define PT_MAGIC0   0xA5
#define PT_MAGIC1   0x5A
#define PT_VERSION  0x01
#define PT_TAIL     0xC3


/* =========================
   AES-128 implementation (ECB, 16B)
   ========================= */
#define AES_keyExpSize 176
//old
//static const uint8_t master_key1[16] = { 0x97, 0xf4, 0x20, 0xb5, 0x36, 0xe1, 0x88, 0xdd, 0xcc, 0x07, 0x74, 0xaf, 0xb3, 0x28, 0xee, 0x3b };
//static const uint8_t master_key2[16] = { 0x97, 0xf4, 0x20, 0xb5, 0x36, 0xe1, 0x88, 0xdd, 0xcc, 0x07, 0x74, 0xaf, 0xb3, 0x28, 0xee, 0x3b };
//new
static const uint8_t master_key1[16] = { 0x58, 0xC7, 0x3A, 0x92, 0xEB, 0x14, 0x6D, 0x8F, 0x03, 0x71, 0xA6, 0x29, 0xF4, 0x5C, 0xBE, 0x0D };
static const uint8_t master_key2[16] = { 0x9B, 0x4A, 0xF1, 0x08, 0x3C, 0x8D, 0x5E, 0x27, 0xAA, 0x1F, 0x65, 0xCB, 0x70, 0x93, 0xE2, 0x34 };

typedef struct {
    uint8_t RoundKey[AES_keyExpSize];
} AES128_ctx;


AES128_ctx ctx1, ctx2;

static const uint8_t sbox[256] = {
  0x63,0x7c,0x77,0x7b,0xf2,0x6b,0x6f,0xc5,0x30,0x01,0x67,0x2b,0xfe,0xd7,0xab,0x76,
  0xca,0x82,0xc9,0x7d,0xfa,0x59,0x47,0xf0,0xad,0xd4,0xa2,0xaf,0x9c,0xa4,0x72,0xc0,
  0xb7,0xfd,0x93,0x26,0x36,0x3f,0xf7,0xcc,0x34,0xa5,0xe5,0xf1,0x71,0xd8,0x31,0x15,
  0x04,0xc7,0x23,0xc3,0x18,0x96,0x05,0x9a,0x07,0x12,0x80,0xe2,0xeb,0x27,0xb2,0x75,
  0x09,0x83,0x2c,0x1a,0x1b,0x6e,0x5a,0xa0,0x52,0x3b,0xd6,0xb3,0x29,0xe3,0x2f,0x84,
  0x53,0xd1,0x00,0xed,0x20,0xfc,0xb1,0x5b,0x6a,0xcb,0xbe,0x39,0x4a,0x4c,0x58,0xcf,
  0xd0,0xef,0xaa,0xfb,0x43,0x4d,0x33,0x85,0x45,0xf9,0x02,0x7f,0x50,0x3c,0x9f,0xa8,
  0x51,0xa3,0x40,0x8f,0x92,0x9d,0x38,0xf5,0xbc,0xb6,0xda,0x21,0x10,0xff,0xf3,0xd2,
  0xcd,0x0c,0x13,0xec,0x5f,0x97,0x44,0x17,0xc4,0xa7,0x7e,0x3d,0x64,0x5d,0x19,0x73,
  0x60,0x81,0x4f,0xdc,0x22,0x2a,0x90,0x88,0x46,0xee,0xb8,0x14,0xde,0x5e,0x0b,0xdb,
  0xe0,0x32,0x3a,0x0a,0x49,0x06,0x24,0x5c,0xc2,0xd3,0xac,0x62,0x91,0x95,0xe4,0x79,
  0xe7,0xc8,0x37,0x6d,0x8d,0xd5,0x4e,0xa9,0x6c,0x56,0xf4,0xea,0x65,0x7a,0xae,0x08,
  0xba,0x78,0x25,0x2e,0x1c,0xa6,0xb4,0xc6,0xe8,0xdd,0x74,0x1f,0x4b,0xbd,0x8b,0x8a,
  0x70,0x3e,0xb5,0x66,0x48,0x03,0xf6,0x0e,0x61,0x35,0x57,0xb9,0x86,0xc1,0x1d,0x9e,
  0xe1,0xf8,0x98,0x11,0x69,0xd9,0x8e,0x94,0x9b,0x1e,0x87,0xe9,0xce,0x55,0x28,0xdf,
  0x8c,0xa1,0x89,0x0d,0xbf,0xe6,0x42,0x68,0x41,0x99,0x2d,0x0f,0xb0,0x54,0xbb,0x16
};

static const uint8_t rsbox[256] = {
  0x52,0x09,0x6a,0xd5,0x30,0x36,0xa5,0x38,0xbf,0x40,0xa3,0x9e,0x81,0xf3,0xd7,0xfb,
  0x7c,0xe3,0x39,0x82,0x9b,0x2f,0xff,0x87,0x34,0x8e,0x43,0x44,0xc4,0xde,0xe9,0xcb,
  0x54,0x7b,0x94,0x32,0xa6,0xc2,0x23,0x3d,0xee,0x4c,0x95,0x0b,0x42,0xfa,0xc3,0x4e,
  0x08,0x2e,0xa1,0x66,0x28,0xd9,0x24,0xb2,0x76,0x5b,0xa2,0x49,0x6d,0x8b,0xd1,0x25,
  0x72,0xf8,0xf6,0x64,0x86,0x68,0x98,0x16,0xd4,0xa4,0x5c,0xcc,0x5d,0x65,0xb6,0x92,
  0x6c,0x70,0x48,0x50,0xfd,0xed,0xb9,0xda,0x5e,0x15,0x46,0x57,0xa7,0x8d,0x9d,0x84,
  0x90,0xd8,0xab,0x00,0x8c,0xbc,0xd3,0x0a,0xf7,0xe4,0x58,0x05,0xb8,0xb3,0x45,0x06,
  0xd0,0x2c,0x1e,0x8f,0xca,0x3f,0x0f,0x02,0xc1,0xaf,0xbd,0x03,0x01,0x13,0x8a,0x6b,
  0x3a,0x91,0x11,0x41,0x4f,0x67,0xdc,0xea,0x97,0xf2,0xcf,0xce,0xf0,0xb4,0xe6,0x73,
  0x96,0xac,0x74,0x22,0xe7,0xad,0x35,0x85,0xe2,0xf9,0x37,0xe8,0x1c,0x75,0xdf,0x6e,
  0x47,0xf1,0x1a,0x71,0x1d,0x29,0xc5,0x89,0x6f,0xb7,0x62,0x0e,0xaa,0x18,0xbe,0x1b,
  0xfc,0x56,0x3e,0x4b,0xc6,0xd2,0x79,0x20,0x9a,0xdb,0xc0,0xfe,0x78,0xcd,0x5a,0xf4,
  0x1f,0xdd,0xa8,0x33,0x88,0x07,0xc7,0x31,0xb1,0x12,0x10,0x59,0x27,0x80,0xec,0x5f,
  0x60,0x51,0x7f,0xa9,0x19,0xb5,0x4a,0x0d,0x2d,0xe5,0x7a,0x9f,0x93,0xc9,0x9c,0xef,
  0xa0,0xe0,0x3b,0x4d,0xae,0x2a,0xf5,0xb0,0xc8,0xeb,0xbb,0x3c,0x83,0x53,0x99,0x61,
  0x17,0x2b,0x04,0x7e,0xba,0x77,0xd6,0x26,0xe1,0x69,0x14,0x63,0x55,0x21,0x0c,0x7d
};

static const uint8_t Rcon[11] = {0x00,0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80,0x1B,0x36};

uint32_t counter_next = 0;
uint32_t next_ttt = 0;
uint32_t chip_id1=0;
uint32_t chip_id2=0;

uint8_t SaveCipher1[16];
uint8_t SaveCipher2[16];

static uint8_t xtime(uint8_t x) { return (uint8_t)((x<<1) ^ ((x>>7) * 0x1B)); }
   


// =========================
//   CRC16-CCITT-FALSE
//   poly=0x1021 init=0xFFFF xorout=0
//   ========================= 
static uint16_t crc16_ccitt_false(const uint8_t *data, uint32_t len)
{
	uint32_t i ;
	uint8_t b;
    uint16_t crc = 0xFFFF;
    for ( i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for ( b = 0; b < 8; b++) {
            crc = (crc & 0x8000) ? (uint16_t)((crc << 1) ^ 0x1021) : (uint16_t)(crc << 1);
        }
    }
    return crc;
}
// 패킹 로직: uint32_t 사용
static void pack_value_counter(uint32_t value, uint32_t counter,uint32_t chip_id, uint8_t block[16], uint32_t max_val) {
	uint16_t crc;
    if (value > max_val) value = max_val;
    memset(block, 0, 16);
    block[0] = PT_MAGIC0; block[1] = PT_MAGIC1;
    
    // Value (4 bytes)
    block[2] = (uint8_t)(value & 0xFF);
    block[3] = (uint8_t)((value >> 8) & 0xFF);
    block[4] = (uint8_t)((value >> 16) & 0xFF);
    block[5] = (uint8_t)((value >> 24) & 0xFF);

    // Counter (4 bytes)
    block[6] = (uint8_t)(counter & 0xFF);
    block[7] = (uint8_t)((counter >> 8) & 0xFF);
    block[8] = (uint8_t)((counter >> 16) & 0xFF);
    block[9] = (uint8_t)((counter >> 24) & 0xFF);
	
	block[10] = (uint8_t)(chip_id & 0xFF);
    block[11] = (uint8_t)((chip_id >> 8) & 0xFF);
	block[12] = (uint8_t)((chip_id >> 16) & 0xFF);
	block[13] = (uint8_t)((chip_id >> 24) & 0xFF);
    
    crc = crc16_ccitt_false(block, 14);
    block[14] = (uint8_t)(crc & 0xFF);
    block[15] = (uint8_t)((crc >> 8) & 0xFF);

}

// 범용 언패킹 (32비트 대응)
static int unpack_value_counter(const uint8_t block[16], uint32_t *out_value, uint32_t *out_counter,
                                uint32_t max_val, uint32_t expect_id) {
	uint16_t crc_calc;
	uint16_t crc_stor;
	uint32_t id;
	
    if (block[0] != PT_MAGIC0 || block[1] != PT_MAGIC1) return -1;


    crc_calc = crc16_ccitt_false(block, 14);
    crc_stor = (uint16_t)block[14] | ((uint16_t)block[15] << 8);
    if (crc_calc != crc_stor) return -3;

    *out_value = (uint32_t)block[2] | ((uint32_t)block[3] << 8) | ((uint32_t)block[4] << 16) | ((uint32_t)block[5] << 24);
    if (*out_value > max_val) return -4;

	// chip_id 검증 : 다른 칩에 암호문만 복사한 경우 차단
	id  = (uint32_t)block[10] | ((uint32_t)block[11] << 8)
	    | ((uint32_t)block[12] << 16) | ((uint32_t)block[13] << 24);
	if (id != expect_id) return -5;

    *out_counter = (uint32_t)block[6] | ((uint32_t)block[7] << 8) | ((uint32_t)block[8] << 16) | ((uint32_t)block[9] << 24);
    return 0;
}

static int is_all_ff(const uint8_t b[16])
{
	int i;
    for ( i=0;i<16;i++) if (b[i] != 0xFF) return 0;
    return 1;
}

// 마지막 로드 결과 코드
//   0   정상
//  -1   키 불일치 (위조 / 다른 키로 쓴 칩)
//  -3   CRC 손상
//  -4   값 범위 초과
//  -5   chip_id 불일치 (암호문 복제)
// -11   빈 칩 (연결됨 , 데이터 없음 = 신품 / 소거 직후)
// -12   칩 미연결 / 하네스 불량
int8_t  last_load_err1 = 0;
int8_t  last_load_err2 = 0;

uint8_t bChipPresent1 = 0;      // 1 = 필터1 칩 연결됨
uint8_t bChipPresent2 = 0;      // 1 = 필터2 칩 연결됨

#if FILTER_WRITER
// 위조 칩 굽기 (에러 확인용) : 정상 펌웨어에는 포함되지 않음
//   0 = 정품   정상 마스터 키 + 정상 chip_id  -> 에러 없음
//   1 = 키위조 다른 마스터 키                 -> 정품 펌웨어에서 -1  -> E03 / E04
//   2 = ID위조 정상 키 + 틀린 chip_id         -> 정품 펌웨어에서 -5  -> E03 / E04
uint8_t bFwKeyMode = 0;

#define FW_FAKE_ID_XOR   0x5A5A5A5AUL

static const uint8_t fake_key1[16] = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00 };
static const uint8_t fake_key2[16] = { 0x0F, 0x1E, 0x2D, 0x3C, 0x4B, 0x5A, 0x69, 0x78, 0x87, 0x96, 0xA5, 0xB4, 0xC3, 0xD2, 0xE1, 0xF0 };
#endif

// 칩 존재 판정 : JEDEC ID (0x9F) 로 1차 확인
// 칩 미연결시 MISO 풀업으로 0xFF , 반단선시 0x00 이 읽힘
static int is_chip_absent(uint8_t mid, uint8_t mtype, uint8_t mcap)
{
	if( (mid == 0xFF) && (mtype == 0xFF) && (mcap == 0xFF) )	return 1;
	if( (mid == 0x00) && (mtype == 0x00) && (mcap == 0x00) )	return 1;
	
	return 0;
}



static void KeyExpansion(uint8_t* RoundKey, const uint8_t* Key)
{
    uint32_t i, j, k;
    uint8_t tempa[4];

    for (i = 0; i < 16; ++i) RoundKey[i] = Key[i];

    for (i = 4; i < 44; ++i) {
        k = (i - 1) * 4;
        tempa[0] = RoundKey[k + 0];
        tempa[1] = RoundKey[k + 1];
        tempa[2] = RoundKey[k + 2];
        tempa[3] = RoundKey[k + 3];

        if (i % 4 == 0) {
            uint8_t t = tempa[0];
            tempa[0] = tempa[1];
            tempa[1] = tempa[2];
            tempa[2] = tempa[3];
            tempa[3] = t;

            tempa[0] = sbox[tempa[0]];
            tempa[1] = sbox[tempa[1]];
            tempa[2] = sbox[tempa[2]];
            tempa[3] = sbox[tempa[3]];

            tempa[0] ^= Rcon[i/4];
        }

        j = i * 4;
        k = (i - 4) * 4;
        RoundKey[j + 0] = RoundKey[k + 0] ^ tempa[0];
        RoundKey[j + 1] = RoundKey[k + 1] ^ tempa[1];
        RoundKey[j + 2] = RoundKey[k + 2] ^ tempa[2];
        RoundKey[j + 3] = RoundKey[k + 3] ^ tempa[3];
    }
}

static void AddRoundKey(uint8_t round, uint8_t* state, const uint8_t* RoundKey)
{
	uint8_t i;
    for ( i = 0; i < 16; ++i) state[i] ^= RoundKey[(round * 16) + i];
}

static void SubBytes(uint8_t* state)     
 { 
	 uint8_t i;
	 for ( i=0;i<16;i++) state[i] = sbox[state[i]];
 }
static void InvSubBytes(uint8_t* state)  
 {
	 uint8_t i;
	for ( i=0;i<16;i++) state[i] = rsbox[state[i]]; 
 }

static void ShiftRows(uint8_t* s)
{
    uint8_t t;
    t=s[1];  s[1]=s[5];  s[5]=s[9];  s[9]=s[13]; s[13]=t;
    t=s[2];  s[2]=s[10]; s[10]=t;
    t=s[6];  s[6]=s[14]; s[14]=t;
    t=s[3];  s[3]=s[15]; s[15]=s[11]; s[11]=s[7]; s[7]=t;
}

static void InvShiftRows(uint8_t* s)
{
    uint8_t t;
    t=s[13]; s[13]=s[9]; s[9]=s[5]; s[5]=s[1]; s[1]=t;
    t=s[2];  s[2]=s[10]; s[10]=t;
    t=s[6];  s[6]=s[14]; s[14]=t;
    t=s[3];  s[3]=s[7]; s[7]=s[11]; s[11]=s[15]; s[15]=t;
}

static void MixColumns(uint8_t* s)
{
	uint8_t* c;
	uint8_t t ;
	uint8_t u;
	uint8_t i;
	
    for ( i=0;i<4;i++) {
        c = &s[i*4];
        t = c[0]^c[1]^c[2]^c[3];
        u = c[0];
        c[0] ^= t ^ xtime((uint8_t)(c[0]^c[1]));
        c[1] ^= t ^ xtime((uint8_t)(c[1]^c[2]));
        c[2] ^= t ^ xtime((uint8_t)(c[2]^c[3]));
        c[3] ^= t ^ xtime((uint8_t)(c[3]^u));
    }
}

static uint8_t Multiply(uint8_t x, uint8_t y)
{
    uint8_t r=0;
	
    while (y) {
        if (y & 1) r ^= x;
        x = xtime(x);
        y >>= 1;
    }
    return r;
}

static void InvMixColumns(uint8_t* s)
{
	uint8_t i;
	uint8_t* c;
	uint8_t a ,b,d,e;
	
    for ( i=0;i<4;i++) {
        c = &s[i*4];
        a=c[0], b=c[1], d=c[3], e=c[2];
        c[0] = Multiply(a,0x0e)^Multiply(b,0x0b)^Multiply(e,0x0d)^Multiply(d,0x09);
        c[1] = Multiply(a,0x09)^Multiply(b,0x0e)^Multiply(e,0x0b)^Multiply(d,0x0d);
        c[2] = Multiply(a,0x0d)^Multiply(b,0x09)^Multiply(e,0x0e)^Multiply(d,0x0b);
        c[3] = Multiply(a,0x0b)^Multiply(b,0x0d)^Multiply(e,0x09)^Multiply(d,0x0e);
    }
}

static void Cipher(uint8_t* state, const uint8_t* RoundKey)
{
	uint8_t round;
    AddRoundKey(0, state, RoundKey);
    for ( round=1; round<=9; round++) {
        SubBytes(state);
        ShiftRows(state);
        MixColumns(state);
        AddRoundKey(round, state, RoundKey);
    }
    SubBytes(state);
    ShiftRows(state);
    AddRoundKey(10, state, RoundKey);
}

static void InvCipher(uint8_t* state, const uint8_t* RoundKey)
{
	int8_t round;
	
    AddRoundKey(10, state, RoundKey);
    for ( round=9; round>=1; round--) {
        InvShiftRows(state);
        InvSubBytes(state);
        AddRoundKey((uint8_t)round, state, RoundKey);
        InvMixColumns(state);
    }
    InvShiftRows(state);
    InvSubBytes(state);
    AddRoundKey(0, state, RoundKey);
}

void AES128_init(AES128_ctx* ctx, const uint8_t key[16])
{
    KeyExpansion(ctx->RoundKey, key);
}

void AES128_ECB_encrypt(const AES128_ctx* ctx, const uint8_t input[16], uint8_t output[16])
{
    uint8_t s[16];
    memcpy(s, input, 16);
    Cipher(s, ctx->RoundKey);
    memcpy(output, s, 16);
}

void AES128_ECB_decrypt(const AES128_ctx* ctx, const uint8_t input[16], uint8_t output[16])
{
    uint8_t s[16];
    memcpy(s, input, 16);
    InvCipher(s, ctx->RoundKey);
    memcpy(output, s, 16);
}


int LoadValueGeneric(AES128_ctx* ctx, uint32_t addr, uint32_t* out_v, uint8_t channel) {
    uint8_t cipher[16], plain[16];
	uint32_t max_v ;
	uint32_t dummy_c;
	uint32_t chip_id_temp = (channel == 1) ? chip_id1 : chip_id2;
	uint8_t  present      = (channel == 1) ? bChipPresent1 : bChipPresent2;
	
	if(!present)	return -12;          // 칩 미연결 / 하네스 불량
	
    if(channel == 1) GD25D10_Read16Bytes(addr, cipher);
    else             GD25D10_2_Read16Bytes(addr, cipher);

    if (is_all_ff(cipher)) return -11;   // 빈 칩 / 신품
    AES128_ECB_decrypt(ctx, cipher, plain);
    
    max_v = (channel == 1) ? F1_MAX_LIFE : F2_MAX_LIFE;
   
    return unpack_value_counter(plain, out_v, &dummy_c, max_v, chip_id_temp);
}

int PrepareValue(AES128_ctx* ctx, uint32_t addr, uint32_t value, uint8_t channel, uint8_t* out_cipher) {
    uint8_t cipher[16], plain[16], block[16];
    uint32_t old_v, old_c;
	uint32_t next_c = 0;
	uint32_t chip_id_temp = (channel == 1) ? chip_id1 : chip_id2;
    uint32_t max_v = (channel == 1) ? F1_MAX_LIFE : F2_MAX_LIFE;
	uint8_t  present = (channel == 1) ? bChipPresent1 : bChipPresent2;
	
	if(!present)	return -12;          // 칩 미연결시 암호문 생성 금지
	
	next_c=0;
    if(channel == 1) GD25D10_Read16Bytes(addr, cipher);
    else             GD25D10_2_Read16Bytes(addr, cipher);

    if (!is_all_ff(cipher)) {
        AES128_ECB_decrypt(ctx, cipher, plain);
        if (unpack_value_counter(plain, &old_v, &old_c, max_v, chip_id_temp) == 0) {
            next_c = old_c + 1;
        }
    }
   

    pack_value_counter(value, next_c,chip_id_temp, block, max_v);
    AES128_ECB_encrypt(ctx, block, out_cipher);
    return 0;
}

void LoadFilter_Init(void)
{
	// chip_id 는 LoadFilter1_Init / LoadFilter2_Init 에서 칩 고유 UID 로부터 유도
	// 평문 영역(ADDR_F1_ID / ADDR_F2_ID) 읽기는 사용하지 않음
}
void LoadFilter1_Init(void)
{
	uint8_t uid1[16];
	uint8_t derived_key1[16];
	uint8_t mid, mtype, mcap;
	AES128_ctx temp_ctx;

	// 1차 : JEDEC ID 로 칩 존재 확인
	GD25D10_ReadID(&mid, &mtype, &mcap);
	if(is_chip_absent(mid, mtype, mcap)){
		bChipPresent1 = 0;
		chip_id1 = 0;
		return;                          // 키 유도 안 함 (엉뚱한 키 생성 방지)
	}
	bChipPresent1 = 1;

	// 채널 1 : UID 읽기
	GD25D10_Read16Bytes_UID(0, uid1); 

	// 키 유도 : AES(master_key1, uid1)
#if FILTER_WRITER
	if(bFwKeyMode == 1)	AES128_init(&temp_ctx, fake_key1);   // 키위조 : 다른 마스터 키
	else				AES128_init(&temp_ctx, master_key1);
#else
	AES128_init(&temp_ctx, master_key1);
#endif
	AES128_ECB_encrypt(&temp_ctx, uid1, derived_key1);

	// 각 컨텍스트 초기화
	AES128_init(&ctx1, derived_key1);

	// chip_id : 평문 영역이 아니라 칩 고유 UID 에서 유도 (복제 차단)
	chip_id1  = (uint32_t)uid1[0]
	          | ((uint32_t)uid1[1] << 8)
	          | ((uint32_t)uid1[2] << 16)
	          | ((uint32_t)uid1[3] << 24);

#if FILTER_WRITER
	if(bFwKeyMode == 2)	chip_id1 ^= FW_FAKE_ID_XOR;          // ID위조 : 틀린 chip_id
#endif
}

void LoadFilter2_Init(void)
{
	uint8_t uid2[16];
	uint8_t derived_key2[16];
	uint8_t mid, mtype, mcap;
	AES128_ctx temp_ctx;

	// 1차 : JEDEC ID 로 칩 존재 확인
	GD25D10_2_ReadID(&mid, &mtype, &mcap);
	if(is_chip_absent(mid, mtype, mcap)){
		bChipPresent2 = 0;
		chip_id2 = 0;
		return;
	}
	bChipPresent2 = 1;

	// 채널 2 : UID 읽기
	GD25D10_2_Read16Bytes_UID(0, uid2); 

#if FILTER_WRITER
	if(bFwKeyMode == 1)	AES128_init(&temp_ctx, fake_key2);   // 키위조 : 다른 마스터 키
	else				AES128_init(&temp_ctx, master_key2);
#else
	AES128_init(&temp_ctx, master_key2);
#endif
	AES128_ECB_encrypt(&temp_ctx, uid2, derived_key2);

	AES128_init(&ctx2, derived_key2);

	chip_id2  = (uint32_t)uid2[0]
	          | ((uint32_t)uid2[1] << 8)
	          | ((uint32_t)uid2[2] << 16)
	          | ((uint32_t)uid2[3] << 24);

#if FILTER_WRITER
	if(bFwKeyMode == 2)	chip_id2 ^= FW_FAKE_ID_XOR;          // ID위조 : 틀린 chip_id
#endif
}

 void  Aes128Init(void) 
{
	// 키 유도는 LoadFilter1_Init / LoadFilter2_Init 과 동일한 AES 방식으로 통일
	LoadFilter1_Init();
	LoadFilter2_Init();
 }
void ReadCnt(uint32_t addr, uint32_t data) {
    PrepareValue(&ctx1, addr, data, 1, SaveCipher1);
  
}
 
 // 채널 1 저장
void SaveFilter(uint32_t addr, uint32_t data) {
  //  PrepareValue(&ctx1, addr, data, 1, SaveCipher1);
	uint8_t i;
	uint8_t save_data[20];
	
	save_data[0] = chip_id1&0xff;
	save_data[1] =( chip_id1>>8)&0xff;
	save_data[2] =( chip_id1>>16)&0xff;
	save_data[3] =( chip_id1>>24)&0xff;
	for(i=0;i<16;i++)	save_data[i+4] = SaveCipher1[i];
	
	
    GD25D10_ByteWrite(addr, save_data);
}

void ReadCnt_2(uint32_t addr, uint32_t data) {
    PrepareValue(&ctx2, addr, data, 2, SaveCipher2);
 
}
// 채널 2 저장
void SaveFilter_2(uint32_t addr, uint32_t data) {
  //  PrepareValue(&ctx2, addr, data, 2, SaveCipher2);
	
	uint8_t i;
	uint8_t save_data[20];
	
	save_data[0] = chip_id2&0xff;
	save_data[1] =( chip_id2>>8)&0xff;
	save_data[2] =( chip_id2>>16)&0xff;
	save_data[3] =( chip_id2>>24)&0xff;
	for(i=0;i<16;i++)	save_data[i+4] = SaveCipher2[i];
	
    GD25D10_2_ByteWrite(addr, save_data);
}
 
// 채널 1 로드
uint32_t  LoadFilter(uint32_t addr) {
    uint32_t v = 0;
	int r;
	
	r = LoadValueGeneric(&ctx1, addr, &v, 1);
	
	last_load_err1 = (int8_t)r;             // 실패 원인 보존
	
	if(r==0)	return  v;
	else		return  0;	//error
}

// 채널 2 로드
uint32_t LoadFilter_2(uint32_t addr) {
    uint32_t v = 0;
	int r;
	
	r = LoadValueGeneric(&ctx2, addr, &v, 2);
	
	last_load_err2 = (int8_t)r;             // 실패 원인 보존
	
	if(r==0)	return  v;
	else		return  0;	//error
}

