[Defines]
  !include MdeModulePkg/MdeModulePkg.dsc
  SUPPORTED_ARCHITECTURES = X64
  OUTPUT_DIRECTORY     = Build/UefiGame

[LibraryClasses]
  RngLib|MdePkg/Library/BaseRngLib/BaseRngLib.inf

[Components.X64]
  UefiGamePkg/UserEvaluationForIneptness/UserEvaluationForIneptness.inf
  UefiGamePkg/InsultSwordFighting/InsultSwordFighting.inf
  UefiGamePkg/FallToBoot/FallToBoot.inf
  UefiGamePkg/AgeVerification/AgeVerification.inf
  UefiGamePkg/UEFISays/UEFISays.inf

