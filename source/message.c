#include "message.h"
#include "main.h"

void keyWait(u32 key)
{
	while (!programEnd)
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

	iprintf("\x1B[33m");	//yellow
	iprintf("%s\n", message);
	iprintf("\x1B[47m");	//white
	iprintf("\x1b[%d;0H\tYes\n\tNo\n", choiceRow);

	while (!programEnd)
	{
		swiWaitForVBlank();
		scanKeys();

		//Clear cursor
		iprintf("\x1b[%d;0H ", choiceRow + cursor);

		if (keysDown() & (KEY_UP | KEY_DOWN))
			cursor = !cursor;

		//Print cursor
		iprintf("\x1b[%d;0H>", choiceRow + cursor);

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

	iprintf("\x1B[33m");	//yellow
	iprintf("\n%s\n", message);
	iprintf("\x1B[47m");	//white
	iprintf("Yes - [A]\nNo  - [B]\n");

	while (!programEnd)
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

const static u16 keys[] = {KEY_UP, KEY_DOWN, KEY_RIGHT, KEY_LEFT, KEY_A, KEY_B, KEY_X, KEY_Y};
const static char *keysLabels[] = {"\x18", "\x19", "\x1A", "\x1B", "<A>", "<B>", "<X>", "<Y>"};

bool randomConfirmBox(char* message)
{
	const int choiceRow = 10;
	int sequencePosition = 0;

	u8 sequence[8];
	for (int i = 0; i < sizeof(sequence); i++)
	{
		sequence[i] = rand() % (sizeof(keys) / sizeof(keys[0]));
	}

	clearScreen(&bottomScreen);

	iprintf("\x1B[43m");	//yellow
	iprintf("%s\n", message);
	iprintf("\x1B[47m");	//white
	iprintf("\n<START> cancel\n");

	while (!programEnd && sequencePosition < sizeof(sequence))
	{
		swiWaitForVBlank();
		scanKeys();

		//Print sequence
		iprintf("\x1b[%d;0H", choiceRow);
		for (int i = 0; i < sizeof(sequence); i++)
		{
			iprintf("\x1B[%0om", i < sequencePosition ? 032 : 047);
			iprintf("%s ", keysLabels[sequence[i]]);
		}

		if (keysDown() & (KEY_UP | KEY_DOWN | KEY_RIGHT | KEY_LEFT | KEY_A | KEY_B | KEY_X | KEY_Y))
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
	iprintf("%s\n", message);
	iprintf("\nOkay - [A]\n");

	while (!programEnd)
	{
		swiWaitForVBlank();
		scanKeys();

		if (keysDown() & (KEY_A | KEY_B | KEY_START))
			break;
	}

	scanKeys();
}