#include "install.h"
#include "sav.h"
#include "main.h"
#include "message.h"
#include "maketmd.h"
#include "crypto/crypto.h"
#include "nandio.h"
#include "rom.h"
#include "storage.h"
#include "crypto/ticket0.h"
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>

static bool _titleIsUsed(tDSiHeader* h)
{
	if (!h) return false;

	char path[64];
	sprintf(path, "%s:/title/%08x/%08x/", sdnandMode ? "sd" : "nand", (unsigned int)h->tid_high, (unsigned int)h->tid_low);

	return dirExists(path);
}

//patch homebrew roms if gameCode is #### or null
static bool _patchGameCode(tDSiHeader* h)
{
	if (!h) return false;

	if ((strcmp(h->ndshdr.gameCode, "####") == 0 && h->tid_low == 0x23232323) || (!*h->ndshdr.gameCode && h->tid_low == 0))
	{
		iprintf("Fixing Game Code...");
		swiWaitForVBlank();

		//set as standard app
		h->tid_high = 0x00030004;

		do {
			do {
				//generate a random game code
				for (int i = 0; i < 4; i++)
					h->ndshdr.gameCode[i] = 'A' + (rand() % 26);
			}
			while (h->ndshdr.gameCode[0] == 'A'); //first letter shouldn't be A

			//correct title id
			h->tid_low = ( (h->ndshdr.gameCode[0] << 24) | (h->ndshdr.gameCode[1] << 16) | (h->ndshdr.gameCode[2] << 8) | h->ndshdr.gameCode[3] );
		}
		while (_titleIsUsed(h));

		iprintf("\x1B[42m");	//green
		iprintf("Done\n");
		iprintf("\x1B[47m");	//white
		return true;
	}

	return false;
}

static bool _iqueHack(tDSiHeader* h)
{
	if (!h) return false;

	if (h->ndshdr.reserved1[8] == 0x80)
	{
		iprintf("iQue Hack...");

		h->ndshdr.reserved1[8] = 0x00;

		iprintf("\x1B[42m");	//green
		iprintf("Done\n");
		iprintf("\x1B[47m");	//white
		return true;
	}

	return false;
}

static unsigned long long _getSaveDataSize(tDSiHeader* h)
{
	unsigned long long size = 0;

	if (h)
	{
		size += h->public_sav_size;
		size += h->private_sav_size;

		//banner.sav
		if (h->appflags & 0x4)
			size += 0x4000;
	}

	return size;
}

static bool _checkSdSpace(unsigned long long size)
{
	iprintf("Enough room on SD card?...");
	swiWaitForVBlank();

	if (getSDCardFree() < size)
	{
		iprintf("\x1B[31m");	//red
		iprintf("No\n");
		iprintf("\x1B[47m");	//white
		return false;
	}

	iprintf("\x1B[42m");	//green
	iprintf("Yes\n");
	iprintf("\x1B[47m");	//white
	return true;
}

static bool _checkDsiSpace(unsigned long long size, bool systemApp)
{
	iprintf("Enough room on DSi?...");
	swiWaitForVBlank();

	//ensure there's at least 1 MiB free, to leave margin for error
	if (((systemApp ? getDsiRealFree() : getDsiFree()) < size) || (((systemApp ? getDsiRealFree() : getDsiFree()) - size) < (1 << 20)))
	{
		iprintf("\x1B[31m");	//red
		iprintf("No\n");
		iprintf("\x1B[47m");	//white
		return false;
	}

	iprintf("\x1B[42m");	//green
	iprintf("Yes\n");
	iprintf("\x1B[47m");	//white
	return true;
}

static bool _openMenuSlot()
{
	iprintf("Open DSi menu slot?...");
	swiWaitForVBlank();

	if (getMenuSlotsFree() <= 0)
	{
		iprintf("\x1B[31m");	//red
		iprintf("No\n");
		iprintf("\x1B[47m");	//white
		return false;
	}

	iprintf("\x1B[42m");	//green
	iprintf("Yes\n");
	iprintf("\x1B[47m");	//white
	return true;
}

