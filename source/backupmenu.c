#include "install.h"
#include "main.h"
#include "menu.h"
#include "rom.h"
#include "storage.h"
#include "message.h"
#include "nandio.h"
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

enum {
	BACKUP_MENU_RESTORE,
	BACKUP_MENU_DELETE,
	BACKUP_MENU_BACK
};

static void generateList(Menu* m);
static void printItem(Menu* m);
static int subMenu();
static bool delete(Menu* m);

void backupMenu()
{
	clearScreen(&topScreen);

	Menu* m = newMenu();
	setMenuHeader(m, "BACKUP MENU");
	generateList(m);

	//no files found
	if (m->itemCount <= 0)
	{
		messageBox("\x1B[33mNo backups found.\n\x1B[47m");
	}
	else
	{
		while (pmMainLoop())
		{
			swiWaitForVBlank();
			scanKeys();

			if (moveCursor(m))
			{
				if (m->changePage != 0)
					generateList(m);

				printMenu(m);
				printItem(m);
			}

			if (keysDown() & KEY_B || m->itemCount <= 0)
				break;

			else if (keysDown() & KEY_A)
			{
				switch (subMenu())
				{
					case BACKUP_MENU_RESTORE:
						install(m->items[m->cursor].value, false);
						break;

					case BACKUP_MENU_DELETE:
					{
						if (delete(m))
						{
							resetMenu(m);
							generateList(m);
						}
					}
					break;
				}

				printMenu(m);
			}
		}
	}

	freeMenu(m);
}

static int subMenu()
{
	int result = -1;

	Menu* m = newMenu();

	addMenuItem(m, "Restore", NULL, 0);
	addMenuItem(m, "Delete", NULL, 0);
	addMenuItem(m, "Back - [B]", NULL, 0);

	printMenu(m);

	while (pmMainLoop())
	{
		swiWaitForVBlank();
		scanKeys();

		if (moveCursor(m))
			printMenu(m);

		if (keysDown() & KEY_B)
			break;

		else if (keysDown() & KEY_A)
		{
			result = m->cursor;
			break;
		}
	}

	freeMenu(m);
	return result;
}

static void generateList(Menu* m)
{
	if (!m) return;

	//reset menu
	clearMenu(m);

	m->page += sign(m->changePage);
	m->changePage = 0;

	bool done = false;

	struct dirent* ent;
	DIR* dir = opendir(BACKUP_PATH);

	if (dir)
	{
		int count = 0;

		while ( (ent = readdir(dir)) && !done)
		{
			if (ent->d_name[0] == '.')
				continue;

			if (ent->d_type == DT_DIR)
			{
				if (count < m->page * ITEMS_PER_PAGE)
						count += 1;

				else
				{
					if (m->itemCount >= ITEMS_PER_PAGE)
						done = true;

					else
					{
						char* fpath = (char*)malloc(strlen(BACKUP_PATH) + strlen(ent->d_name) + 8);
						sprintf(fpath, "%s/%s", BACKUP_PATH, ent->d_name);

						addMenuItem(m, ent->d_name, fpath, 1);
					}
				}
			}
			else
			{
				if (strcasecmp(strrchr(ent->d_name, '.'), ".nds") == 0 ||
					strcasecmp(strrchr(ent->d_name, '.'), ".app") == 0 ||
					strcasecmp(strrchr(ent->d_name, '.'), ".dsi") == 0 ||
					strcasecmp(strrchr(ent->d_name, '.'), ".ids") == 0 ||
					strcasecmp(strrchr(ent->d_name, '.'), ".srl") == 0 ||
					strcasecmp(strrchr(ent->d_name, '.'), ".cia") == 0)
				{
					if (count < m->page * ITEMS_PER_PAGE)
						count += 1;

					else
					{
						if (m->itemCount >= ITEMS_PER_PAGE)
							done = true;

						else
						{
							char* fpath = (char*)malloc(strlen(BACKUP_PATH) + strlen(ent->d_name) + 8);
							sprintf(fpath, "%s/%s", BACKUP_PATH, ent->d_name);

							addMenuItem(m, ent->d_name, fpath, 0);

							free(fpath);
						}
					}
				}
			}
		}
	}

	closedir(dir);

	sortMenuItems(m);

	m->nextPage = done;

	if (m->cursor >= m->itemCount)
		m->cursor = m->itemCount - 1;

	printItem(m);
	printMenu(m);
}

static void printItem(Menu* m)
{
	if (!m) return;
	if (m->itemCount <= 0) return;

	if (m->items[m->cursor].directory)
		clearScreen(&topScreen);
	else
		printRomInfo(m->items[m->cursor].value);
}

static bool delete(Menu* m)
{
	if (!m) return false;

	char* label = m->items[m->cursor].label;
	char* fpath = m->items[m->cursor].value;

	bool result = false;
	bool choice = NO;
	{
		const char str[] = "Are you sure you want to delete\n";
		char* msg = (char*)malloc(strlen(str) + strlen(label) + 2);
		sprintf(msg, "%s%s?", str, label);

		choice = choiceBox(msg);

		free(msg);
	}

	if (choice == YES)
	{
		if (!fpath)
		{
			messageBox("\x1B[31mFailed to delete backup.\n\x1B[47m");
		}
		else
		{
			if (access(fpath, F_OK) != 0)
			{
				messageBox("\x1B[31mFailed to delete backup.\n\x1B[47m");
			}
			else
			{
				clearScreen(&bottomScreen);

				//app
				remove(fpath);

				//tmd
				strcpy(strrchr(fpath, '.'), ".tmd");
				remove(fpath);

				//public save
				strcpy(strrchr(fpath, '.'), ".pub");
				remove(fpath);

				//private save
				strcpy(strrchr(fpath, '.'), ".prv");
				remove(fpath);

				//banner save
				strcpy(strrchr(fpath, '.'), ".bnr");
				remove(fpath);

				result = true;
				messagePrint("\x1B[42m\nBackup deleted.\n\x1B[47m");
			}
		}
	}

	return result;
}