#include "message.h"
#include <nds/ndstypes.h>

static int writingLocked = true;

bool nandio_lock_writing()
{
	writingLocked = true;

	return writingLocked;
}

bool nandio_unlock_writing()
{
	if (writingLocked && randomConfirmBox("Writing to NAND is locked!\nIf you're sure you understand\nthe risk, input the sequence\nbelow."))
		writingLocked = false;

	return !writingLocked;
}
