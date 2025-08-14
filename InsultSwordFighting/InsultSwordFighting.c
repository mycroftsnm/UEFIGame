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


void ParseInsult(IN CHAR16 *raw_text, OUT CHAR16 **insult, OUT CHAR16 ***options, OUT UINTN *option_count) {
    UINTN line_start = 0;
    UINTN line_end = 0;
    while (raw_text[line_end] != L'\0') {
      if (raw_text[line_end] == L'\n') {
          UINTN len = line_end - line_start;
          if (*insult == NULL) {
              // First line is the insult
              *insult = AllocateZeroPool((len + 1) * sizeof(CHAR16));
              StrnCpyS(*insult, len + 1, &raw_text[line_start], len);
          } else {
            // Following lines are responses
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
    CHAR16 *insult = NULL;

    CHAR16 *raw_text = ReadRandomPhraseFromFile(L"\\EFI\\UEFIGame\\insults.txt");

    UINTN Columns, Rows;
    SystemTable->ConOut->QueryMode(SystemTable->ConOut, SystemTable->ConOut->Mode->Mode, &Columns, &Rows);


    ParseInsult(
      raw_text,
      &insult,
      &options,
      &option_count
    );

    UINTN insult_len = StrLen(insult);
    UINTN start_col = 0;
    if (insult_len < Columns/2) {
        start_col = Columns/2 - insult_len;
    }

    SystemTable->ConOut->ClearScreen(SystemTable->ConOut);
    SystemTable->ConOut->SetCursorPosition(SystemTable->ConOut, start_col, 0);

    Print(L"- %s\n", insult);

    UINTN *indexes = GetRandomIndexes(option_count);
    UINTN Selected = 0;

    SystemTable->ConOut->SetAttribute(SystemTable->ConOut, EFI_TEXT_ATTR(EFI_MAGENTA, EFI_BLACK));
    while (1) {
        SystemTable->ConOut->SetCursorPosition(SystemTable->ConOut, 0, 4);
        for (UINTN i = 0; i < option_count; i++) {
            if (i == Selected) {
                SystemTable->ConOut->SetAttribute(SystemTable->ConOut, EFI_TEXT_ATTR(EFI_LIGHTMAGENTA, EFI_BLACK));
                Print(L"> %s\n", options[indexes[i]]);
                SystemTable->ConOut->SetAttribute(SystemTable->ConOut, EFI_TEXT_ATTR(EFI_MAGENTA, EFI_BLACK));
            } else {
                Print(L"  %s\n", options[indexes[i]]);
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
                break;
            } else {
                SystemTable->RuntimeServices->ResetSystem(EfiResetShutdown, EFI_SUCCESS, 0, NULL);
            }
            break;
        }
    }
    return EFI_ABORTED;
}
