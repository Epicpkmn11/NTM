#include "main.h"
#include "rom.h"
#include "menu.h"
#include "message.h"
#include "nandio.h"
#include "storage.h"
#include <dirent.h>
#include <unistd.h>

enum {
	TITLE_MENU_BACKUP,
	TITLE_MENU_DELETE,
	TITLE_MENU_READ_ONLY,
	TITLE_MENU_BACK
};

static bool readOnly = false;

static void generateList(Menu* m);
static void printItem(Menu* m);
static int subMenu();
static void backup(Menu* m);
static bool delete(Menu* m);
static void toggleReadOnly(Menu* m);

void titleMenu()
{
	Menu* m = newMenu();
	setMenuHeader(m, "INSTALLED TITLES");
	generateList(m);

	//no titles
	if (m->itemCount <= 0)
	{
		messageBox("No titles found.");
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
				readOnly = FAT_getAttr(m->items[m->cursor].value) & ATTR_READONLY;

				switch (subMenu())
				{
					case TITLE_MENU_BACKUP:
						backup(m);
						break;

					case TITLE_MENU_DELETE:
					{
						if (delete(m))
						{
							resetMenu(m);
							generateList(m);
						}
					}
					break;
					case TITLE_MENU_READ_ONLY:
						toggleReadOnly(m);
						break;
				}

				printMenu(m);
			}
		}
	}

	freeMenu(m);
}

static void generateList(Menu* m)
{
	if (!m) return;

	const int NUM_OF_DIRS = 4;
	const char* dirs[] = {
		"00030004",
		"00030005",
		"00030015",
		"00030017"
	};

	const char* blacklist[4][6] = {
		{ // 00030004
			NULL //nothing blacklisted
		},
		{ // 00030005
			"484e44", // DS Download Play
			"484e45", // PictoChat
			"484e49", // Nintendo DSi Camera
			"484e4a", // Nintendo Zone
			"484e4b", // Nintendo DSi Sound
			NULL
		},
		{ // 00030015
			"484e42", // System Settings
			"484e46", // Nintendo DSi Shop
			NULL
		},
		{ // 00030017
			"484e41", // Launcher
			NULL
		}
	};

	//Reset menu
	clearMenu(m);

	m->page += sign(m->changePage);
	m->changePage = 0;

	bool done = false;
	int count = 0;	//used to skip to the right page

	//search each category directory /title/XXXXXXXX
	for (int i = 0; i < NUM_OF_DIRS && done == false; i++)
	{
		char* dirPath = (char*)malloc(strlen(dirs[i])+15);
		sprintf(dirPath, "%s:/title/%s", sdnandMode ? "sd" : "nand", dirs[i]);

		struct dirent* ent;
		DIR* dir = opendir(dirPath);

		if (dir)
		{
			while ( (ent = readdir(dir)) && done == false)
			{
				if (strcmp(".", ent->d_name) == 0 || strcmp("..", ent->d_name) == 0)
					continue;

				//blacklisted titles
				if (!sdnandMode)
				{
					//if the region check somehow failed blacklist all-non DSiWare
					if (region == 0 && i > 0) continue;

					bool blacklisted = false;
					for (int j = 0; blacklist[i][j] != NULL; j++)
					{
						char titleId[9];
						sprintf(titleId, "%s%02x", blacklist[i][j], region);
						if (strcmp(titleId, ent->d_name) == 0) 
							blacklisted = true;

						// also blacklist specific all-region titles
						if ((strcmp("484e4441", ent->d_name) == 0) ||  // Download Play
							(strcmp("484e4541", ent->d_name) == 0) ||  // PictoChat
							(strcmp("34544e41", ent->d_name) == 0))    // TwlNmenu
							blacklisted = true;
					}
					if (blacklisted) continue;
				}

				if (ent->d_type == DT_DIR)
				{
					//scan content folder /title/XXXXXXXX/content
					char* contentPath = (char*)malloc(strlen(dirPath) + strlen(ent->d_name) + 20);
					sprintf(contentPath, "%s/%s/content", dirPath, ent->d_name);

					struct dirent* subent;
					DIR* subdir = opendir(contentPath);

					if (subdir)
					{
						while ( (subent = readdir(subdir)) && done == false)
						{
							if (strcmp(".", subent->d_name) == 0 || strcmp("..", subent->d_name) == 0)
								continue;

							if (subent->d_type != DT_DIR)
							{
								//found .app file
								if (strstr(subent->d_name, ".app") != NULL)
								{
									//current item is not on page
									if (count < m->page * ITEMS_PER_PAGE)
										count += 1;

									else
									{
										if (m->itemCount >= ITEMS_PER_PAGE)
											done = true;

										else
										{
											//found requested title
											char* path = (char*)malloc(strlen(contentPath) + strlen(subent->d_name) + 10);
											sprintf(path, "%s/%s", contentPath, subent->d_name);

											char title[128];
											getGameTitlePath(path, title, false);

											addMenuItem(m, title, path, 0);

											free(path);
										}
									}
								}
							}
						}
					}

					closedir(subdir);
					free(contentPath);
				}
			}
		}

		closedir(dir);
		free(dirPath);
	}

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
	printRomInfo(m->items[m->cursor].value);
}

