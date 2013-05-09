#include "Util.h"
#include "Boot.h"
#include "Tables.h"

EFIAPI EFI_STATUS UefiMain(EFI_HANDLE ImageHandle,
		EFI_SYSTEM_TABLE* SystemTable)
{
	EFI_STATUS res;

	//load file
	void* data;
	UINTN size;

	res = LoadFile(ImageHandle, SystemTable,
			L"\\EFI\\Microsoft\\Boot\\slic.bin", &data, &size);

	if (res)
	{
		ErrorPrint(L"Failed to get new SLIC.\r\n");
	}
	else
	{
		PatchTables(ImageHandle, SystemTable, data, size);
	}

	res = Boot(ImageHandle, SystemTable);

	if (res)
	{
		ErrorPrint(L"Failed to boot. Error %d\r\n", res);
		WaitForESC(SystemTable);
	}

	return 0;
}
