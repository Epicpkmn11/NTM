#pragma once

#include <nds.h>

#ifdef __cplusplus
extern "C" {
#endif

/************************ Constants / Defines *********************************/

#define GET_UINT32_BE(n, b, i) \
	((uint8_t*)&(n))[0] = (b)[i + 3]; \
	((uint8_t*)&(n))[1] = (b)[i + 2]; \
	((uint8_t*)&(n))[2] = (b)[i + 1]; \
	((uint8_t*)&(n))[3] = (b)[i + 0]

#define PUT_UINT32_BE(n, b, i) \
	(b)[i + 0] = ((uint8_t*)&(n))[3]; \
	(b)[i + 1] = ((uint8_t*)&(n))[2]; \
	(b)[i + 2] = ((uint8_t*)&(n))[1]; \
	(b)[i + 3] = ((uint8_t*)&(n))[0]


typedef enum {
	ENCRYPT,
	DECRYPT
} crypt_mode_t;

typedef enum {
	NAND,
	NAND_3DS,
	ES
} key_mode_t;


void dsi_crypt_init(const uint32_t *console_id);

int dsi_es_block_crypt(uint8_t *buf, unsigned buf_len, crypt_mode_t mode);

#ifdef __cplusplus
}
#endif
