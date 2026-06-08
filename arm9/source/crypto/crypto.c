#include <stdint.h>
#include "crypto.h"
#include "u128_math.h"
#include "dsi.h"

/************************ Constants / Defines *********************************/

const uint8_t DSi_KEY_MAGIC[16] = {
	0x79, 0x3e, 0x4f, 0x1a, 0x5f, 0x0f, 0x68, 0x2a,
	0x58, 0x02, 0x59, 0x29, 0x4e, 0xfb, 0xfe, 0xff
};

const uint8_t DSi_ES_KEY_Y[16] = {
	0xe5, 0xcc, 0x5a, 0x8b, 0x56, 0xd0, 0xc9,0x72,
	0x9c, 0x17, 0xe8, 0xdc, 0x39, 0x12, 0x36, 0xa9
};

// more info:
//		https://github.com/Jimmy-Z/TWLbf/blob/master/dsi.c
//		https://github.com/Jimmy-Z/bfCL/blob/master/dsi.h
// ported back to 32 bit for ARM9

static dsi_es_context es_ctx;

void F_XY(uint8_t *key, const uint8_t *key_x, const uint8_t *key_y)
{
	uint8_t key_xy[16];

	for (int i=0; i<16; i++)
		key_xy[i] = key_x[i] ^ key_y[i];

	memcpy(key, DSi_KEY_MAGIC, sizeof(DSi_KEY_MAGIC));

	u128_add(key, key_xy);
	u128_lrot(key, 42);
}

void dsi_crypt_init(const uint32_t *console_id)
{
	u32 normalkey[4];
	u32 tadsrl_keyX[4] = {0x4E00004A, 0x4A00004E, 0, 0};
	tadsrl_keyX[2] = console_id[1] ^ 0xC80C4B72;
	tadsrl_keyX[3] = console_id[0];
	F_XY((u8 *)normalkey, (u8 *)tadsrl_keyX, DSi_ES_KEY_Y);
	dsi_es_init(&es_ctx, (u8*)normalkey);

}

int dsi_es_block_crypt(uint8_t *buf, unsigned buf_len, crypt_mode_t mode)
{
	if (mode == DECRYPT)
		return dsi_es_decrypt(&es_ctx, buf, buf + buf_len - 0x20, buf_len - 0x20);
	else
		dsi_es_encrypt(&es_ctx, buf, buf + buf_len - 0x20, buf_len - 0x20);

	return 0;
}

