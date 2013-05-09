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
