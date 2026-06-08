#include "main.h"
#include "message.h"
#include "storage.h"

void testMenu()
{
	//top screen
	clearScreen(&topScreen);
	printf("Storage Check Test\n\n");

	//bottom screen
	clearScreen(&bottomScreen);

	unsigned int free = 0;
	unsigned int size = 0;

	//home menu slots
	{
		printf("Free Home Menu Slots:\n");

		free = getMenuSlotsFree();
		printf("\t%d / ", free);

		size = getMenuSlots();
		printf("%d\n", size);
	}

	//dsi menu
	{
		printf("\nFree DSi Menu Space:\n\t");

		free = getDsiFree();
		printBytes(free);
		printf(" / ");

		size = getDsiSize();
		printBytes(size);
		printf("\n");

		printf("\t%d / %d blocks\n", free / BYTES_PER_BLOCK, size / BYTES_PER_BLOCK);
	}

	//nand
	if (!sdnandMode)
	{
		printf("\nFree NAND Space:\n\t");

		free = getDsiRealFree();
		printBytes(free);
		printf(" / ");

		size = getDsiRealSize();
		printBytes(size);
		printf("\n");
	}

	//SD Card
	{
		printf("\nFree SD Space:\n\t");

		unsigned long long sdfree = getSDCardFree();
		printBytes(sdfree);
		printf(" / ");

		unsigned long long sdsize = getSDCardSize();
		printBytes(sdsize);
		printf("\n");

		printf("\t%d / %d blocks\n", (unsigned int)(sdfree / BYTES_PER_BLOCK), (unsigned int)(sdsize / BYTES_PER_BLOCK));
	}

	//end
	printf("\nBack - [B]\n");
	keyWait(KEY_B);
}