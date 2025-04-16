#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/RngLib.h>

EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  UINT16          RandomNumber1;
  UINT16          RandomNumber2;
  EFI_INPUT_KEY   Key = {0};
  UINT32          Sum = 0;
  CHAR16          InputBuffer[10];
  UINTN           InputIndex;
  EFI_STATUS Status;  

  SystemTable->ConOut->ClearScreen(SystemTable->ConOut);
  SystemTable->ConOut->EnableCursor(SystemTable->ConOut, TRUE);

  if (!GetRandomNumber16(&RandomNumber1)) {
    Print(L"Error trying to get random number\n");
    return EFI_SUCCESS;
  }
  if (!GetRandomNumber16(&RandomNumber2)) {
    Print(L"Error trying to get random number\n");
    return EFI_SUCCESS;
  }
  RandomNumber1 = RandomNumber1 % 100;
  RandomNumber2 = RandomNumber2 % 100;
  Sum = RandomNumber1 + RandomNumber2;

  Print(L"\nWhat is %u + %u?\n", RandomNumber1, RandomNumber2);

  InputIndex = 0;
  while (TRUE) {
    Status = SystemTable->ConIn->ReadKeyStroke(SystemTable->ConIn, &Key);
    if (!EFI_ERROR(Status)) {
      if (Key.UnicodeChar == CHAR_CARRIAGE_RETURN) {
        SystemTable->ConOut->EnableCursor(SystemTable->ConOut, FALSE);
        Print(L"\n");
        InputBuffer[InputIndex] = L'\0';
        UINT32 UserAnswer = StrDecimalToUintn(InputBuffer);
        if (UserAnswer == Sum) {
          Print(L"Ok\n");
          break;
        } else {
          SystemTable->ConOut->SetAttribute(SystemTable->ConOut, EFI_TEXT_ATTR(EFI_LIGHTRED, EFI_BLACK));
          Print(L"\nPlease do not use computers.\n");
          gBS->Stall(3000000);
          SystemTable->RuntimeServices->ResetSystem(EfiResetShutdown, EFI_SUCCESS, 0, NULL);
        }
      } else if (Key.UnicodeChar == CHAR_BACKSPACE) {
        if (InputIndex > 0) {
          InputIndex--;
          Print(L"\b \b");
        }
      } else if (Key.UnicodeChar >= L'0' && Key.UnicodeChar <= L'9' && InputIndex < 9) {
        InputBuffer[InputIndex++] = Key.UnicodeChar;
        Print(L"%c", Key.UnicodeChar);
      }
    }
  }

  return EFI_ABORTED;
}
