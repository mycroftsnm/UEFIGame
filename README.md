# UEFIGame
**UserEvaluationForIneptness**

## Building 
1. First set up an [EDK II build environment](https://github.com/tianocore/tianocore.github.io/wiki/Getting-Started-with-EDK-II)
2. Clone this repo as UefiGamePkg
```
git clone https://github.com/mycroftsnm/UEFIGame.git UefiGamePkg       
```
3. Build command example:
```
build -p UefiGamePkg/UefiGamePkg.dsc -a X64 -t GCC5 -b DEBUG -m UefiGamePkg/UserEvaluationForIneptness.inf  
```
> The `-m` part is useful to avoid building all `MdeModulePkg` modules.


This will generate a (hopefully) working efi binary at `Build/UefiGamePkg/DEBUG_GCC5/X64/UserEvaluationForIneptness.efi`
