#include "Util.h"
#include "Boot.h"

EFIAPI EFI_STATUS UefiMain(EFI_HANDLE ImageHandle,
		EFI_SYSTEM_TABLE* SystemTable)
{
	EFI_STATUS res = Boot(ImageHandle, SystemTable);

	if (res)
	{
		ErrorPrint(L"Failed to boot. Error %d\r\n", res);
		WaitForESC(SystemTable);
	}

	return 0;
}
