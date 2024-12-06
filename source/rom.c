#include "rom.h"
#include "main.h"
#include "storage.h"
#include <dirent.h>
#include <nds.h>
#include <malloc.h>
#include <stdio.h>
#include <unistd.h>

tDSiHeader* getRomHeader(char const* fpath)
{
	if (!fpath) return NULL;

	tDSiHeader* h = NULL;
	FILE* f = fopen(fpath, "rb");

	if (f)
	{
		h = (tDSiHeader*)malloc(sizeof(tDSiHeader));

		if (h)
		{
			if (romIsCia(fpath))
				fseek(f, 0x3900, SEEK_SET);
			else
				fseek(f, 0, SEEK_SET);

			fread(h, sizeof(tDSiHeader), 1, f);
		}

		fclose(f);
	}

	return h;
}

tNDSBanner* getRomBanner(char const* fpath)
{
	if (!fpath) return NULL;

	tDSiHeader* h = getRomHeader(fpath);
	tNDSBanner* b = NULL;

	if (h)
	{
		FILE* f = fopen(fpath, "rb");

		if (f)
		{
			b = (tNDSBanner*)malloc(sizeof(tNDSBanner));

			if (b)
			{
				if (romIsCia(fpath))
					fseek(f, 0x3900, SEEK_SET);
				else
					fseek(f, 0, SEEK_SET);

				fseek(f, h->ndshdr.bannerOffset, SEEK_CUR);
				fread(b, sizeof(tNDSBanner), 1, f);
			}
		}

		free(h);
		fclose(f);
	}

	return b;
}

bool getGameTitle(tNDSBanner* b, char* out, bool full)
{
	if (!b) return false;
	if (!out) return false;

	//get system language
	int lang = PersonalData->language;

	//not japanese, chinese, or korean
	if (lang == 0 || lang >= 6)
		lang = 1;

	//read title
	u16 c;
	for (int i = 0; i < 128; i++)
	{
		c = b->titles[lang][i];

		//remove accents
		if (c == 0x00F3)
			c = 'o';

		if (c == 0x00E1)
			c = 'a';

		out[i] = (char)c;

		if (!full && out[i] == '\n')
		{
			out[i] = '\0';
			break;
		}
	}
	out[128] = '\0';

	return true;
}

bool getGameTitlePath(char const* fpath, char* out, bool full)
{
	if (!fpath) return false;
	if (!out) return false;

	tNDSBanner* b = getRomBanner(fpath);
	bool result = getGameTitle(b, out, full);

	free(b);
	return result;
}

bool getRomLabel(tDSiHeader* h, char* out)
{
	if (!h) return false;
	if (!out) return false;

	sprintf(out, "%.12s", h->ndshdr.gameTitle);

	return true;
}

bool getRomCode(tDSiHeader* h, char* out)
{
	if (!h) return false;
	if (!out) return false;

	sprintf(out, "%.4s", h->ndshdr.gameCode);

	return true;
}

