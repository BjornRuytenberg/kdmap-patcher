/*
 *  SLICLoader, an EFI program to modify ACPI tables before boot.
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

#include "Util.h"
#include "Boot.h"
#include "Tables.h"

EFIAPI EFI_STATUS UefiMain(EFI_HANDLE ImageHandle,
		EFI_SYSTEM_TABLE* SystemTable)
{
	EFI_STATUS res;

	//Attempt to load SLIC data
	CHAR16 slic_filename[] = L"\\EFI\\Microsoft\\Boot\\slic.bin";
	void* slic_data;
	UINTN slic_size;
	res = LoadFile(ImageHandle, SystemTable, slic_filename, &slic_data,
			&slic_size, NULL );

	if (res)
	{
		//load file failed
		ErrorPrint(L"Warning: Failed to find slic.bin.\r\n");
		SystemTable->BootServices->Stall(2000000);
	}
	else
	{
		//patch tables
		res = PatchTables(ImageHandle, SystemTable, slic_data, slic_size);
		if (res)
		{
			ErrorPrint(L"Warning: Failed to patch SLIC table.\r\n");
			SystemTable->BootServices->Stall(2000000);
		}
	}

	//boot
	res = Boot(ImageHandle, SystemTable);

	if (res)
	{
		ErrorPrint(L"Error: Could not load new boot manager.\r\n");
		ErrorPrint(L"\r\nPress ESC to exit.\r\n\r\n");
		WaitForESC(SystemTable);
		return EFI_NOT_FOUND ;
	}

	return EFI_SUCCESS;
}
