#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/RngLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/LoadedImage.h>  // Necesario para EFI_LOADED_IMAGE_PROTOCOL
/**
 * Reads a UTF-16 file from filesystem and returns a random phrase.
 * A phrase can consist of a single or multiple lines.
 * Phrases are separated by empty lines.
 * If fails returns a default phrase.
 *
 * @param FileName   Filename (ej: L"\\EFI\\UEFIGame\\phrases.txt").
 * @return CHAR16*   Pointer to the selected phrase.
 */
CHAR16* ReadRandomPhraseFromFile(IN CHAR16* FileName)
{
    EFI_STATUS                      Status;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *FileSystem = NULL;
    EFI_FILE_PROTOCOL               *Root = NULL, *File = NULL;
    EFI_GUID                        gEfiLoadedImageProtocolGuid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
    EFI_LOADED_IMAGE_PROTOCOL       *LoadedImage = NULL;
    CHAR16                          *SelectedPhrase = NULL;
    CHAR16                          PhraseBuffer[420];
    CHAR16                          CurrentChar;
    UINTN                           ReadSize;
    UINTN                           PhraseLength = 0;
    UINTN                           PhraseCount = 0;
    BOOLEAN                         LastWasNewline = TRUE;

    Status = gBS->HandleProtocol(
        gImageHandle,
        &gEfiLoadedImageProtocolGuid,
        (VOID**)&LoadedImage
    );
    if (EFI_ERROR(Status)) goto default_phrase;

    Status = gBS->HandleProtocol(
      LoadedImage->DeviceHandle,
      &gEfiSimpleFileSystemProtocolGuid,
      (VOID**)&FileSystem
    );
    if (EFI_ERROR(Status)) goto default_phrase;

    Status = FileSystem->OpenVolume(FileSystem, &Root);
    if (EFI_ERROR(Status)) goto default_phrase;

    Status = Root->Open(Root, &File, FileName, EFI_FILE_MODE_READ, 0);
    if (EFI_ERROR(Status)) goto default_phrase;

    while (TRUE) {
      ReadSize = sizeof(CurrentChar);
      Status = File->Read(File, &ReadSize, &CurrentChar);
      if (EFI_ERROR(Status) || ReadSize == 0) break;

      if (CurrentChar == L'\r') continue;

      if (CurrentChar == L'\n') {
        if (LastWasNewline) {
          PhraseBuffer[PhraseLength] = L'\0';
          PhraseCount++;

          UINT16 Rand;
          Status = GetRandomNumber16(&Rand);
          if (EFI_ERROR(Status)) goto default_phrase;

          if ((Rand % PhraseCount) == 0) {
              if (SelectedPhrase) FreePool(SelectedPhrase);
              SelectedPhrase = AllocateCopyPool((PhraseLength + 1) * sizeof(CHAR16), PhraseBuffer);
          }
          PhraseLength = 0;
        }
        else {
          if (PhraseLength < ARRAY_SIZE(PhraseBuffer) - 1) {
              PhraseBuffer[PhraseLength++] = L'\r';
              PhraseBuffer[PhraseLength++] = L'\n';
          }
        }
        LastWasNewline = TRUE;
      }
      else {
        LastWasNewline = FALSE;
        if (PhraseLength < ARRAY_SIZE(PhraseBuffer) - 1) {
            PhraseBuffer[PhraseLength++] = CurrentChar;
        }
      }
    }
    if (PhraseLength > 0) {
      PhraseBuffer[PhraseLength] = L'\0';
      PhraseCount++;
      UINT16 Rand;
      Status = GetRandomNumber16(&Rand);
      if (!EFI_ERROR(Status) && (Rand % PhraseCount) == 0) {
        if (SelectedPhrase) FreePool(SelectedPhrase);
        SelectedPhrase = AllocateCopyPool((PhraseLength + 1) * sizeof(CHAR16), PhraseBuffer);
      }
    }

  default_phrase:
    if (File) File->Close(File);
    if (Root) Root->Close(Root);
    return SelectedPhrase ? SelectedPhrase : L"Please do not use computers.\n";
}

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
  CHAR16 *RandomPhrase = ReadRandomPhraseFromFile(L"\\EFI\\UEFIGame\\phrases.txt");

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
          Print(L"%s\n", RandomPhrase);
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
