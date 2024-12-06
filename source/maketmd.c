/*---------------------------------------------------------------------------------

maketmd.cpp -- TMD Creator for DSiWare Homebrew

Copyright (C) 2018
 Przemyslaw Skryjomski (Tuxality)

Big thanks to:
 Apache Thunder

This software is provided 'as-is', without any express or implied
warranty.  In no event will the authors be held liable for any
damages arising from the use of this software.

Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:

1.	The origin of this software must not be misrepresented; you
must not claim that you wrote the original software. If you use
this software in a product, an acknowledgment in the product
documentation would be appreciated but is not required.

2.	Altered source versions must be plainly marked as such, and
must not be misrepresented as being the original software.

3.	This notice may not be removed or altered from any source
distribution.

---------------------------------------------------------------------------------*/

/*	September 2018 - Jeff - Translated from C++ to C and uses libnds instead of openssl
	Original: github.com/Tuxality/maketmd
*/

#include "maketmd.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <nds/sha1.h>
#include <nds/ndstypes.h>
#include <machine/endian.h>

//#define TMD_CREATOR_VER  "0.2"

#define TMD_SIZE          0x208
#define SHA_BUFFER_SIZE   0x200
#define SHA_DIGEST_LENGTH 0x14

void tmd_create(uint8_t* tmd, FILE* app)
{
	// Phase 1 - offset 0x18C (Title ID, first part)
	{
		fseek(app, 0x234, SEEK_SET);

		uint32_t value;
		fread(&value, 4, 1, app);
		value = __bswap32(value);

		memcpy(tmd + 0x18c, &value, 4);
	}

	// Phase 2 - offset 0x190 (Title ID, second part)
	{
		// We can take this also from 0x230, but reversed
		fseek(app, 0x0C, SEEK_SET);
		fread((char*)&tmd[0x190], 4, 1, app);
	}

	// Phase 3 - offset 0x198 (Group ID = '01')
	{
		fseek(app, 0x10, SEEK_SET);
		fread((char*)&tmd[0x198], 2, 1, app);
	}

	// Phase 4 - offset 0x1AA (fill-in 0x80 value, 0x10 times)
	{
		for (size_t i = 0; i<0x10; i++)
		{
			tmd[0x1AA + i] = 0x80;
		}
	}

	// Phase 5 - offset 0x1DE (number of contents = 1)
	{
		tmd[0x1DE] = 0x00;
		tmd[0x1DF] = 0x01;
	}

	// Phase 6 - offset 0x1EA (type of content = 1)
	{
		tmd[0x1EA] = 0x00;
		tmd[0x1EB] = 0x01;
	}

	// Phase 7 - offset, 0x1EC (file size, 8B)
	uint32_t filesize = 0;
	uint32_t fileread = 0;
	{
		fseek(app, 0, SEEK_END);
		filesize = ftell(app);
		uint32_t size = __bswap32(filesize);

		// We only use 4B for size as for now
		memcpy((tmd + 0x1F0), &size, sizeof(u32));
	}

	// Phase 8 - offset, 0x1F4 (SHA1 sum, 20B)
	{
		// Makes use of libnds
		fseek(app, 0, SEEK_SET);

		uint8_t buffer[SHA_BUFFER_SIZE] = { 0 };
		uint32_t buffer_read = 0;

		swiSHA1context_t ctx;
		swiSHA1Init(&ctx);

		do {
			buffer_read = fread((char*)&buffer[0], 1, SHA_BUFFER_SIZE, app);
			fileread += buffer_read;

			swiSHA1Update(&ctx, buffer, buffer_read);

			printProgressBar((float)fileread / (float)filesize);
		}
		while (buffer_read == SHA_BUFFER_SIZE);

		clearProgressBar();
		consoleSelect(&bottomScreen);

		swiSHA1Final(buffer, &ctx);

		//Store SHA1 sum
		memcpy((tmd + 0x1F4), buffer, SHA_DIGEST_LENGTH);
	}
}

int maketmd(char* input, char* tmdPath)
{
	iprintf("MakeTMD for DSiWare Homebrew\n");
	iprintf("by Przemyslaw Skryjomski\n\t(Tuxality)\n");

	if (input == NULL || tmdPath == NULL)
	{
		iprintf("\x1B[33m");	//yellow
		iprintf("\nUsage: %s file.app <file.tmd>\n", "maketmd");
		iprintf("\x1B[47m");	//white
		return 1;
	}

	// APP file (input)
	FILE* app = fopen(input, "rb");

	if (!app)
	{
		iprintf("\x1B[31m");	//red
		iprintf("Error at opening %s for reading.\n", input);
		iprintf("\x1B[47m");	//white
		return 1;
	}

	// TMD file (output)
	FILE* tmd = fopen(tmdPath, "wb");

	if (!tmd)
	{
		fclose(app);
		iprintf("\x1B[31m");	//white
		iprintf("Error at opening %s for writing.\n", tmdPath);
		iprintf("\x1B[47m");	//white
		return 1;
	}

	// Allocate memory for TMD
	uint8_t* tmd_template = (uint8_t*)malloc(sizeof(uint8_t) * TMD_SIZE);
	memset(tmd_template, 0, sizeof(uint8_t) * TMD_SIZE); // zeroed

	// Prepare TMD template then write to file
	tmd_create(tmd_template, app);
	fwrite((const char*)(&tmd_template[0]), TMD_SIZE, 1, tmd);

	// Free allocated memory for TMD
	free(tmd_template);

	// This is done in dtor, but we additionally flush tmd.
	fclose(app);
	fclose(tmd);

	return 0;
}