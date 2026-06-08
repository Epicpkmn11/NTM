#ifndef ROM_H
#define ROM_H

#include <nds/ndstypes.h>
#include <nds/memory.h>

tDSiHeader* getRomHeader(char const* fpath);
tNDSBanner* getRomBanner(char const* fpath);

bool getGameTitle(tNDSBanner* b, char* out, bool full);
bool getGameTitlePath(char const* fpath, char* out, bool full);

bool getRomLabel(tDSiHeader* h, char* out);
bool getRomCode(tDSiHeader* h, char* out);

void printRomInfo(char const* fpath);

unsigned long long getRomSize(char const* fpath);

bool romIsCia(char const* fpath);
bool isDsiHeader(tDSiHeader* h);

#endif