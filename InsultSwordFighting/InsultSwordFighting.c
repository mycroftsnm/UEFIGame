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

/**
 * Counts the number of lines in a phrase.
 * Lines are separated by \r\n sequences.
 *
 * @param Phrase   Pointer to the phrase string.
 * @return UINTN   Number of lines in the phrase.
 */
UINTN CountLinesInPhrase(IN CHAR16* Phrase)
{
    UINTN   LineCount = 0;
    UINTN   Index = 0;

    while (Phrase[Index+1] != L'\0') {
        if (Phrase[Index] == L'\r' && Phrase[Index + 1] == '\n') {
            LineCount++;
        }
        Index++;
    }

    return LineCount;
}

void ParseInsult(CHAR16 *raw_text, CHAR16 **insult, CHAR16 **options, UINTN *option_count) {
    UINTN line_start = 0;
    UINTN line_end = 0;
    UINTN line_index = 0;
    while (raw_text[line_end] != L'\0') {
      if (raw_text[line_end] == L'\n') {
          if (line_index == 0) {
              // First line is the insult
              UINTN insult_length = line_end - line_start;
              StrnCpyS(*insult, insult_length + 1, &raw_text[line_start], insult_length);
          } else {
              // Following lines are responses
              UINTN option_length = line_end - line_start;
              StrnCpyS(options[line_index - 1], option_length + 1, &raw_text[line_start], option_length);
          }
          line_index++;
          line_end += 1;
          line_start = line_end;
      } else {
          line_end++;
      }
    }
    *option_count = line_index - 1;
}


void Shuffle(UINTN *idxs, UINTN n) {
  for (UINTN i = n - 1; i > 0; i--) {
    UINT16 rand;
    GetRandomNumber16(&rand);
    UINTN j = rand % (i + 1);
    UINTN tmp = idxs[i];
    idxs[i] = idxs[j];
    idxs[j] = tmp;
    }
}

EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
)
{
    EFI_INPUT_KEY Key = {0};
    UINTN Selected = 0;
    UINTN i;

    CHAR16 *options[4];
    for (UINTN k = 0; k < 4; k++) {
        options[k] = AllocateZeroPool(128 * sizeof(CHAR16));
    }

    CHAR16 *insult = AllocateZeroPool(256 * sizeof(CHAR16));

    CHAR16 *raw_text = ReadRandomPhraseFromFile(L"\\EFI\\UEFIGame\\insults.txt");

    ParseInsult(
        raw_text,
        &insult,
        options,
        &i
    );

    UINTN indexes[4] = {0, 1, 2, 3};

    Shuffle(indexes, 4);

    SystemTable->ConOut->ClearScreen(SystemTable->ConOut);

    Print(L"%s\n", insult);

    SystemTable->ConOut->SetAttribute(SystemTable->ConOut, EFI_TEXT_ATTR(EFI_MAGENTA, EFI_BLACK));

    while (1) {
        SystemTable->ConOut->SetCursorPosition(SystemTable->ConOut, 0, 4);
        for (i = 0; i < 4; i++) {
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
        else if (Key.ScanCode == SCAN_DOWN && Selected < 4 - 1) {
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
