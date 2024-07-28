/*  This file is part of UKNCBTL.
    UKNCBTL is free software: you can redistribute it and/or modify it under the terms
of the GNU Lesser General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.
    UKNCBTL is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU Lesser General Public License for more details.
    You should have received a copy of the GNU Lesser General Public License along with
UKNCBTL. If not, see <http://www.gnu.org/licenses/>. */

#pragma once

#ifdef _DEBUG
#define APP_VERSION_STRING "DEBUG"
#else
#include "Version.h"
#endif

//////////////////////////////////////////////////////////////////////


#define MAX_LOADSTRING 100

extern LPCTSTR g_szTitle;  // The title bar text
extern TCHAR g_szWindowClass[MAX_LOADSTRING];      // Main window class name

extern HINSTANCE g_hInst; // current instance

extern double g_dFramesPercent;

extern bool g_okDebugCpuPpu;

extern bool g_okVsyncSwitchable;

extern uint32_t* m_bits;  // Screen buffer
extern ImTextureID g_ScreenTextureID;

// Derived colors based and calculated using the current style
extern ImVec4 g_colorPlay;              // Play sign in Disasm window
extern ImVec4 g_colorBreak;             // Breakpoint
extern ImVec4 g_colorJumpLine;          // Jump arrow
extern ImVec4 g_colorDisabledRed;       // Disabled color shifted to red
extern ImVec4 g_colorDisabledGreen;     // Disabled color shifted to green
extern ImVec4 g_colorDisabledBlue;      // Disabled color shifted to blue


//////////////////////////////////////////////////////////////////////
// Main Window

extern HWND g_hwnd;  // Main window handle

enum ColorIndices
{
    ColorDebugText          = 0,
    ColorDebugBackCurrent   = 1,
    ColorDebugValueChanged  = 2,
    ColorDebugPrevious      = 3,
    ColorDebugMemoryRom     = 4,
    ColorDebugMemoryIO      = 5,
    ColorDebugMemoryNA      = 6,
    ColorDebugValue         = 7,
    ColorDebugValueRom      = 8,
    ColorDebugSubtitles     = 9,
    ColorDebugJump          = 10,
    ColorDebugJumpYes       = 11,
    ColorDebugJumpNo        = 12,
    ColorDebugJumpHint      = 13,
    ColorDebugHint          = 14,
    ColorDebugBreakpoint    = 15,
    ColorDebugHighlight     = 16,
    ColorDebugBreakptZone   = 17,

    ColorIndicesCount       = 18,
};

void ScreenView_KeyEvent(BYTE keyscan, BOOL pressed);


//////////////////////////////////////////////////////////////////////
// Settings

void Settings_Init();
void Settings_Done();
BOOL Settings_GetWindowRect(RECT * pRect);
void Settings_SetWindowRect(const RECT * pRect);
void Settings_SetWindowMaximized(BOOL flag);
BOOL Settings_GetWindowMaximized();
void Settings_SetFloppyFilePath(int slot, LPCTSTR sFilePath);
void Settings_GetFloppyFilePath(int slot, LPTSTR buffer);
void Settings_SetCartridgeFilePath(int slot, LPCTSTR sFilePath);
void Settings_GetCartridgeFilePath(int slot, LPTSTR buffer);
void Settings_SetHardFilePath(int slot, LPCTSTR sFilePath);
void Settings_GetHardFilePath(int slot, LPTSTR buffer);
void Settings_SetScreenViewMode(int mode);
int  Settings_GetScreenViewMode();
void Settings_SetScreenSizeMode(int mode);
int  Settings_GetScreenSizeMode();
BOOL Settings_GetScreenVsync();
void Settings_SetScreenVsync(BOOL flag);
void Settings_SetScreenshotMode(int mode);
int  Settings_GetScreenshotMode();
BOOL Settings_GetDebugCpuPpu();
void Settings_SetDebugCpuPpu(BOOL flag);
void Settings_SetDebugMemoryMode(WORD mode);
WORD Settings_GetDebugMemoryMode();
void Settings_SetDebugMemoryAddress(WORD address);
WORD Settings_GetDebugMemoryAddress();
void Settings_SetDebugMemoryBase(WORD address);
WORD Settings_GetDebugMemoryBase();
BOOL Settings_GetDebugMemoryByte();
void Settings_SetDebugMemoryByte(BOOL flag);
void Settings_SetDebugMemoryNumeral(WORD mode);
WORD Settings_GetDebugMemoryNumeral();
void Settings_SetAutostart(BOOL flag);
BOOL Settings_GetAutostart();
void Settings_SetRealSpeed(WORD speed);
WORD Settings_GetRealSpeed();
void Settings_SetSound(BOOL flag);
BOOL Settings_GetSound();
void Settings_SetSoundVolume(WORD value);
BOOL Settings_GetSoundAY();
void Settings_SetSoundAY(BOOL flag);
BOOL Settings_GetSoundCovox();
void Settings_SetSoundCovox(BOOL flag);
WORD Settings_GetSoundVolume();
BOOL Settings_GetMouse();
void Settings_SetMouse(BOOL flag);
void Settings_SetToolbar(BOOL flag);
BOOL Settings_GetToolbar();
void Settings_SetKeyboard(BOOL flag);
BOOL Settings_GetKeyboard();
void Settings_SetTape(BOOL flag);
BOOL Settings_GetTape();
WORD Settings_GetSpriteAddress();
void Settings_SetSpriteAddress(WORD value);
WORD Settings_GetSpriteWidth();
void Settings_SetSpriteWidth(WORD value);
WORD Settings_GetSpriteMode();
void Settings_SetSpriteMode(WORD value);
void Settings_SetSerial(BOOL flag);
BOOL Settings_GetSerial();
void Settings_GetSerialPort(LPTSTR buffer);
void Settings_SetSerialPort(LPCTSTR sValue);
void Settings_GetSerialConfig(DCB * pDcb);
void Settings_SetSerialConfig(const DCB * pDcb);
void Settings_SetParallel(BOOL flag);
BOOL Settings_GetParallel();
void Settings_SetNetwork(BOOL flag);
BOOL Settings_GetNetwork();
int  Settings_GetNetStation();
void Settings_SetNetStation(int value);
void Settings_GetNetComPort(LPTSTR buffer);
void Settings_SetNetComPort(LPCTSTR sValue);
void Settings_GetNetComConfig(DCB * pDcb);
void Settings_SetNetComConfig(const DCB * pDcb);
int  Settings_GetColorScheme();
void Settings_SetColorScheme(int value);

