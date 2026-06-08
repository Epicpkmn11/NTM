#include "message.h"
#include "main.h"

void keyWait(u32 key)
{
	while (1)
	{
		swiWaitForVBlank();
		scanKeys();

		if (keysDown() & key)
			break;
	}
}

bool choiceBox(char* message)
{
	const int choiceRow = 10;
	int cursor = 0;

	clearScreen(&bottomScreen);

	printf("\x1B[33m");	//yellow
	printf("%s\n", message);
	printf("\x1B[47m");	//white
	printf("\x1b[%d;0H\tYes\n\tNo\n", choiceRow);

	while (1)
	{
		swiWaitForVBlank();
		scanKeys();

		//Clear cursor
		printf("\x1b[%d;0H ", choiceRow + cursor);

		if (keysDown() & (KEY_UP | KEY_DOWN))
			cursor = !cursor;

		//Print cursor
		printf("\x1b[%d;0H>", choiceRow + cursor);

		if (keysDown() & (KEY_A | KEY_START))
			break;

		if (keysDown() & KEY_B)
		{
			cursor = 1;
			break;
		}
	}

	scanKeys();
	return (cursor == 0)? YES: NO;
}

bool choicePrint(char* message)
{
	bool choice = NO;

	printf("\x1B[33m");	//yellow
	printf("\n%s\n", message);
	printf("\x1B[47m");	//white
	printf("Yes - [A]\nNo  - [B]\n");

	while (1)
	{
		swiWaitForVBlank();
		scanKeys();

		if (keysDown() & KEY_A)
		{
			choice = YES;
			break;
		}

		else if (keysDown() & KEY_B)
		{
			choice = NO;
			break;
		}
	}

	scanKeys();
	return choice;
}

const static u16 keys[] = {KEY_UP, KEY_DOWN, KEY_RIGHT, KEY_LEFT, KEY_A};
const static char *keysLabels[] = {"^", "v", ">", "<", "<A>"};

bool randomConfirmBox(char* message)
{
	const int choiceRow = 10;
	int sequencePosition = 0;

	u8 sequence[8];
	for (int i = 0; i < 7; i++)
	{
		sequence[i] = rand() % 4;
	}
	sequence[7] = 4;

	clearScreen(&bottomScreen);

	printf("\x1B[43m");	//yellow
	printf("%s\n", message);
	printf("\x1B[47m");	//white
	printf("\n<START> cancel\n");

	while (1 && sequencePosition < sizeof(sequence))
	{
		swiWaitForVBlank();
		scanKeys();

		//Print sequence
		printf("\x1b[%d;0H", choiceRow);
		for (int i = 0; i < sizeof(sequence); i++)
		{
			printf("\x1B[%0om", i < sequencePosition ? 032 : 047);
			printf("%s ", keysLabels[sequence[i]]);
		}

		if (keysDown() & (KEY_UP | KEY_DOWN | KEY_RIGHT | KEY_LEFT | KEY_A))
		{
			if (keysDown() & keys[sequence[sequencePosition]])
				sequencePosition++;
			else
				sequencePosition = 0;
		}

		if (keysDown() & KEY_START)
		{
			sequencePosition = 0;
			break;
		}
	}

	scanKeys();
	return sequencePosition == sizeof(sequence);
}

void messageBox(char* message)
{
	clearScreen(&bottomScreen);
	messagePrint(message);
}

void messagePrint(char* message)
{
	printf("%s\n", message);
	printf("\nOkay - [A]\n");

	while (1)
	{
		swiWaitForVBlank();
		scanKeys();

		if (keysDown() & (KEY_A | KEY_B | KEY_START))
			break;
	}

	scanKeys();
}