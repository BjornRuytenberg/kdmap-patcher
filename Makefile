.EXPORT_ALL_VARIABLES:
	EDK_TOOLS_PATH = $(PWD)/edk2/BaseTools

default: 
	@echo "Available targets:"
	@echo " linux-edk2\t\t\tBuild EDK2 tree."
	@echo " linux-efi-payload\t\tBuild kdmap-patcher UEFI binary."
	@echo " linux-run\t\t\tDebug kdmap-patcher in QEMU."
	@echo " linux-efi-menu-install\t\tCreate and enable kdmap-patcher as default UEFI boot target."
	@echo " linux-efi-menu-uninstall\tRemove kdmap-patcher from the UEFI boot list."
	@echo " linux-install\t\t\tOnly copy kdmap-patcher to EFI System Partition."
	@echo " linux-uninstall\t\tOnly remove kdmap-patcher from EFI System Partition."
	@echo " secureboot-status\t\tShow the current SecureBoot state."
	@echo " kdmap-status\t\t\tShow the current kDMA Protection state."
	@echo " clean\t\t\t\tClean all build output."
	@echo ""

linux-edk2: 
	cd edk2 && git submodule update --init
	cd edk2 && bash -c '. edksetup.sh BaseTools'
	echo "ACTIVE_PLATFORM       = MdeModulePkg/MdeModulePkg.dsc" > edk2/Conf/target.txt
	echo "TARGET                = RELEASE" >> edk2/Conf/target.txt
	echo "TARGET_ARCH           = X64" >> edk2/Conf/target.txt
	echo "TOOL_CHAIN_CONF       = Conf/tools_def.txt" >> edk2/Conf/target.txt
	echo "TOOL_CHAIN_TAG        = GCC5" >> edk2/Conf/target.txt
	echo "BUILD_RULE_CONF = Conf/build_rule.txt"  >> edk2/Conf/target.txt
	cd edk2 && make -C BaseTools

linux-efi-payload: linux-edk2
	cd edk2 && patch -p1 --forward < ../patches/MdeModulePkg.dsc.patch || true
	patch -p1 --forward < patches/UBUNTU_BOOTLOADER.patch || true
	mkdir -p edk2/MdeModulePkg/Application/kdmap-patcher/
	cp src/*.c src/*.h src/*.inf edk2/MdeModulePkg/Application/kdmap-patcher/
	cd edk2 && bash -c '. edksetup.sh BaseTools && build'
	mkdir -p build
	cp edk2/Build/MdeModule/RELEASE_GCC5/X64/MdeModulePkg/Application/kdmap-patcher/kdmap-patcher/OUTPUT/kdmap-patcher.efi ./build
	echo "Build completed:" && sha256sum build/kdmap-patcher.efi

linux-run: linux-efi-payload
	cp /usr/share/ovmf/OVMF.fd build/OVMF.fd
	if [ ! -f build/startup.nsh ]; then echo "fs0:\kdmap-patcher.efi" >> build/startup.nsh; fi
	iasl samples/dmar-no-kdmap.dsl
	qemu-system-x86_64 -name "kdmap-patcher Debugger" -drive file=build/OVMF.fd,if=pflash,format=raw,unit=0,readonly=on -net none -drive file=build/OVMF.fd,if=pflash,format=raw,unit=1 -drive file=fat:rw:build\,media=disk,if=virtio,format=raw -m 512 -machine q35,smm=on -nodefaults -vga std -global driver=cfi.pflash01,property=secure,value=on -global ICH9-LPC.disable_s3=1 -acpitable file="samples/dmar-no-kdmap.aml"

linux-install: linux-efi-payload
	echo "\n\nThe following bootloader path has been configured in 'kdmap-patcher.c':\n" && grep -B 3 "CHAR16 BOOT_OS\[\] \= " src/kdmap-patcher.c && echo "" && read -p "Is this bootloader path correct? [y/n]" yn; if [ "$$yn" = "y" ]; then echo "Copying kdmap-patcher.efi to /boot/efi/EFI/Boot..." && sudo cp build/kdmap-patcher.efi /boot/efi/EFI/Boot/ && echo "\nDone! In UEFI/BIOS, point your default boot entry to '/EFI/Boot/kdmap-patcher.efi'. After enabling Kernel DMA Protection, kdmap-patcher will chainload your bootloader."; else echo "Installation aborted."; fi

linux-uninstall: 
	@read -p "Are you sure you wish to uninstall kdmap-patcher? [y/n]" yn; if [ "$$yn" = "y" ]; then echo "Deleting kdmap-patcher.efi from /boot/efi/EFI/Boot..." && sudo rm /boot/efi/EFI/Boot/kdmap-patcher.efi && echo "\nDone! In UEFI/BIOS, please point the default boot entry back to your bootloader."; else echo "Aborted."; fi

SECUREBOOT := $(shell mokutil --sb-state)
secureboot-status: 
	test -n "$(SECUREBOOT)"
	echo $(SECUREBOOT)

kdmap-status: 
	cat /sys/bus/thunderbolt/devices/domain0/iommu_dma_protection

KDMAP_INSTALLED := $(shell efibootmgr |grep kDMA |cut -f1 -d\ |cut -f2 -dt|cut -f1 -d\* )
DISK := $(shell mount|grep /boot/efi|cut -f1 -d\ |cut -f1 -dp)
PART := $(shell mount|grep /boot/efi|cut -f1 -d\ |cut -f2 -dp)
linux-efi-menu-install: linux-efi-payload
	echo "\n\nThe following bootloader path has been configured in 'kdmap-patcher.c':\n" && grep -B 3 "CHAR16 BOOT_OS\[\] \= " src/kdmap-patcher.c && echo "" && read -p "Is this bootloader path correct? [y/n]" yn; if [ "$$yn" = "y" ]; then echo "Copying kdmap-patcher.efi to /boot/efi/EFI/Boot..." && sudo cp build/kdmap-patcher.efi /boot/efi/EFI/Boot/ && test -z "$(KDMAP_INSTALLED)" && test -n $(DISK) && test -n $(PART) && sudo efibootmgr -c -d $(DISK) -p $(PART) -L 'kDMA patcher' -l \\EFI\\BOOT\\kdmap-patcher.efi && echo "\nDone! Please reboot your system."; else echo "Installation aborted."; fi

linux-efi-menu-uninstall: 
	@read -p "Are you sure you wish to uninstall kdmap-patcher? [y/n]" yn; if [ "$$yn" = "y" ]; then echo "Deleting kdmap-patcher.efi from /boot/efi/EFI/Boot..." && test -n "$(KDMAP_INSTALLED)" && sudo rm /boot/efi/EFI/Boot/kdmap-patcher.efi && sudo efibootmgr -B -b $(KDMAP_INSTALLED) echo "\nDone! Please reboot your system."; else echo "Aborted."; fi

clean: 
	-rm -rf edk2/MdeModulePkg/Application/kdmap-patcher/
	-rm -rf edk2/Build/MdeModule/RELEASE_GCC5/X64/MdeModulePkg/Application/kdmap-patcher/
	-cd edk2 && git restore MdeModulePkg/MdeModulePkg.dsc
	-rm edk2/MdeModulePkg/MdeModulePkg.dsc.rej
	-rm -rf build
	-rm samples/dmar-no-kdmap.aml
	-rm src/kdmap-patcher.c.rej
