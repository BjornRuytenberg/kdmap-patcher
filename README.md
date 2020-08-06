# Kernel DMA Protection Patcher (kdmap-patcher)

## Summary
Kernel DMA Protection helps keep your computer secure by mediating all Thunderbolt Direct Memory Access operations through an IOMMU. Many systems produced after 2013 have an IOMMU, but their BIOS does not enable Kernel DMA Protection. In the absence of vendor BIOS updates retroactively adding this protection, this leaves about 9 years worth of systems fully vulnerable to [Thunderspy](https://thunderspy.io) forever. This project aims to bring Kernel DMA Protection to systems that do not ship Kernel DMA Protection, but are in fact technically capable.

## Introduction
In our vulnerability disclosure procedure for Thunderspy, Intel has stated that [Kernel DMA Protection](https://thunderspy.io/#kernel-dma-protection) mitigates the Thunderspy vulnerabilities. While this protection may address the DMA attack vector portion of Thunderspy, it is only available on a limited number of systems shipping since 2019. Hence, all systems released between 2011-2019, and more recent systems that do not ship Kernel DMA Protection, will remain fully vulnerable to Thunderspy forever.

Starting from [Haswell](https://en.wikipedia.org/wiki/Haswell_(microarchitecture)) (2013), a lot of Intel CPUs are IOMMU-equipped. Hence, systems shipping these CPUs should technically be capable of supporting DMA remapping Thunderbolt devices through Kernel DMA Protection. However, in the absence of vendor BIOS updates, this protection has never been available for any pre-2019 systems.

As part of our on-going research, we present two options that aim to bring Kernel DMA Protection to Thunderbolt-equipped systems that do not ship Kernel DMA Protection, but do satisfy all hardware and firmware requirements. These options currently include:

- **kdmap-patcher (this project)**: An experimental, OS-agnostic UEFI extension that serves as a drop-in patch requiring no changes to the operating system.
- **Upgrading ACPI tables via initrd**: A guide outlining how to [manually patch the ACPI DMAR table on Linux](Thunderspy-ACPI-table-upgrade.md).


## Requirements
kdmap-patcher requires:

- Windows 10 build 1803 or later; Linux kernel 5.0 or later
- A UEFI (BIOS) implementation compliant with UEFI specification 2.3 (2011) or later
- An Intel system featuring an IOMMU and Thunderbolt controller

**Please note**: While we have verified kdmap-patcher to be working flawlessly on various Thunderbolt-equipped systems, it is currently in an experimental stage and may therefore not yet be suitable for use in production. We would appreciate your feedback. Thank you!

## Usage

### Windows 10 build 1803 or later

- [Build](#using-visualuefi) a copy of kdmap-patcher.
- Open a Command Prompt with administrator privileges.
- Mount the EFI system partition: `mountvol T: /S`.
- From the release ZIP or your build folder, copy `kdmap-patcher.efi` to your EFI System Partition: `copy kdmap-patcher.efi T:\EFI\Microsoft\Boot`
- Rename the Windows bootloader: `ren T:\EFI\Microsoft\Boot\bootmgfw.efi T:\EFI\Microsoft\Boot\bootmgfworg.efi`
- Rename `kdmap-patcher.efi` to take the Windows bootloader's filename: `ren T:\EFI\Microsoft\Boot\kdmap-patcher.efi T:\EFI\Microsoft\Boot\bootmgfw.efi`
- Reboot your system. Observe that kdmap-patcher hotpatches the DMA remapping opt-in flag in memory, and subsequently loads Windows.
- Optionally, using `msinfo32`, confirm Kernel DMA Protection has been enabled. All done!

**TODO**

- The steps above only work when Secure Boot has been disabled. It is currently unclear whether non-enterprise users can sign UEFI binaries using their own keys. Possibly worth reading: https://docs.microsoft.com/en-us/windows-hardware/manufacture/desktop/windows-secure-boot-key-creation-and-management-guidance
- Possibly make an installer for easier deployment.

### Linux kernel 5.0 or later

- [Build](#using-edk2) a copy of kdmap-patcher.
- From the release ZIP or your build folder, copy `kdmap-patcher.efi` to the folder `/boot/efi/EFI/boot` using root privileges.
- Depending on your boot configuration, in the latter folder, you can either choose to:
	- Configure your default boot entry in UEFI to reference `kdmap-patcher.efi` (recommended if supported by your UEFI), or
	- Replace `bootx64.efi` with `kdmap-patcher.efi`.
- Reboot your system. Observe that kdmap-patcher hotpatches the DMA remapping opt-in flag in memory, and subsequently loads the bootloader of your choice.
- Optionally, observe that `$ cat /sys/bus/thunderbolt/devices/domain0/iommu_dma_protection` returns the value `1`. All done!

**TODO**

- Add steps for Secure Boot use case, i.e. signing using one's own keys.
- Add optional steps to use Heads for measured boot.

## Building
kdmap-patcher can be built against EDK2 and VisualUefi. Other toolchains, such as GNU-EFI and LLVM using the barebones UEFI target, may work but have not been tested.

### Using EDK2
[EDK2](https://github.com/tianocore/edk2/), maintained by Tianocore, is a reference UEFI development toolchain.

- Clone kdmap-patcher, including all of its dependencies: `$ git clone --recurse-submodules -j4 https://github.com/BjornRuytenberg/kdmap-patcher.git`
- Install all EDK2 build dependencies. For example, on Ubuntu/Debian: `sudo apt-get install -y build-essential uuid-dev iasl git gcc nasm python3-distutils`
- Build EDK2: `make linux-edk2`
- Set your bootloader path in [kdmap-patcher.c#L42](https://github.com/BjornRuytenberg/kdmap-patcher/blob/master/src/kdmap-patcher.c#L42).
- Build kdmap-patcher: `make linux-efi-payload`
- Obtain the UEFI binary `kdmap-patcher.efi` from the `build` folder.
	- To install, run `make linux-efi-menu-install`. Alternatively, refer to the [Usage](#linux-kernel-50-or-later) section.
- Optionally, to debug in a QEMU instance:
	- Ensure you have all dependencies installed for QEMU. For example, on Ubuntu/Debian: `sudo apt-get install qemu-system-x86 ovmf`
	- Execute `make run`
- To uninstall, simply run `make linux-efi-menu-uninstall`.

### Using VisualUefi
[VisualUefi](https://github.com/ionescu007/VisualUefi), maintained by Alex Ionescu, is a project that aims to significantly improve the EDK2 development and debugging experience in Visual Studio.

- Clone VisualUefi, including all of its dependencies: `git clone --recurse-submodules https://github.com/ionescu007/VisualUefi.git`
- Set up the build environment: https://github.com/ionescu007/VisualUefi#installation
- Clone kdmap-patcher into the `VisualUefi\samples` folder: `git clone https://github.com/BjornRuytenberg/kdmap-patcher.git`
- Open the samples solution `VisualUefi\samples\samples.sln`. Add `kdmap-patcher.vcxproj` from `VisualUefi\samples\kdmap-patcher\src` to the solution.
- Build the samples solution.
- Obtain the UEFI binary `kdmap-patcher.efi` from `VisualUefi\samples\x64\Release`.
	- To install, please refer to the [Usage](#windows-10-build-1803-or-later) section.
- Optionally, to debug in a QEMU instance:
	- Launch a debugging session in Visual Studio
	- In UEFI shell, run `fs1:\kdmap-patcher.efi`

## License

- For kdmap-patcher, please refer to the [LICENSE](LICENSE) file.

- For the edk2 submodule, please refer to its corresponding license.
