#ifndef MAIN_H
#define MAIN_H

#include <nds.h>
#include <fat.h>
#include <stdio.h>

extern bool programEnd;
extern bool sdnandMode;
extern bool unlaunchFound;
extern bool unlaunchPatches;
extern u8 region;

void installMenu();
void titleMenu();
void backupMenu();
void testMenu();

extern PrintConsole topScreen;
extern PrintConsole bottomScreen;

void clearScreen(PrintConsole* screen);

#define abs(X) ( (X) < 0 ? -(X): (X) )
#define sign(X) ( ((X) > 0) - ((X) < 0) )
#define repeat(X) for (int _I_ = 0; _I_ < (X); _I_++)

#endif