static void _createPublicSav(tDSiHeader* h, char* dataPath)
{
	if (!h) return;

	if (h->public_sav_size > 0)
	{
		iprintf("Creating public.sav...");
		swiWaitForVBlank();

		if (!dataPath)
		{
			iprintf("\x1B[31m");	//red
			iprintf("Failed\n");
			iprintf("\x1B[47m");	//white
		}
		else
		{
			char* publicPath = (char*)malloc(strlen(dataPath) + strlen("/public.sav") + 1);
			sprintf(publicPath, "%s/public.sav", dataPath);

			FILE* f = fopen(publicPath, "wb");

			if (!f)
			{
				iprintf("\x1B[31m");	//red
				iprintf("Failed\n");
				iprintf("\x1B[47m");	//white
			}
			else
			{
				fseek(f, h->public_sav_size-1, SEEK_SET);
				fputc(0, f);
				initFatHeader(f);

				iprintf("\x1B[42m");	//green
				iprintf("Done\n");
				iprintf("\x1B[47m");	//white
			}

			fclose(f);
			free(publicPath);
		}
	}
}

static void _createPrivateSav(tDSiHeader* h, char* dataPath)
{
	if (!h) return;

	if (h->private_sav_size > 0)
	{
		iprintf("Creating private.sav...");
		swiWaitForVBlank();

		if (!dataPath)
		{
			iprintf("\x1B[31m");	//red
			iprintf("Failed\n");
			iprintf("\x1B[47m");	//white
		}
		else
		{
			char* privatePath = (char*)malloc(strlen(dataPath) + strlen("/private.sav") + 1);
			sprintf(privatePath, "%s/private.sav", dataPath);

			FILE* f = fopen(privatePath, "wb");

			if (!f)
			{
				iprintf("\x1B[31m");	//red
				iprintf("Failed\n");
				iprintf("\x1B[47m");	//white
			}
			else
			{
				fseek(f, h->private_sav_size-1, SEEK_SET);
				fputc(0, f);
				initFatHeader(f);

				iprintf("\x1B[42m");	//green
				iprintf("Done\n");
				iprintf("\x1B[47m");	//white
			}

			fclose(f);
			free(privatePath);
		}
	}
}

static void _createBannerSav(tDSiHeader* h, char* dataPath)
{
	if (!h) return;

	if (h->appflags & 0x4)
	{
		iprintf("Creating banner.sav...");
		swiWaitForVBlank();

		if (!dataPath)
		{
			iprintf("\x1B[31m");	//red
			iprintf("Failed\n");
			iprintf("\x1B[47m");	//white
		}
		else
		{
			char* bannerPath = (char*)malloc(strlen(dataPath) + strlen("/banner.sav") + 1);
			sprintf(bannerPath, "%s/banner.sav", dataPath);

			FILE* f = fopen(bannerPath, "wb");

			if (!f)
			{
				iprintf("\x1B[31m");	//red
				iprintf("Failed\n");
				iprintf("\x1B[47m");	//white
			}
			else
			{
				fseek(f, 0x4000 - 1, SEEK_SET);
				fputc(0, f);

				iprintf("\x1B[42m");	//green
				iprintf("Done\n");
				iprintf("\x1B[47m");	//white
			}

			fclose(f);
			free(bannerPath);
		}
	}
}

static void createTicket(tDSiHeader *h, char* ticketPath)
{
	if (!h) return;

	iprintf("Forging ticket...");
	swiWaitForVBlank();

	if (!ticketPath)
	{
		iprintf("\x1B[31m");	//red
		iprintf("Failed\n");
		iprintf("\x1B[47m");	//white
	}
	else
	{
		const u32 encryptedSize = sizeof(ticket_v0_t) + 0x20;
		u8 *buffer = (u8*)memalign(4, encryptedSize); //memalign might be needed for encryption, but not sure
		memset(buffer, 0, encryptedSize);
		ticket_v0_t *ticket = (ticket_v0_t*)buffer;
		ticket->sig_type[0] = 0x00;
		ticket->sig_type[1] = 0x01;
		ticket->sig_type[2] = 0x00;
		ticket->sig_type[3] = 0x01;
		strcpy(ticket->issuer, "Root-CA00000001-XS00000006");
		PUT_UINT32_BE(h->tid_high, ticket->title_id, 0);
		PUT_UINT32_BE(h->tid_low, ticket->title_id, 4);
		memset(ticket->content_access_permissions, 0xFF, 0x20);

		// Encrypt
		if (dsi_es_block_crypt(buffer, encryptedSize, ENCRYPT) != 0)
		{
			iprintf("\x1B[31m");	//red
			iprintf("Failed\n");
			iprintf("\x1B[47m");	//white
			free(buffer);
			return;
		}

		FILE *file = fopen(ticketPath, "wb");
		if (!file)
		{
			iprintf("\x1B[31m");	//red
			iprintf("Failed\n");
			iprintf("\x1B[47m");	//white
			free(buffer);
			return;
		}

		if (fwrite(buffer, 1, encryptedSize, file) != encryptedSize)
		{
			iprintf("\x1B[31m");	//red
			iprintf("Failed\n");
			iprintf("\x1B[47m");	//white
		}
		else
		{
			iprintf("\x1B[42m");	//green
			iprintf("Done\n");
			iprintf("\x1B[47m");	//white
		}

		free(buffer);
		fclose(file);
	}
}