void printRomInfo(char const* fpath)
{
	clearScreen(&topScreen);

	if (!fpath) return;

	tDSiHeader* h = getRomHeader(fpath);
	tNDSBanner* b = getRomBanner(fpath);

	if (!isDsiHeader(h))
	{
		iprintf("Could not read dsi header.\n");
	}
	else
	{
		if (!b)
		{
			iprintf("Could not read banner.\n");
		}
		else
		{
			//proper title
			{
				char gameTitle[128+1];
				getGameTitle(b, gameTitle, true);

				iprintf("%s\n\n", gameTitle);
			}

			//file size
			{
				iprintf("Size: ");
				unsigned long long romSize = getRomSize(fpath);
				printBytes(romSize);
				//size in blocks, rounded up
				iprintf(" (%lld blocks)\n", ((romSize / BYTES_PER_BLOCK) * BYTES_PER_BLOCK + BYTES_PER_BLOCK) / BYTES_PER_BLOCK);
			}

			iprintf("Label: %.12s\n", h->ndshdr.gameTitle);
			iprintf("Game Code: %.4s\n", h->ndshdr.gameCode);

			//system type
			{
				iprintf("Unit Code: ");

				switch (h->ndshdr.unitCode)
				{
					case 0:  iprintf("NDS");     break;
					case 2:  iprintf("NDS+DSi"); break;
					case 3:  iprintf("DSi");     break;
					default: iprintf("unknown");
				}

				iprintf("\n");
			}

			//application type
			{
				iprintf("Program Type: ");

				switch (h->ndshdr.reserved1[7])
				{
					case 0x3: iprintf("Normal");    break;
					case 0xB: iprintf("Sys");       break;
					case 0xF: iprintf("Debug/Sys"); break;
					default:  iprintf("unknown");
				}

				iprintf("\n");
			}

			//DSi title ids
			{
				if (h->tid_high == 0x00030004 ||
					h->tid_high == 0x00030005 ||
					h->tid_high == 0x00030011 || // TID for software in TWL SDK
					h->tid_high == 0x00030015 ||
					h->tid_high == 0x00030017 ||
					h->tid_high == 0x00030000)
				{
					iprintf("Title ID: %08x %08x\n", (unsigned int)h->tid_high, (unsigned int)h->tid_low);
				}
			}

			//print full file path
			iprintf("\n%s\n", fpath);

			//print extra files
			int extensionPos = strrchr(fpath, '.') - fpath;
			char temp[PATH_MAX];
			strcpy(temp, fpath);
			strcpy(temp + extensionPos, ".tmd");
			//DSi TMDs are 520, TMDs from NUS are 2,312. If 2,312 we can simply trim it to 520
			int tmdSize = getFileSizePath(temp);
			if (access(temp, F_OK) == 0)
				printf("\t\x1B[%om%s\n\x1B[47m", (tmdSize == 520 || tmdSize == 2312) ? 047 : 041, strrchr(temp, '/') + 1);

			strcpy(temp + extensionPos, ".pub");
			if (access(temp, F_OK) == 0)
				printf("\t\x1B[%om%s\n\x1B[47m", (getFileSizePath(temp) == h->public_sav_size) ? 047 : 041, strrchr(temp, '/') + 1);

			strcpy(temp + extensionPos, ".prv");
			if (access(temp, F_OK) == 0)
				printf("\t\x1B[%om%s\n\x1B[47m", (getFileSizePath(temp) == h->private_sav_size) ? 047 : 041, strrchr(temp, '/') + 1);

			strcpy(temp + extensionPos, ".bnr");
			if (access(temp, F_OK) == 0)
				printf("\t\x1B[%om%s\n\x1B[47m", (getFileSizePath(temp) == 0x4000) ? 047 : 041, strrchr(temp, '/') + 1);
		}
	}

	free(b);
	free(h);
}

unsigned long long getRomSize(char const* fpath)
{
	if (!fpath) return 0;

	unsigned long long size = 0;
	FILE* f = fopen(fpath, "rb");

	if (f)
	{
		//cia
		if (romIsCia(fpath))
		{
			unsigned char bytes[4] = { 0 };
			fseek(f, 0x38D0, SEEK_SET);
			fread(bytes, 4, 1, f);
			size = (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | bytes[3];
		}
		else
		{
			fseek(f, 0, SEEK_END);
			size = ftell(f);
		}
	}

	fclose(f);
	return size;
}

bool romIsCia(char const* fpath)
{
	if (!fpath) return false;
	return (strstr(fpath, ".cia") != NULL || strstr(fpath, ".CIA") != NULL);
}

bool isDsiHeader(tDSiHeader* h)
{
	if (!h) return false;

	u16 crc16 = swiCRC16(0xFFFF, h, 0x15E);

	return h->ndshdr.headerCRC16 == crc16;
}