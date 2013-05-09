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
