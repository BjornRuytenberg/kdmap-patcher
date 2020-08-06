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

#ifndef BOOT_H_
#define BOOT_H_

#include <Uefi.h>
#include <Library/UefiLib.h>

#include "util.h"

EFI_STATUS Boot(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *systab, CHAR16 boot_filename[]);

#endif
