#include <stdint.h>
#include "crypto.h"
#include "u128_math.h"
#include "dsi.h"

// more info:
//		https://github.com/Jimmy-Z/TWLbf/blob/master/dsi.c
//		https://github.com/Jimmy-Z/bfCL/blob/master/dsi.h
// ported back to 32 bit for ARM9

static dsi_context nand_ctx;
static dsi_context boot2_ctx;
static dsi_es_context es_ctx;

static uint8_t nand_ctr_iv[16];
static uint8_t boot2_ctr[16];

int dsi_sha1_verify(const void *digest_verify, const void *data, unsigned len)
{
	uint8_t digest[SHA1_LEN];
	swiSHA1Calc(digest, data, len);
	return memcmp(digest, digest_verify, SHA1_LEN);
}

// crypt one block, in/out must be aligned to 32 bit(restriction induced by xor_128)
// offset as block offset, block as AES block
void dsi_nand_crypt_1(uint8_t* out, const uint8_t* in, uint32_t offset)
{
	uint8_t ctr[16];
	memcpy(ctr, nand_ctr_iv, sizeof(nand_ctr_iv));
	u128_add32(ctr, offset);
	dsi_set_ctr(&nand_ctx, ctr);
	dsi_crypt_ctr(&nand_ctx, in, out, 16);
}

void dsi_nand_crypt(uint8_t* out, const uint8_t* in, uint32_t offset, unsigned count)
{
	uint8_t ctr[16];
	memcpy(ctr, nand_ctr_iv, sizeof(nand_ctr_iv));
	u128_add32(ctr, offset);
	for (unsigned i = 0; i < count; ++i)
	{
		dsi_set_ctr(&nand_ctx, ctr);
		dsi_crypt_ctr(&nand_ctx, in, out, 16);
		out += AES_BLOCK_SIZE;
		in += AES_BLOCK_SIZE;
		u128_add32(ctr, 1);
	}
}

int dsi_es_block_crypt(uint8_t *buf, unsigned buf_len, crypt_mode_t mode)
{
	if (mode == DECRYPT)
		return dsi_es_decrypt(&es_ctx, buf, buf + buf_len - 0x20, buf_len - 0x20);
	else
		dsi_es_encrypt(&es_ctx, buf, buf + buf_len - 0x20, buf_len - 0x20);

	return 0;
}

void dsi_boot2_crypt_set_ctr(uint32_t size_r)
{
	for (int i=0;i<4;i++)
	{
		boot2_ctr[i] = (size_r) >> (8*i);
		boot2_ctr[i+4] = (-size_r) >> (8*i);
		boot2_ctr[i+8] = (~size_r) >> (8*i);
		boot2_ctr[i+12] = 0;
	}
}

void dsi_boot2_crypt(uint8_t* out, const uint8_t* in, unsigned count)
{
	for (unsigned i = 0; i < count; ++i)
	{
		dsi_set_ctr(&boot2_ctx, boot2_ctr);
		dsi_crypt_ctr(&boot2_ctx, in, out, 16);
		out += AES_BLOCK_SIZE;
		in += AES_BLOCK_SIZE;
		u128_add32(boot2_ctr, 1);
	}
}
