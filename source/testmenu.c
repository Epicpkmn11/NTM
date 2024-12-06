#include "main.h"
#include "message.h"
#include "storage.h"

void testMenu()
{
	//top screen
	clearScreen(&topScreen);
	iprintf("Storage Check Test\n\n");

	//bottom screen
	clearScreen(&bottomScreen);

	unsigned int free = 0;
	unsigned int size = 0;

	//home menu slots
	{
		iprintf("Free Home Menu Slots:\n");

		free = getMenuSlotsFree();
		iprintf("\t%d / ", free);

		size = getMenuSlots();
		iprintf("%d\n", size);
	}

	//dsi menu
	{
		iprintf("\nFree DSi Menu Space:\n\t");

		free = getDsiFree();
		printBytes(free);
		iprintf(" / ");

		size = getDsiSize();
		printBytes(size);
		iprintf("\n");

		iprintf("\t%d / %d blocks\n", free / BYTES_PER_BLOCK, size / BYTES_PER_BLOCK);
	}

	//nand
	if (!sdnandMode)
	{
		iprintf("\nFree NAND Space:\n\t");

		free = getDsiRealFree();
		printBytes(free);
		iprintf(" / ");

		size = getDsiRealSize();
		printBytes(size);
		iprintf("\n");
	}

	//SD Card
	{
		iprintf("\nFree SD Space:\n\t");

		unsigned long long sdfree = getSDCardFree();
		printBytes(sdfree);
		iprintf(" / ");

		unsigned long long sdsize = getSDCardSize();
		printBytes(sdsize);
		iprintf("\n");

		printf("\t%d / %d blocks\n", (unsigned int)(sdfree / BYTES_PER_BLOCK), (unsigned int)(sdsize / BYTES_PER_BLOCK));
	}

	//end
	iprintf("\nBack - [B]\n");
	keyWait(KEY_B);
}