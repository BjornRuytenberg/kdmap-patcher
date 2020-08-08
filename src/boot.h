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

// From Xen: https://lists.xen.org/archives/html/xen-devel/2017-07/msg00981.html
// from Suse: https://build.opensuse.org/package/view_file/openSUSE:12.3/xen/26262-x86-EFI-secure-shim.patch?expand=1
void efi_shim_lock(VOID *Buffer, UINT32 Size, EFI_SYSTEM_TABLE *systab);
#define SHIM_LOCK_PROTOCOL_GUID { 0x605dab50, 0xe046, 0x4300, {0xab, 0xb6, 0x3d, 0xd8, 0x10, 0xdd, 0x8b, 0x23} }

typedef EFI_STATUS
(/* _not_ EFIAPI */ *EFI_SHIM_LOCK_VERIFY) (
    IN VOID *Buffer,
    IN UINT32 Size);

typedef struct {
    EFI_SHIM_LOCK_VERIFY Verify;
} EFI_SHIM_LOCK_PROTOCOL;
// End code import from GPL patches

#endif