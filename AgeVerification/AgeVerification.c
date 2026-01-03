#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/RngLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/LoadedImage.h>

#define QUESTIONS_TO_WIN 3
#define QUESTIONS_TO_LOSE 3
#define MAX_QUESTIONS 6

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
    return SelectedPhrase ? SelectedPhrase : NULL;
}


void ParseQuestion(IN CHAR16 *raw_text, OUT CHAR16 **question, OUT CHAR16 ***options, OUT UINTN *option_count) {
    *question = NULL;
    *options = NULL;
    *option_count = 0;

    if (raw_text == NULL) return;

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

void FreeQuestion(CHAR16 *question, CHAR16 **options, UINTN option_count) {
    if (question) FreePool(question);
    if (options) {
        for (UINTN i = 0; i < option_count; i++) {
            if (options[i]) FreePool(options[i]);
        }
        FreePool(options);
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

/**
 * Ask one question. Returns TRUE if answered correctly, FALSE otherwise.
 */
BOOLEAN AskQuestion(IN EFI_SYSTEM_TABLE *SystemTable, IN UINTN QuestionNum) {
    EFI_INPUT_KEY Key = {0};
    UINTN option_count = 0;
    CHAR16 **options = NULL;
    CHAR16 *question = NULL;
    BOOLEAN result = FALSE;

    CHAR16 *raw_text = ReadRandomPhraseFromFile(L"\\EFI\\UEFIGame\\questions.txt");
    if (raw_text == NULL) {
        raw_text = L"What is the answer to life?\nForty-two\nSeven\nPi\nNone\n";
    }

    ParseQuestion(raw_text, &question, &options, &option_count);

    if (question == NULL || option_count < 2) {
        return TRUE; // Skip bad questions
    }

    SystemTable->ConOut->ClearScreen(SystemTable->ConOut);

    // Title with question number
    SystemTable->ConOut->SetAttribute(SystemTable->ConOut, EFI_TEXT_ATTR(EFI_YELLOW, EFI_BLACK));
    Print(L"\r\n  === AGE VERIFICATION (%d/%d) ===\r\n\r\n", QuestionNum, MAX_QUESTIONS);

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
            result = (indexes[Selected] == 0);
            break;
        }
    }

    if (indexes) FreePool(indexes);
    FreeQuestion(question, options, option_count);

    return result;
}

EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
)
{
    UINTN correct = 0;
    UINTN wrong = 0;
    UINTN question_num = 0;
    BOOLEAN last_was_correct = FALSE;
    BOOLEAN had_correct_before_wrong = FALSE;

    // Pre-load messages
    CHAR16 *fail_message = ReadRandomPhraseFromFile(L"\\EFI\\UEFIGame\\failmessages.txt");
    CHAR16 *success_message = ReadRandomPhraseFromFile(L"\\EFI\\UEFIGame\\successmessages.txt");

    if (fail_message == NULL) {
        fail_message = L"Nice try, kid. Come back when you're older.";
    }
    if (success_message == NULL) {
        success_message = L"Clearly you lived through the 80s. Welcome, elder millennial.";
    }

    while (question_num < MAX_QUESTIONS) {
        question_num++;

        BOOLEAN answered_correctly = AskQuestion(SystemTable, question_num);

        if (answered_correctly) {
            correct++;
            SystemTable->ConOut->SetAttribute(SystemTable->ConOut, EFI_TEXT_ATTR(EFI_GREEN, EFI_BLACK));
            if (had_correct_before_wrong && !last_was_correct) {
                // They got one right, then wrong, now right again - suspicious!
                UINT16 snark;
                GetRandomNumber16(&snark);
                switch (snark % 3) {
                    case 0:
                        Print(L"\r\n  Correct! (%d/%d) ...did you have to call your mom?\r\n", correct, QUESTIONS_TO_WIN);
                        break;
                    case 1:
                        Print(L"\r\n  Correct! (%d/%d) ...you didn't Google that on THIS computer.\r\n", correct, QUESTIONS_TO_WIN);
                        break;
                    default:
                        Print(L"\r\n  Correct! (%d/%d) ...that took a suspiciously long time.\r\n", correct, QUESTIONS_TO_WIN);
                        break;
                }
            } else {
                Print(L"\r\n  Correct! (%d/%d)\r\n", correct, QUESTIONS_TO_WIN);
            }
            if (!last_was_correct && correct > 0) {
                had_correct_before_wrong = FALSE; // Reset after calling them out
            }
            last_was_correct = TRUE;
        } else {
            wrong++;
            SystemTable->ConOut->SetAttribute(SystemTable->ConOut, EFI_TEXT_ATTR(EFI_RED, EFI_BLACK));
            if (last_was_correct) {
                had_correct_before_wrong = TRUE;
            }
            Print(L"\r\n  Wrong! (%d/%d failures)\r\n", wrong, QUESTIONS_TO_LOSE);
            last_was_correct = FALSE;
        }

        // Check win condition
        if (correct >= QUESTIONS_TO_WIN) {
            SystemTable->ConOut->SetAttribute(SystemTable->ConOut, EFI_TEXT_ATTR(EFI_GREEN, EFI_BLACK));
            Print(L"\r\n  %s\r\n", success_message);
            Print(L"\r\n  Booting...\r\n");
            gBS->Stall(2000000);
            return EFI_ABORTED; // Continue to boot
        }

        // Check lose condition
        if (wrong >= QUESTIONS_TO_LOSE) {
            SystemTable->ConOut->SetAttribute(SystemTable->ConOut, EFI_TEXT_ATTR(EFI_RED, EFI_BLACK));
            Print(L"\r\n  %s\r\n", fail_message);
            gBS->Stall(2000000);
            SystemTable->RuntimeServices->ResetSystem(EfiResetShutdown, EFI_SUCCESS, 0, NULL);
        }

        // Check if it's impossible to win (not enough questions left)
        UINTN remaining = MAX_QUESTIONS - question_num;
        if (correct + remaining < QUESTIONS_TO_WIN) {
            SystemTable->ConOut->SetAttribute(SystemTable->ConOut, EFI_TEXT_ATTR(EFI_RED, EFI_BLACK));
            Print(L"\r\n  Not enough questions left. You can't possibly be an adult.\r\n");
            Print(L"\r\n  %s\r\n", fail_message);
            gBS->Stall(2000000);
            SystemTable->RuntimeServices->ResetSystem(EfiResetShutdown, EFI_SUCCESS, 0, NULL);
        }

        // Next question prompt
        if (question_num < MAX_QUESTIONS) {
            SystemTable->ConOut->SetAttribute(SystemTable->ConOut, EFI_TEXT_ATTR(EFI_YELLOW, EFI_BLACK));
            Print(L"\r\n  Next question...\r\n");
            gBS->Stall(1200000);
        }
    }

    // Should not reach here, but just in case
    SystemTable->RuntimeServices->ResetSystem(EfiResetShutdown, EFI_SUCCESS, 0, NULL);
    return EFI_ABORTED;
}
