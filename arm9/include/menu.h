#ifndef MENU_H
#define MENU_H

#include <nds/ndstypes.h>

#define ITEMS_PER_PAGE 20

typedef struct {
	bool directory;
	char* label;
	char* value;
} Item;

typedef struct {
	int cursor;
	int page;
	int itemCount;
	bool nextPage;
	int changePage;
	char header[32];
	Item items[ITEMS_PER_PAGE];
} Menu;

Menu* newMenu();
void freeMenu(Menu* m);

void addMenuItem(Menu* m, char const* label, char const* value, bool directory);
void sortMenuItems(Menu* m);
void setMenuHeader(Menu* m, char* str);

void resetMenu(Menu* m);
void clearMenu(Menu* m);
void printMenu(Menu* m);

bool moveCursor(Menu* m);

#endif