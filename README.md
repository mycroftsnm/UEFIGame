


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
#### 1. Get the EFI Application
   - Download `UserEvaluationForIneptness.efi` from [releases](https://github.com/mycroftsnm/UEFIGame/releases/)
   - Or [build from source](#building)
     
#### 2. Copy EFI Application and phrases file to ESP
> `ESP` is typically mounted at `/boot` or `/efi` [reference](https://wiki.archlinux.org/title/EFI_system_partition#Typical_mount_points)

> Example `phrases.txt` are provided in repo root
```bash
# Create directory structure
sudo mkdir /boot/EFI/UEFIGame

# Copy the application
sudo cp UserEvaluationForIneptness.efi /boot/EFI/UEFIGame/

# Optional: Copy phrases file (must be UTF-16)
sudo cp phrases.txt /boot/EFI/UEFIGame/
```

#### 3. Create EFI boot entry
Easiest way is to use `efibootmgr`

```bash
sudo efibootmgr --create --disk /dev/nvme0n1 --part 1 \ 
    --loader '\EFI\UEFIGame\UserEvaluationForIneptness.efi' \
    --label "UEFIGame"
```
> Adjust `--disk` and `--part` to match your system

#### 4. Reboot and run
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

#### 2. Prepare Virtual FAT Filesystem
Qemu can emulate a virtual drive with a FAT filesystem. It works by prepending fat: to a directory name. [reference](https://en.wikibooks.org/wiki/QEMU/Devices/Storage)
```bash
# Create directory structure
mkdir -p uefi_disk/EFI/UEFIGame
```

#### 3. Copy EFI Application and phrases file
Get `UserEvaluationForIneptness.efi` either from the [release section](https://github.com/mycroftsnm/UEFIGame/releases/) or [build it yourself](#building) and copy it on the virtual disk
> Example `phrases.txt` are provided in repo root
```bash
# Copy the application
cp UserEvaluationForIneptness.efi uefi_disk/EFI/Boot/BOOTX64.efi

# Optional: Copy phrases file (must be UTF-16)
cp phrases.txt uefi_disk/EFI/UEFIGame/
```

#### 4. Launch QEMU
```bash
qemu-system-x86_64 \
    -drive if=pflash,format=raw,readonly=on,file=/usr/share/edk2/x64/OVMF_CODE.4m.fd \
    -drive file=fat:rw:./uefi_disk,format=raw,if=virtio \
    -cpu qemu64,+rdrand
```

**Notes**:  
- Replace OVMF path if needed (common alternatives: `/usr/share/OVMF/OVMF_CODE.fd`)  
- The `BOOTX64.EFI` filename is required for automatic UEFI boot  
   

## Phrases file

### How it works
The game will choose a random phrase (Reservoir Sampling) from this file as response if the user fails the question.
If the file is not available or something crashed the game falls back to a default phrase.

### Format
Nothing fancy, check the example `phrases.txt` provided
- file must be UTF-16
- phrases can be multiline
- phrases are separated by 1 empty line (\n\n)

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


This will generate a (hopefully) working efi binary at `Build/UefiGame/DEBUG_GCC5/X64/UefiGamePkg/UserEvaluationForIneptness/OUTPUT/UserEvaluationForIneptness.efi`


