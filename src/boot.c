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

#include "boot.h"
#include <Protocol/LoadedImage.h>
#include <Guid/GlobalVariable.h>
#include <Guid/ImageAuthentication.h>

// XXX: code borrowed from GPLv3 xen patch (modified)
void efi_shim_lock(VOID *Buffer, UINT32 Size, EFI_SYSTEM_TABLE *systab)
{
    static EFI_GUID shim_lock_guid = SHIM_LOCK_PROTOCOL_GUID;
    EFI_SHIM_LOCK_PROTOCOL *shim_lock;
    EFI_STATUS status;

    if ( !EFI_ERROR(systab->BootServices->LocateProtocol(&shim_lock_guid, NULL, 
                    (void **)&shim_lock)) &&
                    (status = shim_lock->Verify(Buffer, Size))
                    != EFI_SUCCESS ) {
        _Print(L"Image could not be verified (%i)", status);
    }
}
// XXX: end code borrowed from GPLv3 patch

BOOLEAN GetSecureBootState()
{
	UINT8 *SecureBoot = NULL;
	GetEfiGlobalVariable2(EFI_SECURE_BOOT_MODE_NAME, (VOID**)&SecureBoot, NULL);
	if ((SecureBoot != NULL) && (*SecureBoot == SECURE_BOOT_MODE_ENABLE)) {
		return TRUE;
	}
	return FALSE;
}

EFI_STATUS Boot(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *systab, CHAR16 boot_filename[])
{
	EFI_GUID LoadedImageProtocolGuid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
	EFI_STATUS res;
	BOOLEAN SbState = GetSecureBootState();
	
	_Print(L"Secure Boot state: %d\r\n", SbState);

	//load boot file
	void *data = NULL;
	UINTN data_size = 0;

	EFI_DEVICE_PATH_PROTOCOL *file_path;
	res = LoadFile(ImageHandle, systab, boot_filename, &data, &data_size,
				   &file_path);

	if (res == EFI_NOT_FOUND)
	{
		_Print(L"Could not find boot manager.\r\n");
		return EFI_NOT_FOUND;
	}
	else if (res)
	{
		_Print(L"Loading boot manager image failed.\r\n");
		return EFI_LOAD_ERROR;
	}

	//get this image's info
	EFI_LOADED_IMAGE_PROTOCOL *img_proto;
	res = systab->BootServices->HandleProtocol(ImageHandle, &LoadedImageProtocolGuid, (void **)&img_proto);

	if (res)
	{
		systab->BootServices->FreePool(data);
		_Print(L"Failed to get image info protocol. (Error %d)\r\n", res);
		return EFI_LOAD_ERROR;
	}

	if(SbState)
	{
		_Print(L"Pre-efi_shim_lock\n");
		efi_shim_lock(data, data_size, systab);
		_Print(L"Post-efi_shim_lock\n");
	}
	_Print(L"Pre-LoadImage\n");

	//Load the image
	EFI_HANDLE new_img_handle;
	res = systab->BootServices->LoadImage(FALSE, ImageHandle, NULL, data,
										  data_size, &new_img_handle);

	systab->BootServices->FreePool(data);

	if (res)
	{
		_Print(L"Failed to load new image. (Error %d)\r\n", res);
		return EFI_LOAD_ERROR;
	}

	//get new image info
	EFI_LOADED_IMAGE_PROTOCOL *new_img_proto;
	res = systab->BootServices->HandleProtocol(new_img_handle,
											   &LoadedImageProtocolGuid, (void **)&new_img_proto);

	if (res)
	{
		_Print(L"Failed to get new image information. (Error %d)\r\n", res);
		return EFI_LOAD_ERROR;
	}

	//set image data
	new_img_proto->DeviceHandle = img_proto->DeviceHandle;
	new_img_proto->ParentHandle = NULL;
	new_img_proto->FilePath = file_path;

	//Start the new image
	systab->BootServices->StartImage(new_img_handle, 0, NULL);

	return EFI_SUCCESS;
}
