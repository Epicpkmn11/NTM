#include "storage.h"
#include "main.h"
#include "message.h"
#include <errno.h>
#include <dirent.h>

#define TITLE_LIMIT 39

//printing
void printBytes(unsigned long long bytes)
{
	if (bytes < 1024)
		iprintf("%dB", (unsigned int)bytes);

	else if (bytes < 1024 * 1024)
		printf("%.2fKB", (float)bytes / 1024.f);

	else if (bytes < 1024 * 1024 * 1024)
		printf("%.2fMB", (float)bytes / 1024.f / 1024.f);

	else
		printf("%.2fGB", (float)bytes / 1024.f / 1024.f / 1024.f);
}

//progress bar
static int lastBars = 0;

void printProgressBar(float percent)
{
	if (percent < 0.f) percent = 0.f;
	if (percent > 1.f) percent = 1.f;

	int bars = (int)(30.f * percent);

	//skip redundant prints
	if (bars != lastBars)
	{
		consoleSelect(&topScreen);

		iprintf("\x1B[42m");	//green

		//Print frame
		if (lastBars <= 0)
		{
			iprintf("\x1b[23;0H[");
			iprintf("\x1b[23;31H]");
		}

		//Print bars
		if (bars > 0)
		{
			for (int i = 0; i < bars; i++)
				iprintf("\x1b[23;%dH|", 1 + i);
		}

		lastBars = bars;

		iprintf("\x1B[47m");	//white
	}
}

void clearProgressBar()
{
	lastBars = 0;
	consoleSelect(&topScreen);
	iprintf("\x1b[23;0H                                ");
}

//files
bool fileExists(char const* path)
{
	if (!path) return false;

	FILE* f = fopen(path, "rb");
	if (!f)
		return false;

	fclose(f);
	return true;
}

int copyFile(char const* src, char const* dst)
{
	if (!src) return 1;

	unsigned long long size = getFileSizePath(src);
	return copyFilePart(src, 0, size, dst);
}

int copyFilePart(char const* src, u32 offset, u32 size, char const* dst)
{
	if (!src) return 1;
	if (!dst) return 2;

	FILE* fin = fopen(src, "rb");

	if (!fin)
	{
		fclose(fin);
		return 3;
	}
	else
	{
		if (fileExists(dst))
			remove(dst);

		FILE* fout = fopen(dst, "wb");

		if (!fout)
		{
			fclose(fin);
			fclose(fout);
			return 4;
		}
		else
		{
			fseek(fin, offset, SEEK_SET);

			consoleSelect(&topScreen);

			int bytesRead;
			unsigned long long totalBytesRead = 0;

			#define BUFF_SIZE 128 //Arbitrary. A value too large freezes the ds.
			char* buffer = (char*)malloc(BUFF_SIZE);

			while (!programEnd)
			{
				unsigned int toRead = BUFF_SIZE;
				if (size - totalBytesRead < BUFF_SIZE)
					toRead = size - totalBytesRead;

				bytesRead = fread(buffer, 1, toRead, fin);
				fwrite(buffer, bytesRead, 1, fout);

				totalBytesRead += bytesRead;
				printProgressBar( ((float)totalBytesRead / (float)size) );

				if (bytesRead != BUFF_SIZE)
					break;
			}

			clearProgressBar();
			consoleSelect(&bottomScreen);

			free(buffer);
		}

		fclose(fout);
	}

	fclose(fin);
	return 0;
}

unsigned long long getFileSize(FILE* f)
{
	if (!f) return 0;

	fseek(f, 0, SEEK_END);
	unsigned long long size = ftell(f);
	fseek(f, 0, SEEK_SET);

	return size;
}

unsigned long long getFileSizePath(char const* path)
{
	if (!path) return 0;

	FILE* f = fopen(path, "rb");
	unsigned long long size = getFileSize(f);
	fclose(f);

	return size;
}

bool padFile(char const* path, int size)
{
	if (!path) return false;

	FILE* f = fopen(path, "ab");
	if (!f)
	{
		return false;
	}
	else
	{
		for (int i = 0; i < size; i++)
			fputc('\0', f);
	}

	fclose(f);
	return true;
}

