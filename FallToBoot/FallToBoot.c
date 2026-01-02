#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/RngLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>

#define MAP_WIDTH  80
#define MAP_HEIGHT 125
#define VIEWPORT_HEIGHT 25

#define CHAR_WALL   L'#' 
#define CHAR_EMPTY  L' ' 
#define CHAR_PLAYER L'@' 
#define CHAR_GOAL   L'P'

#define COLOR_WALL   EFI_LIGHTGRAY
#define COLOR_EMPTY  EFI_BLACK
#define COLOR_PLAYER EFI_LIGHTMAGENTA
#define COLOR_GOAL   EFI_YELLOW
#define COLOR_DEATH  EFI_RED

#define DIR_NONE  0
#define DIR_LEFT  1
#define DIR_RIGHT 2


CHAR16 GameMap[MAP_HEIGHT][MAP_WIDTH];
UINTN  CurrentAttr = 9999; // Cache
INTN   GoalX;

UINT16 GetRandom(UINT16 Max) {
    UINT16 Rand;
    if (Max == 0) return 0;
    GetRandomNumber16(&Rand);
    return Rand % Max;
}

void Dig(INTN center_x, INTN center_y) {
    // Dig a [11;9;7]x[5;3;1] area centered at (center_x, center_y)
    INTN radius_y;
    INTN radius_x;
    if (center_y < MAP_HEIGHT - VIEWPORT_HEIGHT * 3) {
        radius_y = 2;
        radius_x = 5;
    } else if (center_y < MAP_HEIGHT - VIEWPORT_HEIGHT) {
        radius_y = 1;
        radius_x = 4;
    } else {
        radius_y = 0;
        radius_x = 3;

    }
    for (INTN y = center_y - radius_y; y <= center_y + radius_y; y++) {
        for (INTN x = center_x - radius_x; x <= center_x + radius_x; x++) {
            if (x >= 0 && x < MAP_WIDTH && y >= 0 && y < MAP_HEIGHT) {
                GameMap[y][x] = CHAR_EMPTY;
            }
        }
    }
}

void InitMap() {
    INTN x, y;
    
    // Walled map gen
    for (y = 0; y < MAP_HEIGHT; y++) {
        for (x = 0; x < MAP_WIDTH; x++) {
            GameMap[y][x] = CHAR_WALL;
        }
    }
    INTN CurrentY = 0;
    INTN CurrentX = MAP_WIDTH / 2;
    INTN TargetX = CurrentX;


    while (CurrentY < MAP_HEIGHT) {
        // Tunnel logic following a target X
        Dig(CurrentX, CurrentY);
        CurrentY++; // Dig Down
        
        if (CurrentY >= MAP_HEIGHT) break;

        if (CurrentX == TargetX) {
            INTN TunelDir = GetRandom(100); // Randomness
            if (TunelDir >= 50) continue; // 50% chance to keep straight
            
            INTN amount = GetRandom(MAP_WIDTH / 4) + 1; // 50% chance to turn around
            if (TunelDir < 25) { // Left
                TargetX = MAX(0, CurrentX - amount);
            } else { // Right
                TargetX = MIN(MAP_WIDTH, CurrentX + amount);
            }
        } else if (CurrentX < TargetX) {
            CurrentX++;
        } else if (CurrentX > TargetX) {
            CurrentX--;
        }
    }
    GoalX = CurrentX;
}

CHAR16 LineBuffer[MAP_WIDTH + 1];

void Render(INTN ScrollOffset, INTN PlayerX, INTN PlayerY){
    CHAR16 *MapRowPtr = &GameMap[ScrollOffset][0];

    gST->ConOut->SetAttribute(gST->ConOut, COLOR_WALL);

    for (INTN y = 0; y < VIEWPORT_HEIGHT; y++) {
            // Cursor at row begin
            gST->ConOut->SetCursorPosition(gST->ConOut, 0, y);

            // Pointer Copy entire row 
            CopyMem(LineBuffer, MapRowPtr, MAP_WIDTH * sizeof(CHAR16));
            
            LineBuffer[MAP_WIDTH] = L'\0';

            if (y == PlayerY) {
                LineBuffer[PlayerX] = L'\0'; // Split on player position
                
                // Map before Player
                gST->ConOut->OutputString(gST->ConOut, LineBuffer);
                
                // Player
                gST->ConOut->SetAttribute(gST->ConOut, COLOR_PLAYER);
                CHAR16 PlayerStr[2] = {CHAR_PLAYER, L'\0'};
                gST->ConOut->OutputString(gST->ConOut, PlayerStr);
                
                // Map after Player
                gST->ConOut->SetAttribute(gST->ConOut, COLOR_WALL);
                gST->ConOut->OutputString(gST->ConOut, &LineBuffer[PlayerX + 1]);

            } else {
                // Map Row only
                gST->ConOut->OutputString(gST->ConOut, LineBuffer);
            }

            if (y + ScrollOffset == MAP_HEIGHT - 1) { // Goal Row
                gST->ConOut->SetAttribute(gST->ConOut, COLOR_GOAL);
                gST->ConOut->SetCursorPosition(gST->ConOut, MAX(0,GoalX - 3), y);    
                gST->ConOut->OutputString(gST->ConOut, L" BOOT! ");
                        }

            // Next row
            MapRowPtr += MAP_WIDTH;
        }
}

EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
)
{
    EFI_INPUT_KEY Key;
    
    INTN PlayerX = MAP_WIDTH / 2;
    INTN PlayerY = 0;
    INTN ScrollOffset = 0;
    INTN Direction = DIR_NONE;
    BOOLEAN MapEnded = FALSE;

    gST->ConOut->ClearScreen(gST->ConOut);
    InitMap();

    Render(ScrollOffset, PlayerX, PlayerY);
    while (Direction == DIR_NONE) {
        // Wait for initial input
        gST->ConIn->ReadKeyStroke(gST->ConIn, &Key);
        if (Key.ScanCode == SCAN_LEFT) Direction = DIR_LEFT;
        else if (Key.ScanCode == SCAN_RIGHT) Direction = DIR_RIGHT;
    }

    // Timer 50ms (20 FPS)
    EFI_EVENT TimerEvent;
    gBS->CreateEvent(EVT_TIMER, TPL_CALLBACK, NULL, NULL, &TimerEvent);
    gBS->SetTimer(TimerEvent, TimerPeriodic, 500000); 

    UINTN Index;
    BOOLEAN GameOver = FALSE;

    while (!GameOver) {
        // Timer Sync
        gBS->WaitForEvent(1, &TimerEvent, &Index);

        // Input
        while (gST->ConIn->ReadKeyStroke(gST->ConIn, &Key) != EFI_NOT_READY) {
            if (Key.ScanCode == SCAN_LEFT) Direction = DIR_LEFT;
            else if (Key.ScanCode == SCAN_RIGHT) Direction = DIR_RIGHT;
            else if (Key.UnicodeChar == L'q') GameOver = TRUE;
        }

        // Movement
        if (Direction == DIR_RIGHT) {
            if (PlayerX < MAP_WIDTH - 1 && GameMap[ScrollOffset + PlayerY][PlayerX + 1] != CHAR_WALL) 
                PlayerX++;
        } else if (Direction == DIR_LEFT) {
            if (PlayerX > 0 && GameMap[ScrollOffset + PlayerY][PlayerX - 1] != CHAR_WALL) 
                PlayerX--;
        }

        // Scrolling Logic
        if (ScrollOffset + VIEWPORT_HEIGHT < MAP_HEIGHT) {
            ScrollOffset++; // Scroll normal
        } else {
            MapEnded = TRUE;
            PlayerY++; 
        }

        // Colisiones
        if (GameMap[ScrollOffset + PlayerY][PlayerX] == CHAR_WALL) GameOver = TRUE;
        if (MapEnded && PlayerY >= VIEWPORT_HEIGHT - 1) {
            // Win Condition
            gST->ConOut->SetAttribute(gST->ConOut, EFI_WHITE);
            gST->ConOut->ClearScreen(gST->ConOut);
            Print(L"\n\n    SYSTEM BOOT SUCCESFUL.    \n");
            gBS->Stall(2000000);
            break;
        }

        if (GameOver && MapEnded == FALSE) {
            gST->ConOut->SetAttribute(gST->ConOut, COLOR_DEATH);
            gST->ConOut->SetCursorPosition(gST->ConOut, PlayerX, PlayerY);
            Print(L"X");
            gBS->Stall(1000000);
            gST->RuntimeServices->ResetSystem(EfiResetShutdown, EFI_SUCCESS, 0, NULL);
        }

        // Update
        Render(ScrollOffset, PlayerX, PlayerY);
    }
    gBS->CloseEvent(TimerEvent);
    gST->ConOut->EnableCursor(gST->ConOut, TRUE);
    gST->ConOut->SetAttribute(gST->ConOut, EFI_LIGHTGRAY | EFI_BACKGROUND_BLACK);
    gST->ConOut->ClearScreen(gST->ConOut);

    return EFI_ABORTED;
}