LPCTSTR Settings_GetColorFriendlyName(ColorIndices colorIndex);
COLORREF Settings_GetColor(ColorIndices colorIndex);
COLORREF Settings_GetDefaultColor(ColorIndices colorIndex);
void Settings_SetColor(ColorIndices colorIndex, COLORREF color);


//////////////////////////////////////////////////////////////////////
// Options

//extern bool Option_ShowHelp;
extern int Option_AutoBoot;

extern void SetVSync();


//////////////////////////////////////////////////////////////////////
// Widgets

// MainWindow
void MainWindow_RestoreImages();
void MainWindow_SetColorSheme(int scheme);
void ImGuiMainMenu();
void ControlView_ImGuiWidget();

// ScreenView

enum ScreenViewMode
{
    RGBScreen = 0,
    GRBScreen = 1,
    GrayScreen = 2,
};

enum ScreenSizeMode
{
    ScreenSizeFill = 0,
    ScreenSize4to3ratio = 1,
    ScreenSize640x480 = 2,
    ScreenSize800x600 = 3,
    ScreenSize960x720 = 4,
    ScreenSize1280x960 = 5,
    ScreenSize1600x1200 = 6,
};

void ScreenView_Init();
void ScreenView_Done();
void ScreenView_ImGuiWidget();
ImVec2 ScreenView_GetMousePos();
void ScreenView_ScanKeyboard();
const uint32_t* ScreenView_GetPalette();
bool ScreenView_SaveScreenshot(const char* pFileName, int screenshotMode);
const char* ScreenView_GetScreenshotModeName(int screenshotMode);

// DebugViews
void ProcessorView_ImGuiWidget();
void StackView_ImGuiWidget();
void Breakpoints_ImGuiWidget();
void MemorySchema_ImGuiWidget();

// DisasmView
void DisasmView_ImGuiWidget();

// ConsoleView
void ConsoleView_Init();
void ConsoleView_ImGuiWidget();
void ConsoleView_Print(const char* message);
void ConsoleView_StepInto();
void ConsoleView_StepOver();
void ConsoleView_AddBreakpoint(uint16_t address);
void ConsoleView_DeleteBreakpoint(uint16_t address);
void ConsoleView_DeleteAllBreakpoints();

// MemoryView

enum MemoryViewMode
{
    MEMMODE_RAM0 = 0,  // RAM plane 0
    MEMMODE_RAM1 = 1,  // RAM plane 1
    MEMMODE_RAM2 = 2,  // RAM plane 2
    MEMMODE_ROM  = 3,  // ROM
    MEMMODE_CPU  = 4,  // CPU memory
    MEMMODE_PPU  = 5,  // PPU memory
    MEMMODE_LAST = 5,  // Last mode number
};

enum MemoryViewNumeralMode
{
    MEMMODENUM_OCT = 0,
    MEMMODENUM_HEX = 1,
};

void MemoryView_ImGuiWidget();


//////////////////////////////////////////////////////////////////////