//directories
bool dirExists(char const* path)
{
	if (!path) return false;

	DIR* dir = opendir(path);

	if (!dir)
		return false;

	closedir(dir);
	return true;
}

bool copyDir(char const* src, char const* dst)
{
	if (!src || !dst) return false;

//	iprintf("copyDir\n%s\n%s\n\n", src, dst);

	bool result = true;

	DIR* dir = opendir(src);
	struct dirent* ent;

	if (!dir)
	{
		return false;
	}
	else
	{
		while ( (ent = readdir(dir)) )
		{
			if (strcmp(".", ent->d_name) == 0 || strcmp("..", ent->d_name) == 0)
				continue;

			if (ent->d_type == DT_DIR)
			{
				char* dsrc = (char*)malloc(strlen(src) + strlen(ent->d_name) + 4);
				sprintf(dsrc, "%s/%s", src, ent->d_name);

				char* ddst = (char*)malloc(strlen(dst) + strlen(ent->d_name) + 4);
				sprintf(ddst, "%s/%s", dst, ent->d_name);

				mkdir(ddst, 0777);
				if (!copyDir(dsrc, ddst))
					result = false;

				free(ddst);
				free(dsrc);
			}
			else
			{
				char* fsrc = (char*)malloc(strlen(src) + strlen(ent->d_name) + 4);
				sprintf(fsrc, "%s/%s", src, ent->d_name);

				char* fdst = (char*)malloc(strlen(dst) + strlen(ent->d_name) + 4);
				sprintf(fdst, "%s/%s", dst, ent->d_name);

//				iprintf("%s\n%s\n\n", fsrc, fdst);
				iprintf("%s -> \n%s...", fsrc, fdst);

				int ret = copyFile(fsrc, fdst);

				if (ret != 0)
				{
					iprintf("\x1B[31m");	//red
					iprintf("Fail\n");
					iprintf("\x1B[33m");	//yellow

					iprintf("%s\n", strerror(errno));
/*
					switch (ret)
					{
						case 1:
							iprintf("Empty input path.\n");
							break;

						case 2:
							iprintf("Empty output path.\n");
							break;

						case 3:
							iprintf("Error opening input file.\n");
							break;

						case 4:
							iprintf("Error opening output file.\n");
							break;
					}
*/
					iprintf("\x1B[47m");	//white
					result = false;
				}
				else
				{
					iprintf("\x1B[42m");	//green
					iprintf("Done\n");
					iprintf("\x1B[47m");	//white
				}

				free(fdst);
				free(fsrc);
			}
		}
	}

	closedir(dir);
	return result;
}

bool deleteDir(char const* path)
{
	if (!path) return false;

	if (strcmp("/", path) == 0)
	{
		//oh fuck no
		return false;
	}

	bool result = true;

	DIR* dir = opendir(path);
	struct dirent* ent;

	if (!dir)
	{
		result = false;
	}
	else
	{
		while ( (ent = readdir(dir)) )
		{
			if (strcmp(".", ent->d_name) == 0 || strcmp("..", ent->d_name) == 0)
				continue;

			if (ent->d_type == DT_DIR)
			{
				//Delete directory
				char subpath[512];
				sprintf(subpath, "%s/%s", path, ent->d_name);

				if (!deleteDir(subpath))
					result = false;
			}
			else
			{
				//Delete file
				char fpath[512];
				sprintf(fpath, "%s/%s", path, ent->d_name);

				iprintf("%s...", fpath);
				if (remove(fpath) != 0)
				{
					iprintf("\x1B[31m");
					iprintf("Fail\n");
					iprintf("\x1B[47m");
					result = false;
				}
				else
				{
					iprintf("\x1B[42m");
					iprintf("Done\n");
					iprintf("\x1B[47m");
				}
			}
		}
	}

	closedir(dir);

	iprintf("%s...", path);
	if (remove(path) != 0)
	{
		iprintf("\x1B[31m");
		iprintf("Fail\n");
		iprintf("\x1B[47m");
		result = false;
	}
	else
	{
		iprintf("\x1B[42m");
		iprintf("Done\n");
		iprintf("\x1B[47m");
	}

	return result;
}

