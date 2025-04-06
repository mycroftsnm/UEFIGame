


# UserEvaluationForIneptness

Silly toy project made to learn about [EDK II](https://github.com/tianocore/edk2)

It throws you a simple math question — the sum of two random numbers.

Get it wrong, and you'll be mocked and the system shuts down.

Get it right, and the boot continues like nothing ever happened.

## Demo videos
<details>
    
<summary>Running on QEMU</summary>

[Demo on QEMU](https://github.com/user-attachments/assets/f7605ca6-0123-4931-ac4d-57d805c8defd)

</details>

<details>
    
<summary>Running on Thinkpad X270</summary>

[Demo on Thinkpad X270](https://github.com/user-attachments/assets/7ca6e67a-fca9-41bb-a67a-3f5280960edc)

</details>


## Running

### Real Hardware
#### 1. Copy EFI Application to ESP
Get `UserEvaluationForIneptness.efi` either from the [release section](https://github.com/mycroftsnm/UEFIGame/releases/) or [build it yourself](#building), then copy it to your `ESP`, which is usually mounted at `/boot` 
```bash
sudo cp UserEvaluationForIneptness.efi /boot/EFI/
```
#### 2. Create EFI boot entry
Easiest way is to use `efibootmgr`

```bash
sudo efibootmgr --create --disk /dev/nvme0n1 --part 1 \ 
    --loader '\EFI\UserEvaluationForIneptness.efi' \
    --label "UEFIGame"
```
> Adjust `--disk` and `--part` to match your system

#### 3. Reboot and run
Restart your system and enter the `UEFI Setup` (formerly known as BIOS), look for the boot options section and select the EFI entry you just created


### QEMU + OVFM

#### 1. Install Dependencies
```bash
# Arch Linux
sudo pacman -S qemu edk2-ovmf
# Debian/Ubuntu
sudo apt install qemu-system-x86 ovmf
# Fedora
sudo dnf install qemu edk2-ovmf
```

#### 2. Prepare Virtual Disk (FAT32 ESP)
```bash
# Create 64MB image (adjust 'count' for size)
dd if=/dev/zero of=uefi_disk.img bs=1M count=64 status=progress
# Format as FAT32 (UEFI requirement)
mkfs.vfat -F 32 uefi_disk.img
```

#### 3. Copy EFI Application
Get `UserEvaluationForIneptness.efi` either from the [release section](https://github.com/mycroftsnm/UEFIGame/releases/) or [build it yourself](#building) and copy it on the virtual disk
```bash
sudo mkdir -p /mnt/uefi_disk
sudo mount uefi_disk.img /mnt/uefi_disk
sudo mkdir -p /mnt/uefi_disk/EFI/BOOT
sudo cp Build/UefiGamePkg/DEBUG_GCC5/X64/UserEvaluationForIneptness.efi /mnt/uefi_disk/EFI/BOOT/BOOTX64.EFI
sudo umount /mnt/uefi_disk
```

#### 4. Launch QEMU
```bash
qemu-system-x86_64 \
    -drive if=pflash,format=raw,readonly=on,file=/usr/share/edk2/x64/OVMF_CODE.4m.fd \
    -drive file=uefi_disk.img,format=raw,if=virtio \
    -cpu qemu64,+rdrand\
```

**Notes**:  
- Replace OVMF path if needed (common alternatives: `/usr/share/OVMF/OVMF_CODE.fd`)  
- The `BOOTX64.EFI` filename is required for automatic UEFI boot  
   

## Building 
#### 1. Environment Setup
Follow the instructions to get a working [EDK II build environment](https://github.com/tianocore/tianocore.github.io/wiki/Getting-Started-with-EDK-II)

> All following commands should be run inside the `edk2` directory

#### 2. Clone 
Clone this repo as `UefiGamePkg`
```bash
git clone https://github.com/mycroftsnm/UEFIGame.git UefiGamePkg       
```
#### 3. Build
Build command example
```bash
build -p UefiGamePkg/UefiGamePkg.dsc -a X64 -t GCC5 -b DEBUG -m UefiGamePkg/UserEvaluationForIneptness.inf  
```
> The `-m` part is useful to avoid building all `MdeModulePkg` modules.


This will generate a (hopefully) working efi binary at `Build/UefiGamePkg/DEBUG_GCC5/X64/UserEvaluationForIneptness.efi`


