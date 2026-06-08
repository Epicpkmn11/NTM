/* sector0.c
 * This code was imported from https://github.com/DS-Homebrew/GodMode9i
 *
 * Changes against the source:
 *   - Documentation added
 *   - clean up formatting
 *   - moved magic numbers to defines from sector0.h
 *   - fixed parse_mbr to return valid even when signature was invalid but
 *     the bootstrap region was not all zero. (? Why was this code there ?)
 *   - removed verbose output (must be handled on application level, not
 *     within the helpers
 */

#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "sector0.h"

/************************ Constants / Defines *********************************/

static const mbr_partition_t ptable_DSi[MBR_PARTITIONS] = {
	{0u, {3u, 24u, 4u}, 6u, {15u, 224u, 59u}, 0x00000877u, 0x00066f89u},
	{0u, {2u, 206u, 60u}, 6u, {15u, 224u, 190u}, 0x0006784u, 0x000105b3u},
	{0u, {2u, 222u, 191u}, 1u, {15u, 224u, 191u}, 0x00077e5u, 0x000001a3u},
	{0u, {0u, 0u, 0u}, 0u, {0u, 0u, 0u}, 0u, 0u}
};

static const mbr_partition_t ptable_3DS[MBR_PARTITIONS] = {
	{0u, {4u, 24u, 0u}, 6u, {1u, 160u, 63u}, 0x0000009u, 0x00047da9u},
	{0u, {4u, 142u, 64u}, 6u, {1u, 160u, 195u}, 0x0004808u, 0x000105b3u},
	{0u, {0u, 0u, 0u}, 0u, {0u, 0u, 0u}, 0u, 0u},
	{0u, {0u, 0u, 0u}, 0u, {0u, 0u, 0u}, 0u, 0u}
};

/************************ Functions *******************************************/

/*! \brief Sanity check of the NCSD
 *
 *  The ncsd is checked for
 *    - the signature magic
 *    - the partition types
 *  to ensure a valid 3DS ncsd is present
 *
 *  Return values:
 *     0: NCSD is a valid
 *    -1: the signature/magic is invalid
 *    -2: at least one unknown partition type was found
 */
int parse_ncsd(const uint8_t sector0[SECTOR_SIZE])
{
	const ncsd_header_t * h = (ncsd_header_t *)sector0;
	if (NCSD_MAGIC != h->magic)
	{
		return -1;
	}

	for (unsigned i = 0; i < NCSD_PARTITIONS; ++i)
	{
		unsigned fs_type = h->fs_types[i];
		if (fs_type == 0)
		{
			break;
		}
		switch (fs_type)
		{
			case 1:
			case 3:
			case 4:
				break;
			default:
				return -2;
		}
	}
	return 0;
}

/*! \brief Sanity check of the MBR
 *
 *  The master boot record is checked for
 *    - the signature
 *    - the partition0 values
 *  to ensure a valid DSi main partition can be found
 *
 *  Return values:
 *     0: MBR is a valid DSi partition
 *    -1: the signature is invalid
 *    -2: the first partition does not match expected values
 */
int parse_mbr(const uint8_t sector0[SECTOR_SIZE], const int is3DS)
{
	const mbr_t *m = (mbr_t*)sector0;
	const mbr_partition_t *ref_ptable; // reference partition table

	if ((MBR_SIGNATURE_0 != m->boot_signature[0]) || (MBR_SIGNATURE_1 != m->boot_signature[1]))
	{
		// if the signature is invalid, the bootsector shall not be used!
		return -1;
	}
	ref_ptable = is3DS?ptable_3DS:ptable_DSi;
	// only test the 1st partition now, we've seen variations on the 3rd partition
	// and after all we only care about the 1st partition
	if (memcmp(ref_ptable, m->partitions, sizeof(mbr_partition_t)))
	{
		return -2;
	}
	return 0;
}
