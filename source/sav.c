#include "sav.h"
#include <string.h>
#include <malloc.h>

#define align(v, a) (((v) % (a)) ? ((v) + (a) - ((v) % (a))) : (v))

bool initFatHeader(FILE* f)
{
	if (!f)
		return false;

	//get size
	fseek(f, 0, SEEK_END);
	u32 size = ftell(f);

	//based on GodMode9
	//https://github.com/d0k3/GodMode9/blob/d8d43c14f3317423c677b1c7e0987bcb9bbd7299/arm9/source/game/nds.c#L47-L105
	const u16 sectorSize = 0x200;

	//fit maximum sectors for the size
	const u16 maxSectors = size / sectorSize;
	u16 sectorCount = 1;
	u16 secPerTrk = 1;
	u16 numHeads = 1;
	u16 sectorCountNext = 0;
	while (sectorCountNext <= maxSectors)
	{
		sectorCountNext = secPerTrk * (numHeads + 1) * (numHeads + 1);
		if (sectorCountNext <= maxSectors)
		{
			numHeads++;
			sectorCount = sectorCountNext;

			secPerTrk++;
			sectorCountNext = secPerTrk * numHeads * numHeads;
			if (sectorCountNext <= maxSectors)
			{
				sectorCount = sectorCountNext;
			}
		}
	}
	sectorCountNext = (secPerTrk + 1) * numHeads * numHeads;
	if (sectorCountNext <= maxSectors)
	{
		secPerTrk++;
		sectorCount = sectorCountNext;
	}

	u8 secPerCluster = (sectorCount > (8 << 10)) ? 8 : (sectorCount > (1 << 10) ? 4 : 1);

	u16 rootEntryCount = size < 0x8C000 ? 0x20 : 0x200;

	u16 totalClusters = align(sectorCount, secPerCluster) / secPerCluster;
	u32 fatBytes = (align(totalClusters, 2) / 2) * 3; // 2 sectors -> 3 byte
	u16 fatSize = align(fatBytes, sectorSize) / sectorSize;


	FATHeader* h = (FATHeader*)malloc(sizeof(FATHeader));

	h->BS_JmpBoot[0] = 0xE9;
	h->BS_JmpBoot[1] = 0;
	h->BS_JmpBoot[2] = 0;

	memcpy(h->BS_OEMName, "MSWIN4.1", 8);

	h->BPB_BytesPerSec = sectorSize;
	h->BPB_SecPerClus = secPerCluster;
	h->BPB_RsvdSecCnt = 0x0001;
	h->BPB_NumFATs = 0x02;
	h->BPB_RootEntCnt = rootEntryCount;
	h->BPB_TotSec16 = sectorCount;
	h->BPB_Media = 0xF8; // "hard drive"
	h->BPB_FATSz16 = fatSize;
	h->BPB_SecPerTrk = secPerTrk;
	h->BPB_NumHeads = numHeads;
	h->BPB_HiddSec = 0x00000000;
	h->BPB_TotSec32 = 0x00000000;
	h->BS_DrvNum = 0x05;
	h->BS_Reserved1 = 0x00;
	h->BS_BootSig = 0x29;
	h->BS_VolID = 0x12345678;
	memcpy(h->BS_VolLab, "VOLUMELABEL", 11);
	memcpy(h->BS_FilSysType,"FAT12   ", 8);
	memset(h->BS_BootCode, 0, sizeof(h->BS_BootCode));
	h->BS_BootSign = 0xAA55;

	fseek(f, 0, SEEK_SET);
	fwrite(h, sizeof(FATHeader), 1, f);

	free(h);

	return true;
}