#include "Util.h"
#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleFileSystem.h>
#include <Guid/FileInfo.h>

void WaitForESC(EFI_SYSTEM_TABLE* systab)
{
	for (;;)
	{
		UINTN idx;
		systab->BootServices->WaitForEvent(1, systab->ConIn->WaitForKey, &idx);

		EFI_INPUT_KEY key;

		systab->ConIn->ReadKeyStroke(systab->ConIn, &key);

		if (key.ScanCode == SCAN_ESC)
		{
			break;
		}
	}
}

void* Allocate(UINTN size, EFI_SYSTEM_TABLE* systab)
{
	void* data;
	EFI_STATUS res = systab->BootServices->AllocatePool(EfiLoaderData, size,
			&data);

	if (res)
	{
		return NULL ;
	}
	else
	{
		return data;
	}
}

void Free(void* mem, EFI_SYSTEM_TABLE* systab)
{
	systab->BootServices->FreePool(mem);
}

EFI_STATUS LoadFile(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* systab,
		CHAR16* filename, VOID** dataPtr, UINTN* size)
{
	EFI_GUID LoadedImageProtocolGuid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
	EFI_GUID FileInfoGuid = EFI_FILE_INFO_ID;
	EFI_GUID FileSystemGuid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
	EFI_STATUS res;

	EFI_BOOT_SERVICES* BS = systab->BootServices;

	//get image info
	EFI_LOADED_IMAGE_PROTOCOL* img_proto;
	res = BS->HandleProtocol(ImageHandle, &LoadedImageProtocolGuid,
			(void**) &img_proto);

	if (res)
	{
		ErrorPrint(L"Failed to get image protocol. (Error %d)\r\n", res);
		return EFI_LOAD_ERROR ;
	}

	EFI_HANDLE img_device_handle = img_proto->DeviceHandle;

	//Get filesystem protocol from device
	EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* fs_proto;
	res = BS->HandleProtocol(img_device_handle, &FileSystemGuid,
			(VOID**) &fs_proto);

	if (res)
	{
		ErrorPrint(L"Failed to get file system protocol. (Error %d)\r\n", res);
		return EFI_LOAD_ERROR ;
	}

	//open volume
	EFI_FILE_PROTOCOL* volume;
	res = fs_proto->OpenVolume(fs_proto, &volume);

	if (res)
	{
		ErrorPrint(L"Failed to open file volume. (Error %d)\r\n", res);
		return EFI_LOAD_ERROR ;
	}

	//open file
	EFI_FILE_PROTOCOL* file;
	res = volume->Open(volume, &file, filename, EFI_FILE_MODE_READ, 0);

	if (res)
	{
		//don't print error here
		//ErrorPrint(L"Failed to open file '%s'. (Error %d)\r\n", filename, res);
		return EFI_NOT_FOUND ;
	}

	//get file info, two try process
	EFI_FILE_INFO* file_info = NULL;
	UINTN file_info_size = 0;
	res = file->GetInfo(file, &FileInfoGuid, &file_info_size, NULL );

	if (res != EFI_BUFFER_TOO_SMALL )
	{
		ErrorPrint(L"Failed to stat file '%s'. (Error %d)\r\n", filename, res);
		return EFI_NOT_FOUND ;
	}

	res = BS->AllocatePool(EfiLoaderData, file_info_size, (void**) &file_info);

	if (res)
	{
		ErrorPrint(L"Failed to allocate file info memory. (Error %d)\r\n", res);
		return EFI_OUT_OF_RESOURCES ;
	}

	res = file->GetInfo(file, &FileInfoGuid, &file_info_size,
			(void*) file_info);

	if (res)
	{
		BS->FreePool(file_info);
		ErrorPrint(L"Failed to stat file '%s'. (Error %d)\r\n", filename, res);
		return EFI_NOT_FOUND ;
	}

	UINT64 file_size = file_info->FileSize;

	BS->FreePool(file_info);
	file_info = NULL;

	void* data = NULL;
	res = BS->AllocatePool(EfiLoaderData, file_size, (void**) &data);

	if (res)
	{
		ErrorPrint(L"Failed to allocate file data memory. (Error %d)\r\n", res);
		return EFI_OUT_OF_RESOURCES ;
	}

	//read the file
	res = file->Read(file, &file_size, (void*) data);

	if (res)
	{
		BS->FreePool(data);
		ErrorPrint(L"Failed to read file '%s'. (Error %d)\r\n", filename, res);
		return EFI_NOT_FOUND ;
	}

	//close the file
	file->Close(file);
	volume->Close(volume);

	//set the pointer and data size
	*dataPtr = data;
	*size = file_size;

	//return success
	return EFI_SUCCESS;
}
