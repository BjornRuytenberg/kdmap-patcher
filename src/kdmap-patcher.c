/*
 *  Kernel DMA Protection Patcher (kdmap-patcher)
 *  Copyright (C) 2020 Björn Ruytenberg <bjorn@bjornweb.nl>
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

#include "boot.h"
#include "tables.h"

// Basic UEFI Libraries
#include <Library/UefiApplicationEntryPoint.h>
#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>

// Boot and Runtime Services
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#ifdef _MSC_VER // VisualUefi only
	extern CONST UINT32 _gUefiDriverRevision = 0;
	CHAR8 *gEfiCallerBaseName = "kdmap-patcher";
#endif

#define WINDOWS_BOOTLOADER L"\\EFI\\Microsoft\\Boot\\bootmgfworg.efi";
#define UBUNTU_BOOTLOADER L"\\EFI\\ubuntu\\grubx64.efi";

CHAR16 BOOT_OS[] = WINDOWS_BOOTLOADER; // Choose from WINDOWS_BOOTLOADER, UBUNTU_BOOTLOADER or insert your own

#ifdef _MSC_VER // VisualUefi only
EFI_STATUS EFIAPI UefiUnload(IN EFI_HANDLE ImageHandle)
{
	// This code should be compiled out and never called 
	ASSERT(FALSE);
}
#endif

EFI_STATUS EFIAPI UefiMain(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable)
{
	EFI_STATUS res;

	_clearFb(SystemTable);
	_Print(L"Kernel DMA Protection Patcher\r\nhttps://thunderspy.io\r\n\r\n");

	res = ParseDmarTable(ImageHandle, SystemTable);
	if (res == EFI_SUCCESS)
	{
		_Print(L"Found DMAR table.\r\n");

		res = PatchDmarTable(ImageHandle, SystemTable);
		if (res == EFI_SUCCESS)
		{
			_Print(L"Succesfully patched DMAR table to enable Kernel DMA Protection.\r\n");

			// _Print again to verify
			ParseDmarTable(ImageHandle, SystemTable);
		}
		else
		{
			_Print(L"Kernel DMA Protection already enabled. No need to patch.\r\n");
		}
	}
	else
	{
		_Print(L"System does not provide DMAR table (no IOMMU?) - aborting. Press ENTER to continue.\r\n");
		SystemTable->BootServices->Stall(2000000);
	}
	
	//boot
	res = Boot(ImageHandle, SystemTable, BOOT_OS);

	if (res)
	{
		_Print(L"Could not find boot loader image.\r\n");
		return EFI_NOT_FOUND;
	}

	_clearFb(SystemTable);
	return res;
}