bool install(char* fpath, bool systemTitle)
{
	bool result = false;

	//check battery level
	u32 batteryState = pmGetBatteryState();
	u32 batteryLevel = batteryState & 0xF;
	bool charging = batteryState & BIT(7);
	while (batteryLevel < 7 && !charging)
	{
		if (choiceBox("\x1B[47mBattery is too low!\nPlease plug in the console.\n\nContinue?") == NO)
			return false;
	}

	//start installation
	clearScreen(&bottomScreen);

	tDSiHeader* h = getRomHeader(fpath);

	if (!h)
	{
		iprintf("\x1B[31m");	//red
		iprintf("Error: ");
		iprintf("\x1B[33m");	//yellow
		iprintf("Could not open file.\n");
		iprintf("\x1B[47m");	//white
		goto error;
	}
	else
	{
		bool fixHeader = false;

		if (_patchGameCode(h))
			fixHeader = true;

		//title id must be one of these
		if (h->tid_high == 0x00030004 || // DSiWare
			h->tid_high == 0x00030005 || // "unimportant" system titles
			h->tid_high == 0x00030011 || // SRLs in the TWL SDK
			h->tid_high == 0x00030015 || // system titles
			h->tid_high == 0x00030017)   // Launcher
		{}
		else
		{
			iprintf("\x1B[31m");	//red
			iprintf("Error: ");
			iprintf("\x1B[33m");	//yellow
			iprintf("This is not a DSi rom.\n");
			iprintf("\x1B[47m");	//white
			goto error;
		}

		//patch dev titles to system titles on SysNAND.
		//
		//software released through the TWL SDK usually comes as a TAD and an SRL
		//things like NandFiler have a TAD with a TID of 0x00030015 and an SRL with 0x00030011
		//the TAD is the installable version, so I'm assuming that 0x00030015 is what the console wants to see on NAND
		//this changes the SRL TID accordingly
		//not entirely sure why there's even any difference. I think the installed TAD and SRL the same as each other (minus the TID)
		if(!sdnandMode && h->tid_high == 0x00030011)
		{
			h->tid_high = 0x00030015;
			fixHeader = true;
		}

		//offer to patch system titles to normal DSiWare on SysNAND
		if(!sdnandMode && h->tid_high != 0x00030004 && h->tid_high != 0x00030017) //do not allow patching home menus to be normal DSiWare! This will trigger "ERROR! - 0x0000000000000008 HWINFO_SECURE" on prototype launchers. May also cause issues on the prod versions.
		{
			if(choiceBox("This is set as a system/dev\ntitle, would you like to patch\nit to be a normal DSiWare?\n\nThis is safer, but invalidates\nRSA checks and may not work.\n\nIf the title is homebrew this isstrongly recommended.") == YES)
			{
				h->tid_high = 0x00030004;
				fixHeader = true;
			}
		}

		//offer to patch home menus to be system titles on SysNAND
		if(!sdnandMode && h->tid_high == 0x00030017)
		{
			if(choiceBox("This title is a home menu.\nWould you like to patch it to bea system title?\n\nThis is safer and prevents your\nhome menu from being hidden.") == YES)
			{
				h->tid_high = 0x00030015;
				fixHeader = true;
			}
		}

		//no system titles without Unlaunch
		if (!unlaunchFound && h->tid_high != 0x00030004)
		{
			iprintf("\x1B[31m");	//red
			iprintf("Error: ");
			iprintf("\x1B[33m");	//yellow
			iprintf("This title cannot be\ninstalled without Unlaunch.\n");
			iprintf("\x1B[47m");	//white
			goto error;
		}

		//blacklisted titles
		{
			//tid without region
			u32 tidLow = (h->tid_low & 0xFFFFFF00);
			if (!sdnandMode && (
				(h->tid_high == 0x00030005 && (
					tidLow == 0x484e4400 || // DS Download Play
					tidLow == 0x484e4500 || // PictoChat
					tidLow == 0x484e4900 || // Nintendo DSi Camera
					tidLow == 0x484e4a00 || // Nintendo Zone
					tidLow == 0x484e4b00    // Nintendo DSi Sound
				)) || (h->tid_high == 0x00030011 && (
					tidLow == 0x30535500 || // Twl SystemUpdater
					tidLow == 0x34544e00 || // TwlNmenu
					tidLow == 0x54574c00    // TWL EVA
				)) || (h->tid_high == 0x00030015 && (
					tidLow == 0x484e4200 || // System Settings
					tidLow == 0x484e4600 || // Nintendo DSi Shop
					tidLow == 0x34544e00    // TwlNmenu
				)) || (h->tid_high == 0x00030017 && (
					tidLow == 0x484e4100    // Launcher
				))) && (
					(h->tid_low & 0xFF) == region || // Only blacklist console region, or the following programs that have all-region codes:
					h->tid_low == 0x484e4541 ||      // PictoChat 
					h->tid_low == 0x484e4441 ||      // Download Play
					h->tid_low == 0x30535541 ||      // Twl SystemUpdater (iirc one version fits in NAND)
					h->tid_low == 0x34544e41 ||      // TwlNmenu (blocking due to potential to uninstall system titles)
					h->tid_low == 0x54574c41 ||      // TWL EVA
					region == 0                      //if the region check failed somehow, blacklist everything
				))
			{
				//check if title exists, if it does then show any error
				//otherwise allow reinstalling it
				char path[PATH_MAX];
				sprintf(path, "nand:/title/%08lx/%08lx/content/title.tmd", h->tid_high, h->tid_low);
				if (access(path, F_OK) == 0)
				{
					iprintf("\x1B[31m");	//red
					iprintf("Error: ");
					iprintf("\x1B[33m");	//yellow
					iprintf("This title cannot be\ninstalled to SysNAND.\n");
					iprintf("\x1B[47m");	//white
					goto error;
				}
			}
		}

		//confirmation message
		{
			const char system[] = "\x1B[41mWARNING:\x1B[47m This is a system app,\ninstalling it is potentially\nmore risky than regular DSiWare.\n\x1B[33m";
			const char areYouSure[] = "Are you sure you want to install\n";
			char* msg = (char*)malloc(strlen(system) + strlen(areYouSure) + strlen(fpath) + 2);
			if (sdnandMode || h->tid_high == 0x00030004)
				sprintf(msg, "%s%s?\n", areYouSure, fpath);
			else
				sprintf(msg, "%s%s%s?\n", system, areYouSure, fpath);

			bool choice = choiceBox(msg);
			free(msg);

			if (choice == NO)
				return false;
		}

		if (!sdnandMode && !nandio_unlock_writing())
			return false;

		clearScreen(&bottomScreen);
		iprintf("Installing %s\n\n", fpath); swiWaitForVBlank();

		//check for legit TMD, if found we'll generate a ticket which increases the size
		int extensionPos = strrchr(fpath, '.') - fpath;
		char tmdPath[PATH_MAX];
		strcpy(tmdPath, fpath);
		strcpy(tmdPath + extensionPos, ".tmd");
		//DSi TMDs are 520, TMDs from NUS are 2,312. If 2,312 we can simply trim it to 520
		int tmdSize = getFileSizePath(tmdPath);
		bool tmdFound = (tmdSize == 520) || (tmdSize == 2312);
		if (access(tmdPath, F_OK) == 0 && !tmdFound)
		{
			if (choicePrint("Incorrect TMD.\nInstall anyway?") == YES)
				tmdFound = false;
			else
				goto error;
		}
		else if(!sdnandMode && !unlaunchPatches && access(tmdPath, F_OK) != 0)
		{
			if (choicePrint("TMD not found, game cannot be\nplayed without Unlaunch's\nlauncher patches.\nSee wiki for how to get a TMD.\n\nInstall anyway?") == YES)
				tmdFound = false;
			else
				goto error;
		}

		//get install size
		iprintf("Install Size: ");
		swiWaitForVBlank();

		u32 clusterSize = getDsiClusterSize();
		unsigned long long fileSize = getRomSize(fpath), fileSizeOnDisk = fileSize;
		if ((fileSizeOnDisk % clusterSize) != 0)
			fileSizeOnDisk += clusterSize - (fileSizeOnDisk % clusterSize);
		//file + saves + TMD (rounded up to cluster size)
		unsigned long long installSize = fileSizeOnDisk + _getSaveDataSize(h) + clusterSize;
		if (tmdFound) installSize += clusterSize; //ticket, rounded up to cluster size

		printBytes(installSize);
		iprintf("\n");

		if (sdnandMode && !_checkSdSpace(installSize))
			goto error;

		//system title patch
		if (systemTitle)
		{
			iprintf("System Title Patch...");
			swiWaitForVBlank();
			h->tid_high = 0x00030015;
			iprintf("\x1B[42m");	//green
			iprintf("Done\n");
			iprintf("\x1B[47m");	//white

			fixHeader = true;
		}

		//check that there's space on nand
		if (!_checkDsiSpace(installSize, (h->tid_high != 0x00030004)))
		{
			if (sdnandMode && choicePrint("Install as system title?"))
			{
				h->tid_high = 0x00030015;
				fixHeader = true;
			}
			else
			{
				goto error;
			}
		}

		//check for saves
		char pubPath[PATH_MAX];
		strcpy(pubPath, fpath);
		strcpy(pubPath + extensionPos, ".pub");
		bool pubFound = getFileSizePath(pubPath) == h->public_sav_size;
		if (access(pubPath, F_OK) == 0 && !pubFound)
		{
			if (choicePrint("Incorrect public save.\nInstall anyway?") == YES)
				pubFound = false;
			else
				goto error;
		}

		char prvPath[PATH_MAX];
		strcpy(prvPath, fpath);
		strcpy(prvPath + extensionPos, ".prv");
		bool prvFound = getFileSizePath(prvPath) == h->private_sav_size;
		if (access(prvPath, F_OK) == 0 && !prvFound)
		{
			if (choicePrint("Incorrect private save.\nInstall anyway?") == YES)
				prvFound = false;
			else
				goto error;
		}

		char bnrPath[PATH_MAX];
		strcpy(bnrPath, fpath);
		strcpy(bnrPath + extensionPos, ".bnr");
		bool bnrFound = getFileSizePath(bnrPath) == 0x4000;
		if (access(bnrPath, F_OK) == 0 && !bnrFound)
		{
			if (choicePrint("Incorrect banner save.\nInstall anyway?") == YES)
				bnrFound = false;
			else
				goto error;
		}

		if (_iqueHack(h))
			fixHeader = true;

		if (fixHeader && tmdFound)
		{
			if (choicePrint("Legit TMD cannot be used.\nInstall anyway?") == YES)
				tmdFound = false;
			else
				goto error;
		}

		//create title directory /title/XXXXXXXX/XXXXXXXX
		char dirPath[32];
		mkdir(sdnandMode ? "sd:/title" : "nand:/title", 0777);

		sprintf(dirPath, "%s:/title/%08x", sdnandMode ? "sd" : "nand", (unsigned int)h->tid_high);
		mkdir(dirPath, 0777);

		sprintf(dirPath, "%s:/title/%08x/%08x", sdnandMode ? "sd" : "nand", (unsigned int)h->tid_high, (unsigned int)h->tid_low);

		//check if title is free
		if (_titleIsUsed(h))
		{
			char msg[64];
			sprintf(msg, "Title %08x is already used.\nInstall anyway?", (unsigned int)h->tid_low);

			if (choicePrint(msg) == NO)
				goto error;

			else
			{
				iprintf("\nDeleting:\n");
				deleteDir(dirPath);
				iprintf("\n");
			}
		}

		if (!_openMenuSlot())
			goto error;

		mkdir(dirPath, 0777);

		//content folder /title/XXXXXXXX/XXXXXXXXX/content
		{
			char contentPath[64];
			sprintf(contentPath, "%s/content", dirPath);

			mkdir(contentPath, 0777);

			u8 appVersion = 0;
			if (tmdFound)
			{
				FILE *file = fopen(tmdPath, "rb");
				if (file)
				{
					fseek(file, 0x1E7, SEEK_SET);
					fread(&appVersion, sizeof(appVersion), 1, file);
					fclose(file);
				}
			}

			//create 000000##.app
			{
				iprintf("Creating 000000%02x.app...", appVersion);
				swiWaitForVBlank();

				char appPath[80];
				sprintf(appPath, "%s/000000%02x.app", contentPath, appVersion);

				//copy nds file to app
				{
					int result = 0;

					if (!romIsCia(fpath))
						result = copyFile(fpath, appPath);
					else
						result = copyFilePart(fpath, 0x3900, fileSize, appPath);

					if (result != 0)
					{
						iprintf("\x1B[31m");	//red
						iprintf("Failed\n");
						iprintf("\x1B[33m");	//yellow
						iprintf("%s\n", appPath);
						iprintf("%s\n", strerror(errno));
						iprintf("\x1B[47m");	//white

						goto error;
					}

					iprintf("\x1B[42m");	//green
					iprintf("Done\n");
					iprintf("\x1B[47m");	//white
				}

				//pad out banner if it is the last part of the file
				{
					if (h->ndshdr.bannerOffset > (fileSize - 0x23C0))
					{
						iprintf("Padding banner...");
						swiWaitForVBlank();

						if (padFile(appPath, h->ndshdr.bannerOffset + 0x23C0 - fileSize) == false)
						{
							iprintf("\x1B[31m");	//red
							iprintf("Failed\n");
							iprintf("\x1B[47m");	//white
						}
						else
						{
							iprintf("\x1B[42m");	//green
							iprintf("Done\n");
							iprintf("\x1B[47m");	//white
						}
					}
				}

				//update header
				{
					if (fixHeader)
					{
						iprintf("Fixing header...");
						swiWaitForVBlank();

						//fix header checksum
						h->ndshdr.headerCRC16 = swiCRC16(0xFFFF, h, 0x15E);

						//fix RSA signature
						u8 buffer[20];
						swiSHA1Calc(&buffer, h, 0xE00);
						memcpy(&(h->rsa_signature[0x6C]), buffer, 20);

						FILE* f = fopen(appPath, "r+");

						if (!f)
						{
							iprintf("\x1B[31m");	//red
							iprintf("Failed\n");
							iprintf("\x1B[47m");	//white
						}
						else
						{
							fseek(f, 0, SEEK_SET);
							fwrite(h, sizeof(tDSiHeader), 1, f);

							iprintf("\x1B[42m");	//green
							iprintf("Done\n");
							iprintf("\x1B[47m");	//white
						}

						fclose(f);
					}
				}

				//make/copy TMD
				char newTmdPath[80];
				sprintf(newTmdPath, "%s/title.tmd", contentPath);
				if (tmdFound)
				{
					if (copyFilePart(tmdPath, 0, 520, newTmdPath) != 0)
						goto error;
				}
				else
				{
					if (maketmd(appPath, newTmdPath) != 0)
						goto error;
				}
			}
		}

		//data folder
		{
			char dataPath[64];
			sprintf(dataPath, "%s/data", dirPath);

			mkdir(dataPath, 0777);

			if (pubFound)
			{
				char newPubPath[80];
				sprintf(newPubPath, "%s/public.sav", dataPath);
				copyFile(pubPath, newPubPath);
			}
			else
			{
				_createPublicSav(h, dataPath);
			}

			if (prvFound)
			{
				char newPrvPath[80];
				sprintf(newPrvPath, "%s/private.sav", dataPath);
				copyFile(prvPath, newPrvPath);
			}
			else
			{
				_createPrivateSav(h, dataPath);
			}

			if (bnrFound)
			{
				char newBnrPath[80];
				sprintf(newBnrPath, "%s/banner.sav", dataPath);
				copyFile(bnrPath, newBnrPath);
			}
			else
			{
				_createBannerSav(h, dataPath);
			}
		}

		//ticket folder /ticket/XXXXXXXX
		if (tmdFound)
		{
			//ensure folders exist
			char ticketPath[32];
			siprintf(ticketPath, "%s:/ticket", sdnandMode ? "sd" : "nand");
			mkdir(ticketPath, 0777);
			siprintf(ticketPath, "%s/%08lx", ticketPath, h->tid_high);
			mkdir(ticketPath, 0777);

			//actual tik path
			siprintf(ticketPath, "%s/%08lx.tik", ticketPath, h->tid_low);

			if (access(ticketPath, F_OK) != 0 || (choicePrint("Ticket already exists.\nKeep it? (recommended)") == NO && choicePrint("Are you sure?") == YES))
				createTicket(h, ticketPath);
		}

		//end
		result = true;
		iprintf("\x1B[42m");	//green
		iprintf("\nInstallation complete.\n");
		iprintf("\x1B[47m");	//white
		iprintf("Back - [B]\n");
		keyWait(KEY_A | KEY_B);

		goto complete;
	}

error:
	messagePrint("\x1B[31m\nInstallation failed.\n\x1B[47m");

complete:
	free(h);

	if (!sdnandMode)
		nandio_lock_writing();

	return result;
}