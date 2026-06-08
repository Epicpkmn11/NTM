#include "main.h"
#include "crypto.h"
#include "menu.h"
#include "message.h"
#include "nandio.h"
#include "nds/arm9/exceptions.h"
#include "storage.h"
#include "version.h"
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

extern unsigned g_dvmCalicoNandMount;

bool sdnandMode = true;
bool useFlashcard = false;
bool multipleDrives = false;
bool unlaunchFound = false;
bool unlaunchPatches = false;
bool devkpFound = false;
bool launcherDSiFound = false;
u8 region = 0;

PrintConsole topScreen;
PrintConsole bottomScreen;

enum {
	MAIN_MENU_MODE,
	MAIN_MENU_SD,
	MAIN_MENU_INSTALL,
	MAIN_MENU_TITLES,
	MAIN_MENU_BACKUP,
	MAIN_MENU_TEST,
	MAIN_MENU_FIX,
	MAIN_MENU_DATA_MANAGEMENT,
	MAIN_MENU_LANGUAGE_PATCHER,
	MAIN_MENU_EXIT
};

static void _setupScreens()
{
	REG_DISPCNT = MODE_FB0;
	VRAM_A_CR = VRAM_ENABLE;

	videoSetMode(MODE_0_2D);
	videoSetModeSub(MODE_0_2D);

	vramSetBankA(VRAM_A_MAIN_BG);
	vramSetBankC(VRAM_C_SUB_BG);

	consoleInit(&topScreen,    3, BgType_Text4bpp, BgSize_T_256x256, 31, 0, true,  true);
	consoleInit(&bottomScreen, 3, BgType_Text4bpp, BgSize_T_256x256, 31, 0, false, true);

	clearScreen(&bottomScreen);

	VRAM_A[100] = 0xFFFF;
}

static int _mainMenu(int cursor)
{
	//top screen
	clearScreen(&topScreen);

	printf("\t\tNAND Title Manager\n");
	printf("\t\t\tmodified from\n");
	printf("\tTitle Manager for HiyaCFW\n");
	printf("\nversion %s\n", VER_NUMBER);
	printf("\n\n\x1B[41mWARNING:\x1B[47m This tool can write to\nyour internal NAND!\n\nThis always has a risk, albeit\nlow, of \x1B[41mbricking\x1B[47m your system\nand should be done with caution!\n");
	printf("\n\t  \x1B[46mhttps://dsi.cfw.guide\x1B[47m\n");
	printf("\n\n \x1B[46mgithub.com/Epicpkmn11/NTM/wiki\x1B[47m\n");
	printf("\x1b[22;0HJeff - 2018-2019");
	printf("\x1b[23;0HPk11 - 2022-2026");

	//menu
	Menu* m = newMenu();
	setMenuHeader(m, "MAIN MENU");

	char modeStr[32], sdStr[32], datamanStr[32], launcherStr[32];
	sprintf(modeStr, "Mode: %s", sdnandMode ? "SDNAND" : "\x1B[41mSysNAND\x1B[47m");
	sprintf(sdStr, "\x1B[%02omSD Card: %s", multipleDrives ? 047 : 037, useFlashcard ? "fat:/ (Slot-1)" : "sd:/ (Internal)");
	sprintf(datamanStr, "\x1B[47m%s Data Management", !devkpFound ? "Enable" : "Disable");
	sprintf(launcherStr, "\x1B[%02omUninstall region mod", launcherDSiFound ? 047 : 037);
	addMenuItem(m, modeStr, NULL, 0);
	addMenuItem(m, sdStr, NULL, 0);
	addMenuItem(m, "\x1B[47mInstall", NULL, 0);
	addMenuItem(m, "Titles", NULL, 0);
	addMenuItem(m, "Restore", NULL, 0);
	addMenuItem(m, "Test", NULL, 0);
	addMenuItem(m, "\x1B[37mFix FAT copy mismatch", NULL, 0); // Temporarily disabled
	addMenuItem(m, datamanStr, NULL, 0);
	addMenuItem(m, launcherStr, NULL, 0);
	addMenuItem(m, "\x1B[47mExit", NULL, 0);

	m->cursor = cursor;

	//bottom screen
	printMenu(m);

	while (1)
	{
		swiWaitForVBlank();
		scanKeys();

		if (moveCursor(m))
			printMenu(m);

		if (keysDown() & KEY_A)
			break;
	}

	int result = m->cursor;
	freeMenu(m);

	return result;
}

