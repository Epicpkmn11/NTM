#include "menu.h"
#include "main.h"

Menu* newMenu()
{
	Menu* m = (Menu*)malloc(sizeof(Menu));

	m->cursor = 0;
	m->page = 0;
	m->itemCount = 0;
	m->nextPage = false;
	m->changePage = 0;
	m->header[0] = '\0';

	for (int i = 0; i < ITEMS_PER_PAGE; i++)
	{
		m->items[i].directory = false;
		m->items[i].label = NULL;
		m->items[i].value = NULL;
	}

	return m;
}

void freeMenu(Menu* m)
{
	if (!m) return;

	clearMenu(m);

	free(m);
	m = NULL;
}

void addMenuItem(Menu* m, char const* label, char const* value, bool directory)
{
	if (!m) return;

	int i = m->itemCount;
	if (i >= ITEMS_PER_PAGE) return;

	m->items[i].directory = directory;

	if (label)
	{
		m->items[i].label = (char*)malloc(32);
		sprintf(m->items[i].label, "%.31s", label);
	}

	if (value)
	{
		m->items[i].value = (char*)malloc(strlen(value)+1);
		sprintf(m->items[i].value, "%s", value);
	}

	m->itemCount += 1;
}

static int alphabeticalCompare(const void* a, const void* b)
{
	const Item* itemA = (const Item*)a;
	const Item* itemB = (const Item*)b;

	if (itemA->directory && !itemB->directory)
		return -1;
	else if (!itemA->directory && itemB->directory)
		return 1;
	else
		return strcasecmp(itemA->label, itemB->label);
}

void sortMenuItems(Menu* m)
{
	qsort(m->items, m->itemCount, sizeof(Item), alphabeticalCompare);
}

void setMenuHeader(Menu* m, char* str)
{
	if (!m) return;

	if (!str)
	{
		m->header[0] = '\0';
		return;
	}

	char* strPtr = str;

	if (strlen(strPtr) > 30)
		strPtr = str + (strlen(strPtr) - 30);

	sprintf(m->header, "%.30s", strPtr);
}

void resetMenu(Menu* m)
{
	m->cursor = 0;
	m->page = 0;
	m->changePage = 0;
	m->nextPage = 0;
}

void clearMenu(Menu* m)
{
	if (!m) return;

	for (int i = 0; i < ITEMS_PER_PAGE; i++)
	{
		if (m->items[i].label)
		{
			free(m->items[i].label);
			m->items[i].label = NULL;
		}

		if (m->items[i].value)
		{
			free(m->items[i].value);
			m->items[i].value = NULL;
		}
	}

	m->itemCount = 0;
}

void printMenu(Menu* m)
{
	clearScreen(&bottomScreen);

	if (!m) return;

	//header
	iprintf("\x1B[42m");	//green
	iprintf("%.30s\n\n", m->header);
	iprintf("\x1B[47m");	//white

	if (m->itemCount <= 0)
	{
		iprintf("Back - [B]\n");
		return;
	}

	//items
	for (int i = 0; i < m->itemCount; i++)
	{
		if (m->items[i].label)
		{
			if (m->items[i].directory)
				iprintf(" [%.28s]\n", m->items[i].label);
			else
				iprintf(" %.30s\n", m->items[i].label);
		}
		else
			iprintf(" \n");
	}

	//cursor
	iprintf("\x1b[%d;0H>", 2 + m->cursor);

	//scroll arrows
	if (m->page > 0)
		iprintf("\x1b[2;31H^");

	if (m->nextPage)
		iprintf("\x1b[21;31Hv");
}

static void _moveCursor(Menu* m, int dir)
{
	if (m->changePage != 0)
		return;

	m->cursor += sign(dir);

	if (m->cursor < 0)
	{
		if (m->page <= 0)
			m->cursor = 0;
		else
		{
			m->changePage = -1;
			m->cursor = ITEMS_PER_PAGE - 1;
		}
	}

	else if (m->cursor > m->itemCount-1)
	{
		if (m->nextPage && m->cursor >= ITEMS_PER_PAGE)
		{
			m->changePage = 1;
			m->cursor = 0;
		}
		else
		{
			m->cursor = m->itemCount-1;
		}
	}
}

bool moveCursor(Menu* m)
{
	if (!m) return false;

	m->changePage = 0;
	int lastCursor = m->cursor;

	u32 down = keysDownRepeat();

	if (down & KEY_DOWN)
		_moveCursor(m, 1);

	else if (down & KEY_UP)
		_moveCursor(m, -1);

	if (down & KEY_RIGHT)
	{
		repeat(10)
			_moveCursor(m, 1);
	}

	else if (down & KEY_LEFT)
	{
		repeat(10)
			_moveCursor(m, -1);
	}

	return !(lastCursor == m->cursor);
}