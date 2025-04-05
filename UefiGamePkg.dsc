[Defines]
  !include MdeModulePkg/MdeModulePkg.dsc
  PLATFORM_NAME        = UefiGame
  PLATFORM_GUID        = F1805956-8487-4960-8168-09D66F99BC69
  SUPPORTED_ARCHITECTURES = X64
  OUTPUT_DIRECTORY     = Build/UefiGame

[LibraryClasses]
  RngLib|MdePkg/Library/BaseRngLib/BaseRngLib.inf

[Components.X64]
  UefiGamePkg/UserEvaluationForIneptness.inf

