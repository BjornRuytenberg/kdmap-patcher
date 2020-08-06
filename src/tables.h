/*
 *  Kernel DMA Protection Patcher (kdmap-patcher)
 *  Copyright (C) 2020 Björn Ruytenberg <bjorn@bjornweb.nl>
 *
 *  SLICLoader
 *  Copyright (C) 2013 wweber
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TABLES_H_
#define TABLES_H_

#include "util.h"
#include <Uefi.h>
#include <Library/UefiLib.h>

typedef struct _DESCR_HEADER
{
	CHAR8	Signature[4];
	UINT32	Length;
	UINT8	Revision;
	UINT8	Checksum;
	CHAR8	OEMID[6];
	CHAR8	OEMTableID[8];
	UINT32	OEMRevision;
	CHAR8	CreatorID[4];
	CHAR8	CreatorRevision[4];
} DESCR_HEADER;

#define DESCR_HEADER_SIZE sizeof(DESCR_HEADER)

typedef struct _DMAR_HEADER {
	struct _DESCR_HEADER Header;
	UINT8   HostAddressWidth;
	UINT8   Flags;
	UINT8   Reserved[10];
} DMAR_HEADER;

// DMAR Flags
#define DMAR_INTR_REMAP						0
#define DMAR_X2APIC_OPT_OUT					1
#define DMAR_DMA_CTRL_PLATFORM_OPT_IN_FLAG	2 // DMAR_PLATFORM_OPT_IN in 'linux/include/linux/dmar.h'

typedef struct _RSDP_HEADER
{
	CHAR8 Signature[8];
	UINT8 Checksum;
	CHAR8 OEMID[6];
	UINT8 Revision;
	UINT32 RSDT;
	UINT32 Length;
	UINT64 XSDT;
	UINT8 ExtendedChecksum;
} RSDP_HEADER;

DESCR_HEADER* Int32ToTable(UINT32 i);
DESCR_HEADER* Int64ToTable(UINT64 i);
DESCR_HEADER* GetRSDT(RSDP_HEADER* rsdp);
DESCR_HEADER* GetXSDT(RSDP_HEADER* rsdp);
UINT32* GetRSDTTables(DESCR_HEADER* rsdt, UINTN* count);
UINT64* GetXSDTTables(DESCR_HEADER* xsdt, UINTN* count);
UINT8 ComputeChecksum(UINT8* data, UINTN count, UINTN exclude);
BOOLEAN IsTableType(DESCR_HEADER* table, CHAR8* sig);
EFI_STATUS ParseDmarTable(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SysTab);
EFI_STATUS PatchDmarTable(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SysTab);

#endif