int main(int argc, char **argv)
{
	defaultExceptionHandler();
	srand(time(0));
	keysSetRepeat(25, 5);
	_setupScreens();

	//DSi check
	if (!isDSiMode())
	{
		messageBox("\x1B[31mError:\x1B[33m This app is only for DSi.");
		return 0;
	}

	//setup sd card access
	if (!fatInitDefault())
	{
		messageBox("fatInitDefault()...\x1B[31mFailed\n\x1B[47m");
		return 0;
	}
	else
	{
		char path[16];
		getcwd(path, 16);
		useFlashcard = strcmp(path, "fat:/") == 0;
		multipleDrives = access(useFlashcard ? "sd:/" : "fat:/", F_OK) == 0;
	}

	//setup NAND access
	if (!nandInit(false))
	{
		messageBox("nandInit()...\x1B[31mFailed\n\x1B[47m");
		return 0;
	}

	//initialize crypto
	{
		fifoWaitValue32(FIFO_USER_01);

		u32 consoleId[2];
		consoleId[0] = fifoGetValue32(FIFO_USER_01);
		consoleId[1] = fifoGetValue32(FIFO_USER_01);

		dsi_crypt_init(consoleId);
	}

	//check for unlaunch and region
	{
		FILE *file = fopen("nand:/sys/HWINFO_S.dat", "rb");
		if (file)
		{
			fseek(file, 0xA0, SEEK_SET);
			u32 launcherTid;
			fread(&launcherTid, sizeof(u32), 1, file);
			fclose(file);

			region = launcherTid & 0xFF;

			char path[64];
			sprintf(path, "nand:/title/00030017/%08lx/content/title.tmd", launcherTid);

			// check for traditional unlaunch install
			unsigned long long tmdSize = getFileSizePath(path);
			if (tmdSize > 520)
			{
				unlaunchFound = true;

				//check if launcher patches are enabled
				const static u32 tidValues[][2] = {
					// {location, value}
					{0xE439, 0x382E3176}, // 1.8
					{0xB07C, 0x17484E41}, // 1.9
					{0xB099, 0x17484E41}, // 2.0 (Normal)
					{0xB079, 0x484E1841}, // 2.0 (Patched)
				};

				FILE *tmd = fopen(path, "rb");
				if (tmd)
				{
					for (int i = 0; i < sizeof(tidValues) / sizeof(tidValues[0]); i++)
					{
						if (fseek(tmd, tidValues[i][0], SEEK_SET) == 0)
						{
							u32 tidVal;
							fread(&tidVal, sizeof(u32), 1, tmd);
							if (tidVal == tidValues[i][1])
							{
								unlaunchPatches = true;
								fclose(tmd);
								tmd = NULL;
								break;
							}
						}
					}

					if(tmd)
						fclose(tmd);
				}
			}
			else // check for "safe" unlaunch install
			{
				FILE *tmd = fopen(path, "rb");
				if (tmd)
				{
					// check if its "corrupted" byte is present
					fseek(tmd, 0x190, SEEK_SET);
					unsigned char val;
					fread(&val, 1, 1, tmd);
					fclose(tmd);

					if (val == 0x47)
					{
						// the safe installer always has patches enabled
						unlaunchFound = true;
						unlaunchPatches = true;
					}
				}
			}
		}

		if (!unlaunchFound)
		{
			messageBox("Unlaunch not found. TMD files\nwill be required and there\nis a greater risk something\ncould go wrong.\n\nSee \x1B[46mhttps://dsi.cfw.guide/\x1B[47m to\ninstall.");
		}
		else if (!unlaunchPatches)
		{
			messageBox("Unlaunch's Launcher Patches are\nnot enabled. You will need to\nprovide TMD files or reinstall.\n\n\x1B[46mhttps://dsi.cfw.guide/\x1B[47m");
		}
	}

	//check for dev.kp (Data Management visible)
	devkpFound = (access("/sys/dev.kp", F_OK) == 0);

	//check for launcher.dsi (Language patcher)
	launcherDSiFound = (access("nand:/launcher.dsi", F_OK) == 0);

	messageBox("\x1B[41mWARNING:\x1B[47m This tool can write to\nyour internal NAND!\n\nThis always has a risk, albeit\nlow, of \x1B[41mbricking\x1B[47m your system\nand should be done with caution!\n\nIf you have not yet done so,\nyou should make a NAND backup.");

	messageBox("If you are following a video\nguide, please stop.\n\nVideo guides for console moddingare often outdated or straight\nup incorrect to begin with.\n\nThe recommended guide for\nmodding your DSi is:\n\n\x1B[46mhttps://dsi.cfw.guide/\x1B[47m\n\nFor more information on using\nNTM, see the official wiki:\n\n\x1B[46mhttps://github.com/Epicpkmn11/\n\t\t\t\t\t\t\t\tNTM/wiki\x1B[47m");
	//main menu
	int cursor = 0;

	while (1)
	{
		cursor = _mainMenu(cursor);

		switch (cursor)
		{
			case MAIN_MENU_MODE:
				sdnandMode = !sdnandMode;
				devkpFound = (access(sdnandMode ? "/sys/dev.kp" : "nand:/sys/dev.kp", F_OK) == 0);
				break;

			case MAIN_MENU_SD:
				if (multipleDrives)
				{
					useFlashcard = !useFlashcard;
					chdir(useFlashcard ? "fat:/" : "sd:/");
				}
				break;

			case MAIN_MENU_INSTALL:
				installMenu();
				break;

			case MAIN_MENU_TITLES:
				titleMenu();
				break;

			case MAIN_MENU_BACKUP:
				backupMenu();
				break;

			case MAIN_MENU_TEST:
				testMenu();
				break;

			case MAIN_MENU_FIX:
				// if (nandio_unlock_writing())
				// {
				// 	nandio_force_fat_fix();
				// 	nandio_lock_writing();
				// 	messageBox("Mismatch in FAT copies will be\nfixed on close.\n");
				// }
				break;

			case MAIN_MENU_DATA_MANAGEMENT:
				char* message = !devkpFound ? "Make Data Management visible\nin System Settings?" : "Hide Data Management from\nSystem Settings?";
				if ((choiceBox(message) == YES) && (sdnandMode || nandio_unlock_writing()))
				{
					//ensure sys folder exists
					if(access(sdnandMode ? "/sys" : "nand:/sys", F_OK) != 0)
						mkdir(sdnandMode ? "/sys" : "nand:/sys", 0777);

					//check whether we need to add/remove the file
					char *path = sdnandMode ? "/sys/dev.kp" : "nand:/sys/dev.kp";
					if (!devkpFound)
					{
						//create empty file
						FILE *file = fopen(path, "wb");
						fclose(file);
					}
					else
					{
						//remove file, if not 0 bytes (fake file see above)
						//prompt to make sure
						u32 size = getFileSizePath(path);
						bool res = (size == 0) || choiceBox("dev.kp is not empty! This is\nprobably a real dev.kp, which\ncannot be regenerated,\ndelete anyways?");
						if(res)
							remove(path);
					}

					if(!sdnandMode)
						nandio_lock_writing();
					devkpFound = (access(sdnandMode ? "/sys/dev.kp" : "nand:/sys/dev.kp", F_OK) == 0);
					char* successMessage = devkpFound ? "Data Management is now visible\nin System Settings.\n" : "Data Management is now hidden\nfrom System Settings.\n";
					messageBox(successMessage);
				}
				break;
			case MAIN_MENU_LANGUAGE_PATCHER:
				if (launcherDSiFound && (choiceBox("Uninstall the language patched\nDSi Menu? (launcher.dsi)") == YES) && nandio_unlock_writing())
				{
					//delete launcher.dsi
					remove("nand:/launcher.dsi");

					nandio_lock_writing();
					launcherDSiFound = (access("nand:/launcher.dsi", F_OK) == 0);
					messageBox("The language patched DSi Menu\nhas been removed.\n");
				}
				break;

			case MAIN_MENU_EXIT:
				return 0;
		}
	}
}

void clearScreen(PrintConsole* screen)
{
	consoleSelect(screen);
	consoleClear();
}
