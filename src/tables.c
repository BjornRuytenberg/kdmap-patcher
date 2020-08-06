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

#include "tables.h"
#include <Guid/Acpi.h>

DESCR_HEADER *Int32ToTable(UINT32 i)
{
	return (DESCR_HEADER *)(UINTN)i;
}

DESCR_HEADER *Int64ToTable(UINT64 i)
{
	return (DESCR_HEADER *)(UINTN)i;
}

DESCR_HEADER *GetRSDT(RSDP_HEADER *rsdp)
{
	return Int32ToTable(rsdp->RSDT);
}

DESCR_HEADER *GetXSDT(RSDP_HEADER *rsdp)
{
	return Int64ToTable(rsdp->XSDT);
}

UINT32 *GetRSDTTables(DESCR_HEADER *rsdt, UINTN *count)
{
	*count = (rsdt->Length - DESCR_HEADER_SIZE) / sizeof(UINT32);
	return (UINT32 *)(rsdt + DESCR_HEADER_SIZE);
}

UINT64 *GetXSDTTables(DESCR_HEADER *xsdt, UINTN *count)
{
	*count = (xsdt->Length - DESCR_HEADER_SIZE) / sizeof(UINT64);
	return (UINT64 *)((UINTN)xsdt + DESCR_HEADER_SIZE);
}

UINT8 ComputeChecksum(UINT8 *data, UINTN count, UINTN exclude)
{
	UINT8 sum = 0;
	UINTN i;
	for (i = 0; i < count; ++i)
	{
		if (i == exclude)
		{
			continue;
		}

		UINT8 val = *((UINT8 *)(data + i));
		sum = (sum + val) % 256;
	}

	return (256 - sum) % 256;
}

BOOLEAN IsTableType(DESCR_HEADER *table, CHAR8 *sig)
{
	return table->Signature[0] == sig[0] && table->Signature[1] == sig[1] && table->Signature[2] == sig[2] && table->Signature[3] == sig[3];
}

DESCR_HEADER* GetAcpiTable(char* sig)
{
	EFI_GUID rsdp_2_guid = EFI_ACPI_20_TABLE_GUID;
	EFI_GUID rsdp_1_guid = EFI_ACPI_TABLE_GUID;
	EFI_STATUS res;

	UINTN i;
	UINTN acpi_version = 0;
	DESCR_HEADER *tab = NULL;

	// Get RSDP and revision
	RSDP_HEADER *rsdp;
	res = EfiGetSystemConfigurationTable(&rsdp_2_guid, (void **)&rsdp);

	if (res)
	{
		res = EfiGetSystemConfigurationTable(&rsdp_1_guid, (void **)&rsdp);

		if (res)
		{
			_Print(L"Cannot find RSDP. Aborting.\r\n");
			return NULL;
		}
	}

	if (acpi_version == 1)
	{
		//RSDT only
		DESCR_HEADER *rsdt = GetRSDT(rsdp);

		UINTN table_count = 0;
		UINT32 *tables = GetRSDTTables(rsdt, &table_count);

		// Look up table
		for (i = 0; i < table_count; ++i)
		{
			tab = (void *)Int32ToTable(tables[i]);
			if (IsTableType(tab, sig))
			{
				return tab;
			}
		}
	}
	else // acpi_version >= 2
	{
		//XSDT only
		DESCR_HEADER *xsdt = GetXSDT(rsdp);

		UINTN table_count = 0;
		UINT64 *tables = GetXSDTTables(xsdt, &table_count);

		// Look up table
		for (i = 0; i < table_count; ++i)
		{
			tab = (void *)Int64ToTable(tables[i]);
			if (IsTableType(tab, sig))
			{
				return tab;
			}
		}
	}

	return NULL;
}

EFI_STATUS ParseDmarTable(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SysTab)
{
	DMAR_HEADER* dmar = (DMAR_HEADER*)GetAcpiTable("DMAR");
	if (dmar)
	{
		_Print(L"ACPI DMAR Header:\n");
		_Print(L" HostAddressWidth: 0x%x (%u)\n", dmar->HostAddressWidth, dmar->HostAddressWidth);
		_Print(L" Flags: 0x%x (%u)\n", dmar->Flags, dmar->Flags);
		_Print(L"  DMAR_INTR_REMAP: %u\n", (dmar->Flags >> DMAR_INTR_REMAP) & 1U);
		_Print(L"  DMAR_X2APIC_OPT_OUT: %u\n", (dmar->Flags >> DMAR_X2APIC_OPT_OUT) & 1U);
		_Print(L"  DMAR_DMA_CTRL_PLATFORM_OPT_IN_FLAG: %u\n", (dmar->Flags >> DMAR_DMA_CTRL_PLATFORM_OPT_IN_FLAG) & 1U);

		return EFI_SUCCESS;
	}

	return EFI_NOT_FOUND;
}

EFI_STATUS PatchDmarTable(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SysTab)
{
	DMAR_HEADER* dmar = (DMAR_HEADER*) GetAcpiTable("DMAR");
	if (dmar)
	{
		// Test current value
		BOOLEAN kdmap_enabled = (dmar->Flags >> DMAR_DMA_CTRL_PLATFORM_OPT_IN_FLAG) & 1U;
		if (kdmap_enabled)
		{
			return EFI_NOT_STARTED;
		}

		// Set Kernel DMA Protection opt-in flag
		dmar->Flags |= 1UL << DMAR_DMA_CTRL_PLATFORM_OPT_IN_FLAG;

		// Update checksum
		DESCR_HEADER * dmar_descr_header = (DESCR_HEADER *)dmar;
		dmar_descr_header->Checksum = ComputeChecksum((void *)dmar_descr_header, dmar_descr_header->Length, 9);
		return EFI_SUCCESS;
	}
	return EFI_NOT_FOUND;
}
