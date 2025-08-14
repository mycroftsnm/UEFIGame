

Silly toy project made to learn about [EDK II](https://github.com/tianocore/edk2)

Consists of multiple modules, each implementing a mini-game that can run in a UEFI environment.

* **User Evaluation For Ineptness**: Presents a simple math question, the sum of two random numbers from 0 to 99, if your answer is incorrect the system mocks you and shuts down.
      <img width="697" height="385" alt="image" src="https://github.com/user-attachments/assets/46e6d61d-feb5-4d54-97dd-460518189bf1" />


* **Insult Sword Fighting**: Monkey island inspired. Choose the correct comeback to continue booting.
      <img width="697" height="385" alt="image" src="https://github.com/user-attachments/assets/89dad2f7-9d19-40ff-b932-5f9ae4cbdfbc" />


## Demo videos
<details>
<summary>User Evaluation For Ineptness Running on QEMU</summary>
    
[Demo on QEMU](https://github.com/user-attachments/assets/f7605ca6-0123-4931-ac4d-57d805c8defd)
</details>

<details>
<summary>User Evaluation For Ineptness Running on Thinkpad X270</summary>
    
[Demo on Thinkpad X270](https://github.com/user-attachments/assets/7ca6e67a-fca9-41bb-a67a-3f5280960edc)
</details>

## Aclaration

In this README, **Insult Sword Fighting** is used as the example module throughout. You can substitute any mention of it and its files with any other module.

## Running
<details>
<summary>Real Hardware </summary>
    
#### 1. Get the EFI Application

   - Download `InsultSwordFighting.efi` from [releases](https://github.com/mycroftsnm/UEFIGame/releases/)
   - Or [build from source](#building)
     
#### 2. Copy EFI Application and neccesary files to ESP
> `ESP` is typically mounted at `/boot` or `/efi` [reference](https://wiki.archlinux.org/title/EFI_system_partition#Typical_mount_points)

```bash
# Create directory structure
sudo mkdir /boot/EFI/UEFIGame

# Copy the efi application
sudo cp InsultSwordFighting.efi /boot/EFI/UEFIGame/

# Copy extra file 
sudo cp insults.txt /boot/EFI/UEFIGame/
```

> Insult Sword Fighting needs `insults.txt` to be present in `/esp/EFI/UEFIGame`, example `insults.txt` provided

> User Evaluation For Ineptess needs `phrases.txt` to be present in `/esp/EFI/UEFIGame`, example `phrases.txt` provided

#### 3. Create EFI boot entry
Easiest way is to use `efibootmgr`

```bash
sudo efibootmgr --create --disk /dev/nvme0n1 --part 1 \ 
    --loader '\EFI\UEFIGame\InsultSwordFighting.efi' \
    --label "UEFIGame"
```
> Adjust `--disk` and `--part` to match your system

#### 4. Reboot and run
Restart your system and enter the `UEFI Setup` (formerly known as BIOS), look for the boot options section and select the EFI entry you just created
</details>

<details>
<summary>QEMU + OVMF</summary>

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

#### 3. Copy EFI Application and neccesary files
Get `InsultSwordFighting.efi` either from the [release section](https://github.com/mycroftsnm/UEFIGame/releases/) or [build it yourself](#building) and copy it on the virtual disk
```bash
# Copy the application
cp InsultSwordFighting.efi uefi_disk/EFI/Boot/BOOTX64.efi

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
</details>

### Extra files 

Extra files should be placed in the same directory as the `.efi` file.

<details>
<summary>User Evaluation For Ineptness</summary>

#### phrases.txt
This file is optional. If it is not present, the game will always use the same fallback phrase.

Example provided:  [`phrases.txt`](./UserEvaluationForIneptness/phrases.txt)

##### How it works
The game will choose a random phrase (Reservoir Sampling) from this file as response if the user fails the question.
If the file is not available or something crashed the game falls back to a default phrase.

##### Format
Nothing fancy, check the example `phrases.txt` provided
- file must be UTF-16
- phrases can be multiline
- phrases are separated by 1 empty line (\n\n)

</details>

<details>
<summary>Insult Sword Fighting</summary>

#### insults.txt
This file is mandatory and must be UTF-16.
Example provided:  [`insults.txt`](./InsultSwordFighting/insults.txt)


##### How it works
The game will choose a random insult and its responses.

##### Format
    Insult
    Correct comeback
    incorrect comeback
    another incorrect comeback
    ...
    as many incorrects comebacks as you want

    Another insult
    Correct comeback
    ...
</details>

## Building
<details>
<summary>Instructions</summary>
    
#### 1. Environment Setup
Follow the instructions to get a working [EDK II build environment](https://github.com/tianocore/tianocore.github.io/wiki/Getting-Started-with-EDK-II)

> All following commands should be run inside the `edk2` directory

#### 2. Clone 
Clone this repo as `UefiGamePkg`
```bash
git clone https://github.com/mycroftsnm/UEFIGame.git UefiGamePkg       
```
#### 3. Build
First of all, run `edksetup.sh` (or `edksetup.bat` if using windows)
```bash
source edksetup.sh
```
Build command example
```bash
build -p UefiGamePkg/UefiGamePkg.dsc -a X64 -t GCC5 -b DEBUG -m UefiGamePkg/InsultSwordFighting.inf  
```
> The `-m` part is useful to avoid building all `MdeModulePkg` modules.


This will generate a (hopefully) working efi binary at `Build/UefiGame/DEBUG_GCC5/X64/InsultSwordFighting.efi`

</details>