static int subMenu()
{
	int result = -1;

	Menu* m = newMenu();

	addMenuItem(m, "Backup", NULL, 0);
	addMenuItem(m, "Delete", NULL, 0);
	addMenuItem(m, readOnly ? "Mark not read-only" : "Mark read-only", NULL, 0);
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

static void backup(Menu* m)
{
	char* fpath = m->items[m->cursor].value;
	char *backname = NULL;

	tDSiHeader* h = getRomHeader(fpath);

	{
		//make backup folder name
		char label[13];
		getRomLabel(h, label);

		char gamecode[5];
		getRomCode(h, gamecode);

		backname = (char*)malloc(strlen(label) + strlen(gamecode) + 16);
		sprintf(backname, "%s-%s", label, gamecode);

		//make sure dir is unused
		char* dstpath = (char*)malloc(strlen(BACKUP_PATH) + strlen(backname) + 32);
		sprintf(dstpath, "%s/%s.nds", BACKUP_PATH, backname);

		int try = 1;
		while (access(dstpath, F_OK) == 0)
		{
			try += 1;
			sprintf(backname, "%s-%s(%d)", label, gamecode, try);
			sprintf(dstpath, "%s/%s.nds", BACKUP_PATH, backname);
		}

		free(dstpath);
	}

	bool choice = NO;
	{
		const char str[] = "Are you sure you want to backup\n";
		char* msg = (char*)malloc(strlen(str) + strlen(backname) + 2);
		sprintf(msg, "%s%s?", str, backname);

		choice = choiceBox(msg);

		free(msg);
	}

	if (choice == YES)
	{
		char srcpath[30];
		sprintf(srcpath, "%s:/title/%08lx/%08lx", sdnandMode ? "sd" : "nand", h->tid_high, h->tid_low);

		if (getSDCardFree() < getDirSize(srcpath, 0))
		{
			messageBox("Not enough space on SD.");
		}
		else
		{
			//create dirs
			{
				//create subdirectories
				char backupPath[sizeof(BACKUP_PATH)];
				strcpy(backupPath, BACKUP_PATH);
				for (char *slash = strchr(backupPath, '/'); slash; slash = strchr(slash + 1, '/'))
				{
					char temp = *slash;
					*slash = '\0';
					mkdir(backupPath, 0777);
					*slash = temp;
				}
				mkdir(backupPath, 0777); // sd:/_nds/ntm/backup
			}

			clearScreen(&bottomScreen);

			char path[256], dstpath[256];

			//tmd
			sprintf(path, "%s/content/title.tmd", srcpath);
			sprintf(dstpath, "%s/%s.tmd", BACKUP_PATH, backname);
			if (access(path, F_OK) == 0)
			{
				//get app version
				FILE *tmd = fopen(path, "rb");
				if (tmd)
				{
					u8 appVersion[4];
					fseek(tmd, 0x1E4, SEEK_SET);
					fread(&appVersion, 1, 4, tmd);
					fclose(tmd);

					iprintf("%s -> \n%s...\n", path, dstpath);
					copyFile(path, dstpath);

					//app
					sprintf(path, "%s/content/%02x%02x%02x%02x.app", srcpath, appVersion[0], appVersion[1], appVersion[2], appVersion[3]);
					sprintf(dstpath, "%s/%s.nds", BACKUP_PATH, backname);
					if (access(path, F_OK) == 0)
					{
						iprintf("%s -> \n%s...\n", path, dstpath);
						copyFile(path, dstpath);
					}
				}
			}

			//public save
			sprintf(path, "%s/data/public.sav", srcpath);
			sprintf(dstpath, "%s/%s.pub", BACKUP_PATH, backname);
			if (access(path, F_OK) == 0)
			{
				iprintf("%s -> \n%s...\n", path, dstpath);
				copyFile(path, dstpath);
			}

			//private save
			sprintf(path, "%s/data/private.sav", srcpath);
			sprintf(dstpath, "%s/%s.prv", BACKUP_PATH, backname);
			if (access(path, F_OK) == 0)
			{
				iprintf("%s -> \n%s...\n", path, dstpath);
				copyFile(path, dstpath);
			}

			//banner save
			sprintf(path, "%s/data/banner.sav", srcpath);
			sprintf(dstpath, "%s/%s.bnr", BACKUP_PATH, backname);
			if (access(path, F_OK) == 0)
			{
				iprintf("%s -> \n%s...\n", path, dstpath);
				copyFile(path, dstpath);
			}

			messagePrint("\x1B[42m\nBackup finished.\x1B[47m");
		}
	}

	free(h);
}

static bool delete(Menu* m)
{
	if (!m) return false;

	char* fpath = m->items[m->cursor].value;

	bool result = false;
	bool choice = NO;
	{
		//get app title
		char title[128];
		getGameTitlePath(m->items[m->cursor].value, title, false);

		char str[] = "Are you sure you want to delete\n";
		char* msg = (char*)malloc(strlen(str) + strlen(title) + 8);
		sprintf(msg, "%s%s", str, title);

		choice = choiceBox(msg);

		free(msg);
	}

	if (choice == YES)
	{
		if (!fpath)
		{
			messageBox("Failed to delete title.\n");
		}
		else
		{
			char dirPath[64];
			sprintf(dirPath, "%.*s", sdnandMode ? 27 : 29, fpath);

			if (!dirExists(dirPath))
			{
				messageBox("Failed to delete title.\n");
			}
			else
			{
				if (!sdnandMode && !nandio_unlock_writing())
					return false;

				clearScreen(&bottomScreen);

				if (deleteDir(dirPath))
				{
					result = true;
					messagePrint("\nTitle deleted.\n");
				}
				else
				{
					messagePrint("\nTitle could not be deleted.\n");
				}

				if (!sdnandMode)
					nandio_lock_writing();
			}
		}
	}

	return result;
}

static void toggleReadOnly(Menu* m)
{
	if (!m) return;

	tDSiHeader* h = getRomHeader(m->items[m->cursor].value);

	char path[256];
	char srcpath[30];
	sprintf(srcpath, "%s:/title/%08lx/%08lx", sdnandMode ? "sd" : "nand", h->tid_high, h->tid_low);

	if (!sdnandMode && !nandio_unlock_writing()) return;

	//app
	strcpy(path, m->items[m->cursor].value);
	if (access(path, F_OK) == 0)
		FAT_setAttr(path, FAT_getAttr(path) ^ ATTR_READONLY);

	//tmd
	sprintf(path, "%s/content/title.tmd", srcpath);
	if (access(path, F_OK) == 0)
		FAT_setAttr(path, FAT_getAttr(path) ^ ATTR_READONLY);

	//public save
	sprintf(path, "%s/data/public.sav", srcpath);
	if (access(path, F_OK) == 0)
		FAT_setAttr(path, FAT_getAttr(path) ^ ATTR_READONLY);

	//private save
	sprintf(path, "%s/data/private.sav", srcpath);
	if (access(path, F_OK) == 0)
		FAT_setAttr(path, FAT_getAttr(path) ^ ATTR_READONLY);

	//banner save
	sprintf(path, "%s/data/banner.sav", srcpath);
	if (access(path, F_OK) == 0)
		FAT_setAttr(path, FAT_getAttr(path) ^ ATTR_READONLY);

	if (!sdnandMode)
		nandio_lock_writing();

	free(h);

	messageBox("Title's read-only status\nsuccesfully toggled.");
}
