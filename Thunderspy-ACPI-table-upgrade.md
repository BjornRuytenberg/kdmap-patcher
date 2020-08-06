# Thunderspy ACPI Table Upgrade
## Summary
Kernel DMA Protection helps keep your computer secure by mediating all Thunderbolt Direct Memory Access operations through an IOMMU. Many systems produced after 2013 have an IOMMU, but their BIOS does not enable Kernel DMA Protection. In the absence of vendor BIOS updates retroactively adding this protection, this leaves about 9 years worth of systems vulnerable to [Thunderspy](https://thunderspy.io) forever. This project aims to bring Kernel DMA Protection to systems that do not ship Kernel DMA Protection, but are in fact technically capable.

## Introduction
In our vulnerability disclosure procedure for Thunderspy, Intel has stated that [Kernel DMA Protection](https://thunderspy.io/#kernel-dma-protection) mitigates the Thunderspy vulnerabilities. While this protection may address the DMA attack vector portion of Thunderspy, it is only available on a limited number of systems shipping since 2019. Hence, all systems released between 2011-2019, and more recent systems that do not ship Kernel DMA Protection, will remain fully vulnerable to Thunderspy forever.

Starting from [Haswell](https://en.wikipedia.org/wiki/Haswell_(microarchitecture)) (2013), a lot of Intel CPUs are IOMMU-equipped. Hence, systems shipping these CPUs should technically be capable of supporting DMA remapping Thunderbolt devices through Kernel DMA Protection. However, in the absence of vendor BIOS updates, this protection has never been available for any pre-2019 systems.

As part of our on-going research, we present two options that aim to bring Kernel DMA Protection to Thunderbolt-equipped systems that do not ship Kernel DMA Protection, but do satisfy all hardware and firmware requirements. These options currently include:

- **kdmap-patcher**: An experimental, OS-agnostic UEFI extension that serves as a drop-in patch requiring no changes to the operating system. [Access the project repo](../../).
- **Upgrading ACPI tables via initrd (this document)**: A guide outlining how to manually patch the ACPI DMAR table on Linux.

# Upgrading ACPI tables via initrd (Linux only)
1. Obtain a raw dump of your system's DMAR table: `# cat /sys/firmware/acpi/tables/DMAR > /tmp/dmar.bin`
2. Obtain `acpica-tools` using your distro's package manager. For example, on Debian/Ubuntu: `# apt-get install acpica-tools`
3. Run iasl on the raw dump:

		$ iasl -d /tmp/dmar.bin
		
		Intel ACPI Component Architecture
		ASL+ Optimizing Compiler/Disassembler version 20200528
		Copyright (c) 2000 - 2020 Intel Corporation
		
		File appears to be binary: found 133 non-ASCII characters, disassembling
		Binary file appears to be a valid ACPI table, disassembling
		Input file dmar.bin, Length 0xA8 (168) bytes
		ACPI: DMAR 0x0000000000000000 0000A8 (v01 INTEL  EDK2     00000003 INTL 20200528)
		Acpi Data Table [DMAR] decoded
		Formatted output:  dmar.dsl - 4997 bytes

4. Using your favorite text editor, open `dmar.dsl`. The output should look similar to the following:
		
		$ cat /tmp/dmar.dsl
		
		/*
		 * Intel ACPI Component Architecture
		 * AML/ASL+ Disassembler version 20180105 (64-bit version)
		 * Copyright (c) 2000 - 2018 Intel Corporation
		 * 
		 * Disassembly of dmar-org.bin, Sun Apr  5 15:51:13 2020
		 *
		 * ACPI Data Table [DMAR]
		 *
		 * Format: [HexOffset DecimalOffset ByteLength]  FieldName : FieldValue
		 */

		[000h 0000   4]                    Signature : "DMAR"    [DMA Remapping table]
		[004h 0004   4]                 Table Length : 000000A8
		[008h 0008   1]                     Revision : 01
		[009h 0009   1]                     Checksum : F5
		[00Ah 0010   6]                       Oem ID : "INTEL "
		[010h 0016   8]                 Oem Table ID : "EDK2    "
		[018h 0024   4]                 Oem Revision : 00000002
		[01Ch 0028   4]              Asl Compiler ID : "    "
		[020h 0032   4]        Asl Compiler Revision : 01000013
	
		[024h 0036   1]           Host Address Width : 26
		[025h 0037   1]                        Flags : 01
		[026h 0038  10]                     Reserved : 00 00 00 00 00 00 00 00 00 00
	
		[030h 0048   2]                Subtable Type : 0000 [Hardware Unit Definition]
		[032h 0050   2]                       Length : 0018
		(..snip..)

5. The `Flags` attribute on offset 25h [denotes](https://github.com/tianocore/edk2/blob/master/MdePkg/Include/IndustryStandard/DmaRemappingReportingTable.h#L31) which DMA remapping features have been enabled. For Kernel DMA Protection to function, we need both [interrupt remapping](https://github.com/tianocore/edk2/blob/master/MdePkg/Include/IndustryStandard/DmaRemappingReportingTable.h#L29) and [DMA control platform opt-in](https://github.com/tianocore/edk2/blob/master/MdePkg/Include/IndustryStandard/DmaRemappingReportingTable.h#L31) to be enabled. To assert these bits, change `Flags` value to `05`.

6. Increase the `Oem Revision` value to have the kernel apply the modified table. For example, change `00000002` to `00000003`.

7. Compile the DMAR table:

		$ iasl /tmp/dmar.dsl 
		
		Intel ACPI Component Architecture
		ASL+ Optimizing Compiler/Disassembler version 20200528
		Copyright (c) 2000 - 2020 Intel Corporation
		
		Table Input:   dmar.dsl -    4975 bytes     66 fields      104 source lines
		Binary Output: dmar.aml -     168 bytes

8. To use the custom DMAR table, create a CPIO archive that is loaded by your bootloader. This is a one-time procedure; updating the kernel will not make it necessary to repeat these steps. First, create the following folder structure: `$ mkdir -p kernel/firmware/acpi`

9. Copy the patched DMAR table into the just created folder. For example:
`$ cp dmar.aml kernel/firmware/acpi`

10.  Within the same folder where the newly created `kernel/` folder resides, create the CPIO archive containing the patched DMAR table:
`$ find kernel | cpio -H newc --create > acpi_override`

11. Copy the CPIO archive to the boot directory:
`# cp acpi_override /boot`

12. Finally, configure your bootloader to load the CPIO archive.
	- If using _GRUB2_, create a file `/etc/grub.d/09_custom` with these contents (adjust as necessary):

			#!/bin/sh
			exec tail -n +3 $0
			# This file provides an easy way to add custom menu entries.  Simply type the
			# menu entries you want to add after this comment.  Be careful not to change
			# the 'exec tail' line above.
			
			menuentry 'Ubuntu' {
			    linux (hd0,msdos1)/vmlinuz root=/dev/sda1
			    initrd /acpi_override
			    initrd (hd0,msdos1)/initrd.img
			}

	- If using _systemd-boot_, update `/boot/loader/entries/arch.conf` to (adjust as necessary):
	
			title  Arch Linux
			linux  /vmlinuz-linux
			initrd /acpi_override
			initrd /initramfs-linux.img
			options  root=PARTUUID=ec9d5998-a9db-4bd8-8ea0-35a45df04701 resume=PARTUUID=58d0aa86-d39b-4fe1-81cf-45e7add275a0 ...

13. Reboot and verify the result:
	- Run `$ dmesg | grep ACPI` and look for clues that suggest a DMAR table override.
	- Observe that `cat /sys/bus/thunderbolt/devices/domain0/iommu_dma_protection` returns the value 1. All done!

Special thanks to the [Arch Linux](https://wiki.archlinux.org/index.php/DSDT#Using_modified_code) wiki, on which steps 9-13 are based.