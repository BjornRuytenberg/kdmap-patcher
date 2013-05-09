#ifndef SL_TABLES_H_
#define SL_TABLES_H_

#include <Uefi.h>
#include <Library/UefiLib.h>

/**
 * @file Tables.h
 *
 * Contains functions that work with ACPI tables.
 */

/**
 * The maximum number of tables in the stack.
 */
#define TABLE_STACK_SIZE 256

/**
 * This struct keeps track of ACPI tables.
 *
 * It should be able to dynamically expand, but I'm lazy.
 */
typedef struct _TABLE_STACK
{
	VOID* Tables[TABLE_STACK_SIZE];
	UINTN TableCount;
} TABLE_STACK;

/**
 * Structure of the ACPI RSDP table.
 */
typedef struct _RSDP_TABLE
{
	CHAR8 Signature[8];
	UINT8 Checksum;
	CHAR8 OEMID[6];
	UINT8 Revision;
	UINT32 RSDT;
	UINT32 Length;
	UINT64 XSDT;
	UINT8 ExtendedChecksum;
} RSDP_TABLE;

/**
 * Structure of the ACPI System Description table header
 */
typedef struct _SD_HEADER
{
	CHAR8 Signature[4];
	UINT32 Length;
	UINT8 Revision;
	UINT8 Checksum;
	CHAR8 OEMID[6];
	CHAR8 OEMTableID[8];
	UINT32 OEMRevision;
	CHAR8 CreatorID[4];
	CHAR8 CreatorRevision[4];
} SD_HEADER;
//36 bytes, 0-35

/**
 * Size of the system description table header
 */
#define SD_HEADER_SIZE 36

#endif
