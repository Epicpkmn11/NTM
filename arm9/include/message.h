#ifndef MESSAGE_H
#define MESSAGE_H

#include <nds/ndstypes.h>

enum {
	YES = true,
	NO = false
};

void keyWait(u32 key);
bool choiceBox(char* message);
bool choicePrint(char* message);
bool randomConfirmBox(char* message);
void messageBox(char* message);
void messagePrint(char* message);

#endif