unsigned long long getDirSize(const char* path, u32 blockSize)
{
	if (!path) return 0;

	unsigned long long size = 0;
	DIR* dir = opendir(path);
	struct dirent* ent;

	if (dir)
	{
		while ((ent = readdir(dir)))
		{
			if (strcmp(".", ent->d_name) == 0 || strcmp("..", ent->d_name) == 0)
				continue;

			if (ent->d_type == DT_DIR)
			{
				char fullpath[512];
				sprintf(fullpath, "%s/%s", path, ent->d_name);

				size += getDirSize(fullpath, blockSize);
			}
			else
			{
				char fullpath[260];
				sprintf(fullpath, "%s/%s", path, ent->d_name);

				size += getFileSizePath(fullpath);

				// If we've specified a block size, round up to it
				if ((size % blockSize) != 0)
					size += blockSize - (size % blockSize);
			}
		}
	}

	closedir(dir);
	return size;
}

//home menu
int getMenuSlots()
{
	//Assume the home menu has a hard limit on slots
	//Find a better way to do this
	return TITLE_LIMIT;
}

int getMenuSlotsFree()
{
	//Get number of open menu slots by subtracting the number of directories in the title folders
	//Find a better way to do this
	const int NUM_OF_DIRS = 4;
	const char* dirs[] = {
		"00030004",
		"00030005",
		"00030015",
		"00030017"
	};

	int freeSlots = getMenuSlots();

	DIR* dir;
	struct dirent* ent;

	for (int i = 0; i < NUM_OF_DIRS; i++)
	{
		char path[256];
		sprintf(path, "%s:/title/%s", sdnandMode ? "sd" : "nand", dirs[i]);

		dir = opendir(path);

		if (dir)
		{
			while ( (ent = readdir(dir)) != NULL )
			{
				if (strcmp(".", ent->d_name) == 0 || strcmp("..", ent->d_name) == 0)
					continue;

				if (ent->d_type == DT_DIR)
					freeSlots -= 1;
			}
		}

		closedir(dir);
	}

	return freeSlots;
}

//SD card
bool sdIsInserted()
{
	//Find a better way to do this.
	return true;
}

unsigned long long getSDCardSize()
{
	if (sdIsInserted())
	{
		struct statvfs st;
		if (statvfs("sd:/", &st) == 0)
			return st.f_bsize * st.f_blocks;
	}

	return 0;
}

unsigned long long getSDCardFree()
{
	if (sdIsInserted())
	{
		struct statvfs st;
		if (statvfs("sd:/", &st) == 0)
			return st.f_bsize * st.f_bavail;
	}

	return 0;
}

//internal storage
unsigned long long getDsiSize()
{
	//The DSi has 256MB of internal storage. Some is unavailable and used by other things.
	//An empty DSi reads 1024 open blocks
	return 1024 * BYTES_PER_BLOCK;
}

unsigned long long getDsiFree()
{
	u32 blockSize = getDsiClusterSize();

	//Get free space by subtracting file sizes in nand folders
	unsigned long long size = getDsiSize();
	unsigned long long appSize = getDirSize(sdnandMode ? "sd:/title/00030004" : "nand:/title/00030004", blockSize);

	//subtract, but don't go under 0
	if (appSize > size)
	{
		size = 0;
	}
	else
	{
		size -= appSize;
	}

	unsigned long long realFree = getDsiRealFree();

	return (realFree < size) ? realFree : size;
}

unsigned long long getDsiRealSize()
{
	struct statvfs st;
	if (statvfs(sdnandMode ? "sd:/" : "nand:/", &st) == 0)
		return st.f_bsize * st.f_blocks;

	return 0;
}

unsigned long long getDsiRealFree()
{
	struct statvfs st;
	if (statvfs(sdnandMode ? "sd:/" : "nand:/", &st) == 0)
		return st.f_bsize * st.f_bavail;

	return 0;
}

u32 getDsiClusterSize()
{
	struct statvfs st;
	if (statvfs(sdnandMode ? "sd:/" : "nand:/", &st) == 0)
		return st.f_bsize;

	return 0;
}
