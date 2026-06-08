/* sector0.h
 * This code was imported from https://github.com/DS-Homebrew/GodMode9i
 *
 * Changes against the source:
 *   - Documentation added
 *   - clean up / reorder
 *   - removed verbose output (must be handled on application level, not
 *     within the helpers */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <assert.h>

/************************ Constants / Defines *********************************/

#define SECTOR_SIZE              0x200

#define MBR_PARTITIONS           4
#define MBR_BOOTSTRAP_SIZE       (SECTOR_SIZE - (2 + MBR_PARTITIONS * 16))
#define MBR_SIGNATURE_0          0x55
#define MBR_SIGNATURE_1          0xAA

// https://3dbrew.org/wiki/NCSD#NCSD_header
#define NCSD_PARTITIONS          8
#define NCSD_SIGNATURESIZE       0x100
#define NCSD_HEADERSIZE          (NCSD_SIGNATURESIZE + 0x060)

#define NCSD_MAGIC               0x4453434e

/************************ Structures / Datatypes ******************************/

#ifdef _MSC_VER
	#pragma pack(push, 1)
	#define __PACKED
#elif defined __GNUC__
	#define __PACKED __attribute__ ((__packed__))
#endif

typedef struct {
	uint32_t offset, length;
} __PACKED ncsd_partition_t;

typedef struct {
	uint8_t digest[NCSD_SIGNATURESIZE];
} __PACKED ncsd_sginature;

typedef struct {
	ncsd_sginature signature;
	uint32_t magic, size;
	uint64_t media_id;
	uint8_t fs_types[NCSD_PARTITIONS], crypt_types[NCSD_PARTITIONS];
	ncsd_partition_t partitions[NCSD_PARTITIONS];
} __PACKED ncsd_header_t;

/*
 * CHS Sector Address Format
 *   See https://en.wikipedia.org/wiki/Master_boot_record#PTE
 *
 *  ==========================================================
 * | Offset | Field Length | Description                      |
 *  ==========================================================
 * |   1    |       3      | CHS Address of first sector      |
 * |        |              |    Bit 0..7:   Head              |
 * |        |              |    Bit 8..13:  Sector            |
 * |        |              |    Bit 14..23: Cylinder          |
 *  ----------------------------------------------------------
 */

typedef struct {
	uint8_t head;
	uint8_t sectorAndCylHigh;
	uint8_t cylinderLow;
} __PACKED chs_t;

/*
 * Partition table entries
 *   See https://en.wikipedia.org/wiki/Master_boot_record#PTE
 *
 *
 *  ==========================================================
 * | Offset | Field Length | Description                      |
 *  ==========================================================
 * |   0    |       1      | Bit 7: Bootable                  |
 * |        |              | Bit 0..6: Reserved (0)           |
 *  ----------------------------------------------------------
 * |   1    |       3      | CHS Address of first sector      |
 * |        |              |    Bit 0..7:   Head              |
 * |        |              |    Bit 8..13:  Sector            |
 * |        |              |    Bit 14..23: Cylinder          |
 *  ----------------------------------------------------------
 * |   4    |       1      | Partition Type                   |
 *  ----------------------------------------------------------
 * |   5    |       3      | CHS Address of last sector       |
 * |        |              |    Bit 0..7:   Head              |
 * |        |              |    Bit 8..13:  Sector            |
 * |        |              |    Bit 14..23: Cylinder          |
 *  ----------------------------------------------------------
 * |   8    |       4      | LBA Address of first sector      |
 *  ----------------------------------------------------------
 * |   12   |       4      | Number of sectors                |
 *  ----------------------------------------------------------
 */

typedef struct {
	uint8_t status;
	chs_t chs_first;
	uint8_t type;
	chs_t chs_last;
	uint32_t offset;
	uint32_t length;
} __PACKED mbr_partition_t;

/*
 * Master Boot Record
 *   https://en.wikipedia.org/wiki/Master_boot_record#Sector_layout
 *
 *  ==========================================================
 * | Offset | Field Length | Description                      |
 *  ==========================================================
 * |   0    |      446     | Boot code                        |
 *  ----------------------------------------------------------
 * |   446  |       16     | Partition 0                      |
 *  ----------------------------------------------------------
 * |   462  |       16     | Partition 1                      |
 *  ----------------------------------------------------------
 * |   478  |       16     | Partition 2                      |
 *  ----------------------------------------------------------
 * |   494  |       16     | Partition 3                      |
 *  ----------------------------------------------------------
 * |   510  |        2     | Signature ( 55 aa )              |
 *  ----------------------------------------------------------
 */

typedef struct {
	uint8_t bootstrap[MBR_BOOTSTRAP_SIZE];
	mbr_partition_t partitions[MBR_PARTITIONS];
	uint8_t boot_signature[2];
} __PACKED mbr_t;

#ifdef _MSC_VER
	#pragma pack(pop)
#endif
#undef __PACKED

/************************ Function Protoypes **********************************/

int parse_ncsd(const uint8_t sector0[SECTOR_SIZE]);

int parse_mbr(const uint8_t sector0[SECTOR_SIZE], const int is3DS);


/************************ static code verification ****************************/

static_assert(sizeof(ncsd_header_t) == NCSD_HEADERSIZE, "sizeof(ncsd_header_t) should equal 0x160");
static_assert(sizeof(mbr_t) == SECTOR_SIZE, "sizeof(mbr_t) should equal 0x200");

#ifdef __cplusplus
}
#endif
