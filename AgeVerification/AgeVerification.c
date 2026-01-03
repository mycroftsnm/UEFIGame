#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/RngLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/LoadedImage.h>

/**
 * Reads a UTF-16 file from filesystem and returns a random phrase.
 * A phrase can consist of a single or multiple lines.
 * Phrases are separated by empty lines.
 * If fails returns a default phrase.
 */
CHAR16* ReadRandomPhraseFromFile(IN CHAR16* FileName)
{
    EFI_STATUS                      Status;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *FileSystem = NULL;
    EFI_FILE_PROTOCOL               *Root = NULL, *File = NULL;
    EFI_GUID                        gEfiLoadedImageProtocolGuid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
    EFI_LOADED_IMAGE_PROTOCOL       *LoadedImage = NULL;
    CHAR16                          *SelectedPhrase = NULL;
    CHAR16                          PhraseBuffer[1024];
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
    return SelectedPhrase ? SelectedPhrase : L"What is the answer to life?\nForty-two\nSeven\nPi\nNone\n";
}


void ParseQuestion(IN CHAR16 *raw_text, OUT CHAR16 **question, OUT CHAR16 ***options, OUT UINTN *option_count) {
    UINTN line_start = 0;
    UINTN line_end = 0;
    while (raw_text[line_end] != L'\0') {
      if (raw_text[line_end] == L'\n') {
          UINTN len = line_end - line_start;
          if (len > 0 && raw_text[line_end - 1] == L'\r') len--;

          if (*question == NULL) {
              *question = AllocateZeroPool((len + 1) * sizeof(CHAR16));
              StrnCpyS(*question, len + 1, &raw_text[line_start], len);
          } else {
            UINTN oldSize = (*option_count) * sizeof(CHAR16*);
            UINTN newSize = ( (*option_count) + 1 ) * sizeof(CHAR16*);

            if (*options == NULL) {
                *options = (CHAR16**) AllocateZeroPool(newSize);
            } else {
                *options = (CHAR16**) ReallocatePool(oldSize, newSize, *options);
            }
            (*options)[*option_count] = AllocateZeroPool((len + 1) * sizeof(CHAR16));
            StrnCpyS((*options)[*option_count], len + 1, &raw_text[line_start], len);
            (*option_count)++;
          }
          line_start = line_end + 1;
        }
      line_end++;
    }
}


UINTN* GetRandomIndexes(UINTN n) {
    UINTN *indexes = AllocateZeroPool(n * sizeof(UINTN));
    if (!indexes) return NULL;

    for (UINTN i = 0; i < n; i++) {
        indexes[i] = i;
    }
    for (UINTN i = n - 1; i > 0; i--) {
        UINT16 rand;
        GetRandomNumber16(&rand);
        UINTN j = rand % (i + 1);
        UINTN tmp = indexes[i];
        indexes[i] = indexes[j];
        indexes[j] = tmp;
    }
    return indexes;
}

EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
)
{
    EFI_INPUT_KEY Key = {0};

    UINTN option_count = 0;
    CHAR16 **options = NULL;
    CHAR16 *question = NULL;

    CHAR16 *raw_text = ReadRandomPhraseFromFile(L"\\EFI\\UEFIGame\\questions.txt");
    CHAR16 *fail_message = ReadRandomPhraseFromFile(L"\\EFI\\UEFIGame\\failmessages.txt");

    UINTN Columns, Rows;
    SystemTable->ConOut->QueryMode(SystemTable->ConOut, SystemTable->ConOut->Mode->Mode, &Columns, &Rows);

    ParseQuestion(
      raw_text,
      &question,
      &options,
      &option_count
    );

    SystemTable->ConOut->ClearScreen(SystemTable->ConOut);

    // Title
    SystemTable->ConOut->SetAttribute(SystemTable->ConOut, EFI_TEXT_ATTR(EFI_YELLOW, EFI_BLACK));
    Print(L"\r\n  === AGE VERIFICATION ===\r\n\r\n");

    // Question
    SystemTable->ConOut->SetAttribute(SystemTable->ConOut, EFI_TEXT_ATTR(EFI_WHITE, EFI_BLACK));
    Print(L"  %s\r\n\r\n", question);

    UINTN *indexes = GetRandomIndexes(option_count);
    UINTN Selected = 0;

    SystemTable->ConOut->SetAttribute(SystemTable->ConOut, EFI_TEXT_ATTR(EFI_CYAN, EFI_BLACK));
    while (1) {
        SystemTable->ConOut->SetCursorPosition(SystemTable->ConOut, 0, 6);
        for (UINTN i = 0; i < option_count; i++) {
            if (i == Selected) {
                SystemTable->ConOut->SetAttribute(SystemTable->ConOut, EFI_TEXT_ATTR(EFI_LIGHTCYAN, EFI_BLACK));
                Print(L"  > %c. %s\r\n", L'a' + (CHAR16)i, options[indexes[i]]);
                SystemTable->ConOut->SetAttribute(SystemTable->ConOut, EFI_TEXT_ATTR(EFI_CYAN, EFI_BLACK));
            } else {
                Print(L"    %c. %s\r\n", L'a' + (CHAR16)i, options[indexes[i]]);
            }
        }

        while (SystemTable->ConIn->ReadKeyStroke(SystemTable->ConIn, &Key) == EFI_NOT_READY);

        if (Key.ScanCode == SCAN_UP && Selected > 0) {
            Selected--;
        }
        else if (Key.ScanCode == SCAN_DOWN && Selected < option_count - 1) {
            Selected++;
        }
        else if (Key.UnicodeChar == CHAR_CARRIAGE_RETURN) {
            if (indexes[Selected] == 0) {
                // Correct! Continue boot
                SystemTable->ConOut->SetAttribute(SystemTable->ConOut, EFI_TEXT_ATTR(EFI_GREEN, EFI_BLACK));
                Print(L"\r\n  Welcome, adult user. Booting...\r\n");
                break;
            } else {
                // Wrong - you're a kid, shutdown
                SystemTable->ConOut->SetAttribute(SystemTable->ConOut, EFI_TEXT_ATTR(EFI_RED, EFI_BLACK));
                Print(L"\r\n  %s\r\n", fail_message);
                gBS->Stall(3000000);
                SystemTable->RuntimeServices->ResetSystem(EfiResetShutdown, EFI_SUCCESS, 0, NULL);
            }
            break;
        }
    }
    return EFI_ABORTED;
}
