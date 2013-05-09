#ifndef SL_BOOT_H_
#define SL_BOOT_H_

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiApplicationEntryPoint.h>

#include "Util.h"

/**
 * @file Boot.h
 *
 * Handles loading another boot manager.
 */

/**
 * Loads and starts the standard boot manager.
 * @param ImageHandle The current image handle.
 * @param SystemTable The system table.
 * @return
 */
EFI_STATUS Boot(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable);

#endif
