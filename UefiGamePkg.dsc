[Defines]
  !include MdeModulePkg/MdeModulePkg.dsc
  PLATFORM_NAME        = UefiGame
  PLATFORM_GUID        = 12345678-1234-5678-1234-567812345678
  SUPPORTED_ARCHITECTURES = X64
  OUTPUT_DIRECTORY     = Build/UefiGame

[LibraryClasses]
  RngLib|MdePkg/Library/BaseRngLib/BaseRngLib.inf

[Components.X64]
  UefiGamePkg/UserEvaluationForIneptness.inf

