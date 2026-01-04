#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/RngLib.h>



#define BLOCK_CHAR L"\u2588" // █
#define MAX_SEQUENCE_LENGTH 10

typedef struct {
    UINTN X;
    UINTN Y;
    UINTN OnColor;
    UINTN OffColor;
} BUTTON;

BUTTON Buttons[4];
CHAR16 *SolidRow = L"\u2588\u2588\u2588\u2588\u2588\u2588\u2588\u2588\u2588";

void InitButtons() {
    Buttons[0] = (BUTTON){25, 14,  EFI_LIGHTRED,   EFI_RED};    // left
    Buttons[1] = (BUTTON){47, 14,  EFI_LIGHTBLUE,  EFI_BLUE};   // right
    Buttons[2] = (BUTTON){36, 8,   EFI_LIGHTGREEN, EFI_GREEN};  // up
    Buttons[3] = (BUTTON){36, 14,  EFI_YELLOW,     EFI_BROWN};  // down 
}

void DrawButtonBlock(BUTTON *Btn, BOOLEAN IsOn) {
    UINTN Color = IsOn ? Btn->OnColor : Btn->OffColor;
    UINTN StartY = Btn->Y - 2; // 5 rows high
    
    gST->ConOut->SetAttribute(gST->ConOut, Color);
    for (INTN i = 0; i < 5; i++) {
        gST->ConOut->SetCursorPosition(gST->ConOut, Btn->X - 4, StartY + i);
        gST->ConOut->OutputString(gST->ConOut, SolidRow);
    }
}

// Turn on button for DelayMilliseconds ms
void FlashButton(BUTTON *Btn, UINTN DelayMilliseconds) {
    DrawButtonBlock(Btn, TRUE);
    gBS->Stall(DelayMilliseconds * 1000);
    DrawButtonBlock(Btn, FALSE);
}

UINT16 GetRandom(UINT16 Max) {
    UINT16 Rand;
    if (Max == 0) return 0;
    GetRandomNumber16(&Rand);
    return Rand % Max;
}

void FlashSequence(INTN *Steps, INTN Length, INTN TotalLength) {
    INTN Delay = 500;
    // +Speed up last 2
    if (TotalLength - Length <= 2) {
        Delay = 200;
    // Speed up after first half
    } else if (Length > TotalLength / 2) {
        Delay = 300;
    }
    for (INTN i = 0; i < Length; i++) {
        gBS->Stall(Delay * 1000); // delay between flashes
        UINTN BtnIndex = Steps[i];
        FlashButton(&Buttons[BtnIndex], Delay);
    }
}

EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
)
{
    gST->ConOut->ClearScreen(gST->ConOut);
    gST->ConOut->EnableCursor(gST->ConOut, FALSE);

    // Initialize and draw buttonss
    InitButtons();
    for (UINTN i = 0; i < 4; i++) {
        DrawButtonBlock(&Buttons[i], FALSE);
        DrawButtonBlock(&Buttons[i], FALSE);
    }

    EFI_INPUT_KEY Key;
    INTN Sequence[MAX_SEQUENCE_LENGTH];

    INTN TargetLength = 8;//GetRandom(MAX_SEQUENCE_LENGTH - 3) + 4; // 4 steps min
    INTN CurrentLength = 0;
    INTN PlayerInput = -1;
    UINTN Index;

    // Wait for a key press to start
    gST->ConOut->SetCursorPosition(gST->ConOut, 25, 2);
    gST->ConOut->SetAttribute(gST->ConOut, EFI_WHITE);
    gST->ConOut->OutputString(gST->ConOut, L"Press any key to start...");

    gBS->WaitForEvent(1, &gST->ConIn->WaitForKey, &Index);
    gST->ConIn->ReadKeyStroke(gST->ConIn, &Key);
    
    gST->ConOut->SetCursorPosition(gST->ConOut, 25, 2);
    gST->ConOut->OutputString(gST->ConOut, L"                            "); // Clear line    
    gBS->Stall(250000);
    
    while (CurrentLength < TargetLength) {
        // CPU turn
        gST->ConOut->SetCursorPosition(gST->ConOut, 32, 2);
        gST->ConOut->SetAttribute(gST->ConOut, EFI_LIGHTCYAN);
        gST->ConOut->OutputString(gST->ConOut, L"UEFI SAYS");

        UINT16 BtnIndex = GetRandom(4);
        Sequence[CurrentLength] = BtnIndex;
        CurrentLength++;
        FlashSequence(Sequence, CurrentLength, TargetLength);

        // Player turn
        gST->ConOut->SetCursorPosition(gST->ConOut, 32, 2);
        gST->ConOut->SetAttribute(gST->ConOut, EFI_LIGHTMAGENTA);
        gST->ConOut->OutputString(gST->ConOut, L"YOUR TURN");

        
        gST->ConIn->Reset(gST->ConIn, FALSE);
    
        for (INTN StepIndex = 0; StepIndex < CurrentLength; StepIndex++) {
                UINTN Index;
                gBS->WaitForEvent(1, &gST->ConIn->WaitForKey, &Index);
                gST->ConIn->ReadKeyStroke(gST->ConIn, &Key); 
                
                // Map keys to buttons
                if      (Key.ScanCode == SCAN_LEFT)  PlayerInput = 0;
                else if (Key.ScanCode == SCAN_RIGHT) PlayerInput = 1;
                else if (Key.ScanCode == SCAN_UP)    PlayerInput = 2;
                else if (Key.ScanCode == SCAN_DOWN)  PlayerInput = 3;
                
                // Any other key pressed is ignored
                else {
                    StepIndex--; 
                    continue; 
                }
                
                FlashButton(&Buttons[PlayerInput], 200);
    
                // Game over
                if (PlayerInput != Sequence[StepIndex]) {
                    gST->RuntimeServices->ResetSystem(EfiResetShutdown, EFI_SUCCESS, 0, NULL);
                }
        }
    }
    
    gST->ConOut->EnableCursor(gST->ConOut, TRUE);
    gST->ConOut->SetAttribute(gST->ConOut, EFI_LIGHTGRAY | EFI_BACKGROUND_BLACK);
    gST->ConOut->ClearScreen(gST->ConOut);
    
    return EFI_ABORTED; // Continue to Boot
}
