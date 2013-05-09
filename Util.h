#ifndef SL_UTIL_H_
#define SL_UTIL_H_

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiApplicationEntryPoint.h>

/**
 * Waits for escape key to be pressed
 * @param systab System table
 * @return
 */VOID WaitForESC(EFI_SYSTEM_TABLE* systab);

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

/**
 * Allocate a block of memory.
 * @param size Size in bytes
 * @param systab Pointer to the system table.
 * @return Pointer to memory, or NULL on failure.
 */VOID* Allocate(UINTN size, EFI_SYSTEM_TABLE* systab);

/**
 * Frees memory.
 * @param mem
 * @param systab System table
 * @return
 */VOID Free(VOID* mem, EFI_SYSTEM_TABLE* systab);

#endif
