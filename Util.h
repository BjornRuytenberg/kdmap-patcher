#ifndef SL_UTIL_H_
#define SL_UTIL_H_

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiApplicationEntryPoint.h>

VOID WaitForESC(EFI_SYSTEM_TABLE* systab);

/**
 * Loads a file from the ESP. The caller is responsible for freeing the memory.
 * @param ImageHandle The current boot image handle
 * @param systab The system table
 * @param filename The file path to load.
 * @param[out] dataPtr Pointer to pointer to the buffer.
 * @param[out] size Pointer to the variable that will store the size.
 * @return an EFI_STATUS
 */
EFI_STATUS LoadFile(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* systab,
		CHAR16* filename, VOID** dataPtr, UINTN* size);

#endif
