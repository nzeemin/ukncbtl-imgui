/*  This file is part of UKNCBTL.
    UKNCBTL is free software: you can redistribute it and/or modify it under the terms
of the GNU Lesser General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.
    UKNCBTL is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU Lesser General Public License for more details.
    You should have received a copy of the GNU Lesser General Public License along with
UKNCBTL. If not, see <http://www.gnu.org/licenses/>. */

// ScreenView.cpp

#include "stdafx.h"
#include "Main.h"
#include "Emulator.h"
#include "emubase/Emubase.h"
#include "util/PngFile.h"


//////////////////////////////////////////////////////////////////////


uint32_t* m_bits = nullptr;  // Screen buffer

ImTextureID g_ScreenTextureID = ImTextureID();

bool m_okScreenViewFocused = false;

uint8_t m_ScreenKeyState[256];
const int KEYEVENT_QUEUE_SIZE = 32;
uint16_t m_ScreenKeyQueue[KEYEVENT_QUEUE_SIZE];
int m_ScreenKeyQueueTop = 0;
int m_ScreenKeyQueueBottom = 0;
int m_ScreenKeyQueueCount = 0;
void ScreenView_PutKeyEventToQueue(uint16_t keyevent);
uint16_t ScreenView_GetKeyEventFromQueue();

ImVec2 m_ScreenMouse = ImVec2(NAN, NAN);

BOOL bEnter = FALSE;
BOOL bNumpadEnter = FALSE;
BOOL bEntPressed = FALSE;

typedef void (CALLBACK* PREPARE_SCREENSHOT_CALLBACK)(const void* pSrcBits, void* pDestBits);

const char* ScreenView_GetScreenshotModeName(int screenshotMode);
void ScreenView_GetScreenshotSize(int screenshotMode, int* pwid, int* phei);
PREPARE_SCREENSHOT_CALLBACK ScreenView_GetScreenshotCallback(int screenshotMode);



//////////////////////////////////////////////////////////////////////


void ScreenView_Init()
{
    m_bits = static_cast<uint32_t*>(::calloc(UKNC_SCREEN_WIDTH * UKNC_SCREEN_HEIGHT * 4, 1));
}

void ScreenView_Done()
{
    ::free(m_bits);  m_bits = nullptr;
}

void ScreenView_ImGuiWidget()
{
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f)); // Set window background to black
    ImGui::Begin("Video");
    ImGui::PopStyleColor();

    ImVec2 vMin = ImGui::GetWindowContentRegionMin();
    ImVec2 vMax = ImGui::GetWindowContentRegionMax();

    int sizemode = Settings_GetScreenSizeMode();
    ImVec2 size;
    switch (sizemode)
    {
    default:  // Fill the box
        size = ImVec2(vMax.x - vMin.x, vMax.y - vMin.y);
        break;
    case ScreenSize4to3ratio:
        if ((vMax.x - vMin.x) / 4.0f <= (vMax.y - vMin.y) / 3.0f)
        {
            size.x = vMax.x - vMin.x;
            size.y = size.x / 4.0f * 3.0f;
        }
        {
            size.y = vMax.y - vMin.y;
            size.x = size.y / 3.0f * 4.0f;
        }
        break;
    case ScreenSize640x480:
        size = ImVec2(640.0f, 480.0f);
        break;
    case ScreenSize800x600:
        size = ImVec2(800.0f, 600.0f);
        break;
    case ScreenSize960x720:
        size = ImVec2(960.0f, 720.0f);
        break;
    case ScreenSize1280x960:
        size = ImVec2(1280.0f, 960.0f);
        break;
    case ScreenSize1600x1200:
        size = ImVec2(1600.0f, 1200.0f);
        break;
    }

    ImGui::Image(g_ScreenTextureID, size);

    ImVec2 vMousePos = ImGui::GetMousePos();
    ImVec2 vWindowPos = ImGui::GetWindowPos();
    vMousePos.x -= vWindowPos.x;  vMousePos.y -= vWindowPos.y;  // screen coords to window coords
    float mx = NAN, my = NAN;
    if (vMousePos.x >= vMin.x && vMousePos.x < vMax.x &&
        vMousePos.y >= vMin.y && vMousePos.y < vMax.y)
    {
        mx = (vMousePos.x - vMin.x) / size.x * UKNC_SCREEN_WIDTH;
        my = (vMousePos.y - vMin.y) / size.y * UKNC_SCREEN_HEIGHT;
        if (mx < 0 || mx > UKNC_SCREEN_WIDTH ||
            my < 0 || my > UKNC_SCREEN_HEIGHT)
        {
            mx = NAN;  my = NAN;
        }
    }
    m_ScreenMouse.x = mx;  m_ScreenMouse.y = my;

    if (ImGui::IsWindowFocused() && ImGui::IsItemHovered())
    {
        //TODO: Draw crosshair
        ImGui::SetMouseCursor(ImGuiMouseCursor_TextInput);
    }

    m_okScreenViewFocused = ImGui::IsWindowFocused();

    ImGui::End();
}

ImVec2 ScreenView_GetMousePos()
{
    return m_ScreenMouse;
}

// Choose color palette depending of screen mode
const uint32_t* ScreenView_GetPalette()
{
    const uint32_t* colors;
    int viewmode = Settings_GetScreenViewMode();
    switch (viewmode)
    {
    case RGBScreen:   colors = ScreenView_StandardRGBColors; break;
    case GrayScreen:  colors = ScreenView_StandardGrayColors; break;
    case GRBScreen:   colors = ScreenView_StandardGRBColors; break;
    default:          colors = ScreenView_StandardRGBColors; break;
    }

    return colors;
}

void ScreenView_PutKeyEventToQueue(uint16_t keyevent)
{
    if (m_ScreenKeyQueueCount == KEYEVENT_QUEUE_SIZE) return;  // Full queue

    m_ScreenKeyQueue[m_ScreenKeyQueueTop] = keyevent;
    m_ScreenKeyQueueTop++;
    if (m_ScreenKeyQueueTop >= KEYEVENT_QUEUE_SIZE)
        m_ScreenKeyQueueTop = 0;
    m_ScreenKeyQueueCount++;
}

uint16_t ScreenView_GetKeyEventFromQueue()
{
    if (m_ScreenKeyQueueCount == 0) return 0;  // Empty queue

    uint16_t keyevent = m_ScreenKeyQueue[m_ScreenKeyQueueBottom];
    m_ScreenKeyQueueBottom++;
    if (m_ScreenKeyQueueBottom >= KEYEVENT_QUEUE_SIZE)
        m_ScreenKeyQueueBottom = 0;
    m_ScreenKeyQueueCount--;

    return keyevent;
}

const uint8_t arrPcscan2UkncscanLat[256] =    // ЛАТ
{
    /*       0     1     2     3     4     5     6     7     8     9     a     b     c     d     e     f  */
    /*0*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0132, 0026, 0000, 0000, 0000, 0153, 0166, 0000,
    /*1*/    0105, 0000, 0000, 0000, 0107, 0000, 0000, 0000, 0000, 0000, 0000, 0006, 0000, 0000, 0000, 0000,
    /*2*/    0113, 0004, 0151, 0152, 0155, 0116, 0154, 0133, 0134, 0000, 0000, 0000, 0000, 0171, 0172, 0000,
    /*3*/    0176, 0030, 0031, 0032, 0013, 0034, 0035, 0016, 0017, 0177, 0000, 0000, 0000, 0000, 0000, 0000,
    /*4*/    0000, 0072, 0076, 0050, 0057, 0033, 0047, 0055, 0156, 0073, 0027, 0052, 0056, 0112, 0054, 0075,
    /*5*/    0053, 0067, 0074, 0111, 0114, 0051, 0137, 0071, 0115, 0070, 0157, 0000, 0000, 0106, 0000, 0000,
    /*6*/    0126, 0127, 0147, 0167, 0130, 0150, 0170, 0125, 0145, 0165, 0025, 0000, 0000, 0005, 0146, 0131,
    /*7*/    0010, 0011, 0012, 0014, 0015, 0172, 0152, 0151, 0171, 0000, 0004, 0155, 0000, 0000, 0000, 0000,
    /*8*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000,
    /*9*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000,
    /*a*/    0000, 0000, 0046, 0066, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000,
    /*b*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0174, 0110, 0117, 0175, 0135, 0173,
    /*c*/    0007, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000,
    /*d*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0036, 0136, 0037, 0077, 0000,
    /*e*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000,
    /*f*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000,
};
const uint8_t arrPcscan2UkncscanRus[256] =    // РУС
{
    /*       0     1     2     3     4     5     6     7     8     9     a     b     c     d     e     f  */
    /*0*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0132, 0026, 0000, 0000, 0000, 0153, 0166, 0000,
    /*1*/    0105, 0000, 0000, 0000, 0107, 0000, 0000, 0000, 0000, 0000, 0000, 0006, 0000, 0000, 0000, 0000,
    /*2*/    0113, 0004, 0151, 0152, 0174, 0116, 0154, 0133, 0134, 0000, 0000, 0000, 0000, 0171, 0172, 0000,
    /*3*/    0176, 0030, 0031, 0032, 0013, 0034, 0035, 0016, 0017, 0177, 0000, 0000, 0000, 0000, 0000, 0000,
    /*4*/    0000, 0047, 0073, 0111, 0071, 0051, 0072, 0053, 0074, 0036, 0075, 0056, 0057, 0115, 0114, 0037,
    /*5*/    0157, 0027, 0052, 0070, 0033, 0055, 0112, 0050, 0110, 0054, 0067, 0000, 0000, 0106, 0000, 0000,
    /*6*/    0126, 0127, 0147, 0167, 0130, 0150, 0170, 0125, 0145, 0165, 0025, 0000, 0000, 0005, 0146, 0131,
    /*7*/    0010, 0011, 0012, 0014, 0015, 0172, 0152, 0151, 0171, 0000, 0004, 0174, 0000, 0000, 0000, 0000,
    /*8*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000,
    /*9*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000,
    /*a*/    0000, 0000, 0046, 0066, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000,
    /*b*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0137, 0117, 0076, 0175, 0077, 0173,
    /*c*/    0007, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000,
    /*d*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0156, 0135, 0155, 0136, 0000,
    /*e*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000,
    /*f*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000,
};

void ScreenView_ScanKeyboard()
{
    if (!g_okEmulatorRunning) return;
    if (m_okScreenViewFocused)
    {
        // Read current keyboard state
        uint8_t keys[256];
        VERIFY(::GetKeyboardState(keys));
        if (keys[VK_RETURN] & 128)
        {
            if (bEnter && bNumpadEnter)
                keys[VK_RETURN + 1] = 128;
            if (!bEnter && bNumpadEnter)
            {
                keys[VK_RETURN] = 0;
                keys[VK_RETURN + 1] = 128;
            }
            bEntPressed = TRUE;
        }
        else
        {
            if (bEntPressed)
            {
                if (bEnter) keys[VK_RETURN + 1] = 128;
                if (bNumpadEnter) keys[VK_RETURN + 1] = 128;
            }
            else
            {
                bEnter = FALSE;
                bNumpadEnter = FALSE;
            }
            bEntPressed = FALSE;
        }
        // Выбираем таблицу маппинга в зависимости от флага РУС/ЛАТ в УКНЦ
        uint16_t ukncRegister = g_pBoard->GetKeyboardRegister();

        // Check every key for state change
        for (int scan = 0; scan < 256; scan++)
        {
            uint8_t newstate = keys[scan];
            uint8_t oldstate = m_ScreenKeyState[scan];
            if ((newstate & 128) != (oldstate & 128))  // Key state changed - key pressed or released
            {
                const uint8_t* pTable = nullptr;
                uint8_t pcscan = (uint8_t)scan;
                uint8_t ukncscan;
                if (oldstate & 128)
                {
                    pTable = ((oldstate & KEYB_LAT) != 0) ? arrPcscan2UkncscanLat : arrPcscan2UkncscanRus;
                    m_ScreenKeyState[scan] = 0;
                }
                else
                {
                    pTable = ((ukncRegister & KEYB_LAT) != 0) ? arrPcscan2UkncscanLat : arrPcscan2UkncscanRus;
                    m_ScreenKeyState[scan] = (uint8_t)((newstate & 128) | ukncRegister);
                }
                ukncscan = pTable[pcscan];
                if (ukncscan != 0)
                {
                    uint8_t pressed = newstate & 128;
                    uint16_t keyevent = MAKEWORD(ukncscan, pressed);
                    ScreenView_PutKeyEventToQueue(keyevent);
                    //KeyboardView_KeyEvent(ukncscan, pressed);
                }
            }
        }
    }

    // Process the keyboard queue
    uint16_t keyevent;
    while ((keyevent = ScreenView_GetKeyEventFromQueue()) != 0)
    {
        bool pressed = ((keyevent & 0x8000) != 0);
        uint8_t ukncscan = LOBYTE(keyevent);
        g_pBoard->KeyboardEvent(ukncscan, pressed);
    }
}

// External key event - e.g. from KeyboardView
void ScreenView_KeyEvent(uint8_t keyscan, BOOL pressed)
{
    ScreenView_PutKeyEventToQueue(MAKEWORD(keyscan, pressed ? 128 : 0));
}


//////////////////////////////////////////////////////////////////////

// Table for color conversion yrgb (4 bits) -> DWORD (32 bits)
const uint32_t ScreenView_RGBColorsForPNG[16 * 8] =
{
    0xFF000000, 0xFF000080, 0xFF008000, 0xFF008080, 0xFF800000, 0xFF800080, 0xFF808000, 0xFF808080,
    0xFF000000, 0xFF0000FF, 0xFF00FF00, 0xFF00FFFF, 0xFFFF0000, 0xFFFF00FF, 0xFFFFFF00, 0xFFFFFFFF,
    0xFF000000, 0xFF000060, 0xFF008000, 0xFF008060, 0xFF800000, 0xFF800060, 0xFF808000, 0xFF808060,
    0xFF000000, 0xFF0000DF, 0xFF00FF00, 0xFF00FFDF, 0xFFFF0000, 0xFFFF00DF, 0xFFFFFF00, 0xFFFFFFDF,
    0xFF000000, 0xFF000080, 0xFF006000, 0xFF006080, 0xFF800000, 0xFF800080, 0xFF806000, 0xFF806080,
    0xFF000000, 0xFF0000FF, 0xFF00DF00, 0xFF00DFFF, 0xFFFF0000, 0xFFFF00FF, 0xFFFFDF00, 0xFFFFDFFF,
    0xFF000000, 0xFF000060, 0xFF006000, 0xFF006060, 0xFF800000, 0xFF800060, 0xFF806000, 0xFF806060,
    0xFF000000, 0xFF0000DF, 0xFF00DF00, 0xFF00DFDF, 0xFFFF0000, 0xFFFF00DF, 0xFFFFDF00, 0xFFFFDFDF,
    0xFF000000, 0xFF000080, 0xFF008000, 0xFF008080, 0xFF600000, 0xFF600080, 0xFF608000, 0xFF608080,
    0xFF000000, 0xFF0000FF, 0xFF00FF00, 0xFF00FFFF, 0xFFDF0000, 0xFFDF00FF, 0xFFDFFF00, 0xFFDFFFFF,
    0xFF000000, 0xFF000060, 0xFF008000, 0xFF008060, 0xFF600000, 0xFF600060, 0xFF608000, 0xFF608060,
    0xFF000000, 0xFF0000DF, 0xFF00FF00, 0xFF00FFDF, 0xFFDF0000, 0xFFDF00DF, 0xFFDFFF00, 0xFFDFFFDF,
    0xFF000000, 0xFF000080, 0xFF006000, 0xFF006080, 0xFF600000, 0xFF600080, 0xFF606000, 0xFF606080,
    0xFF000000, 0xFF0000FF, 0xFF00DF00, 0xFF00DFFF, 0xFFDF0000, 0xFFDF00FF, 0xFFDFDF00, 0xFFDFDFFF,
    0xFF000000, 0xFF000060, 0xFF006000, 0xFF006060, 0xFF600000, 0xFF600060, 0xFF606000, 0xFF606060,
    0xFF000000, 0xFF0000DF, 0xFF00DF00, 0xFF00DFDF, 0xFFDF0000, 0xFFDF00DF, 0xFFDFDF00, 0xFFDFDFDF,
};
const uint32_t ScreenView_GRBColorsForPNG[16 * 8] =
{
    0xFF000000, 0xFF000080, 0xFF800000, 0xFF800080, 0xFF008000, 0xFF008080, 0xFF808000, 0xFF808080,
    0xFF000000, 0xFF0000FF, 0xFFFF0000, 0xFFFF00FF, 0xFF00FF00, 0xFF00FFFF, 0xFFFFFF00, 0xFFFFFFFF,
    0xFF000000, 0xFF000060, 0xFF800000, 0xFF800060, 0xFF008000, 0xFF008060, 0xFF808000, 0xFF808060,
    0xFF000000, 0xFF0000DF, 0xFFFF0000, 0xFFFF00DF, 0xFF00FF00, 0xFF00FFDF, 0xFFFFFF00, 0xFFFFFFDF,
    0xFF000000, 0xFF000080, 0xFF600000, 0xFF600080, 0xFF008000, 0xFF008080, 0xFF608000, 0xFF608080,
    0xFF000000, 0xFF0000FF, 0xFFDF0000, 0xFFDF00FF, 0xFF00FF00, 0xFF00FFFF, 0xFFDFFF00, 0xFFDFFFFF,
    0xFF000000, 0xFF000060, 0xFF600000, 0xFF600060, 0xFF008000, 0xFF008060, 0xFF608000, 0xFF608060,
    0xFF000000, 0xFF0000DF, 0xFFDF0000, 0xFFDF00DF, 0xFF00FF00, 0xFF00FFDF, 0xFFDFFF00, 0xFFDFFFDF,
    0xFF000000, 0xFF000080, 0xFF800000, 0xFF800080, 0xFF006000, 0xFF006080, 0xFF806000, 0xFF806080,
    0xFF000000, 0xFF0000FF, 0xFFFF0000, 0xFFFF00FF, 0xFF00DF00, 0xFF00DFFF, 0xFFFFDF00, 0xFFFFDFFF,
    0xFF000000, 0xFF000060, 0xFF800000, 0xFF800060, 0xFF006000, 0xFF006060, 0xFF806000, 0xFF806060,
    0xFF000000, 0xFF0000DF, 0xFFFF0000, 0xFFFF00DF, 0xFF00DF00, 0xFF00DFDF, 0xFFFFDF00, 0xFFFFDFDF,
    0xFF000000, 0xFF000080, 0xFF600000, 0xFF600080, 0xFF006000, 0xFF006080, 0xFF606000, 0xFF606080,
    0xFF000000, 0xFF0000FF, 0xFFDF0000, 0xFFDF00FF, 0xFF00DF00, 0xFF00DFFF, 0xFFDFDF00, 0xFFDFDFFF,
    0xFF000000, 0xFF000060, 0xFF600000, 0xFF600060, 0xFF006000, 0xFF006060, 0xFF606000, 0xFF606060,
    0xFF000000, 0xFF0000DF, 0xFFDF0000, 0xFFDF00DF, 0xFF00DF00, 0xFF00DFDF, 0xFFDFDF00, 0xFFDFDFDF,
};
// Table for color conversion, gray (black and white) display
const uint32_t ScreenView_GrayColorsForPNG[16 * 8] =
{
    0xFF000000, 0xFF242424, 0xFF484848, 0xFF6C6C6C, 0xFF909090, 0xFFB4B4B4, 0xFFD8D8D8, 0xFFFFFFFF,
    0xFF000000, 0xFF242424, 0xFF484848, 0xFF6C6C6C, 0xFF909090, 0xFFB4B4B4, 0xFFD8D8D8, 0xFFFFFFFF,
    0xFF000000, 0xFF242424, 0xFF484848, 0xFF6C6C6C, 0xFF909090, 0xFFB4B4B4, 0xFFD8D8D8, 0xFFFFFFFF,
    0xFF000000, 0xFF242424, 0xFF484848, 0xFF6C6C6C, 0xFF909090, 0xFFB4B4B4, 0xFFD8D8D8, 0xFFFFFFFF,
    0xFF000000, 0xFF242424, 0xFF484848, 0xFF6C6C6C, 0xFF909090, 0xFFB4B4B4, 0xFFD8D8D8, 0xFFFFFFFF,
    0xFF000000, 0xFF242424, 0xFF484848, 0xFF6C6C6C, 0xFF909090, 0xFFB4B4B4, 0xFFD8D8D8, 0xFFFFFFFF,
    0xFF000000, 0xFF242424, 0xFF484848, 0xFF6C6C6C, 0xFF909090, 0xFFB4B4B4, 0xFFD8D8D8, 0xFFFFFFFF,
    0xFF000000, 0xFF242424, 0xFF484848, 0xFF6C6C6C, 0xFF909090, 0xFFB4B4B4, 0xFFD8D8D8, 0xFFFFFFFF,
    0xFF000000, 0xFF242424, 0xFF484848, 0xFF6C6C6C, 0xFF909090, 0xFFB4B4B4, 0xFFD8D8D8, 0xFFFFFFFF,
    0xFF000000, 0xFF242424, 0xFF484848, 0xFF6C6C6C, 0xFF909090, 0xFFB4B4B4, 0xFFD8D8D8, 0xFFFFFFFF,
    0xFF000000, 0xFF242424, 0xFF484848, 0xFF6C6C6C, 0xFF909090, 0xFFB4B4B4, 0xFFD8D8D8, 0xFFFFFFFF,
    0xFF000000, 0xFF242424, 0xFF484848, 0xFF6C6C6C, 0xFF909090, 0xFFB4B4B4, 0xFFD8D8D8, 0xFFFFFFFF,
    0xFF000000, 0xFF242424, 0xFF484848, 0xFF6C6C6C, 0xFF909090, 0xFFB4B4B4, 0xFFD8D8D8, 0xFFFFFFFF,
    0xFF000000, 0xFF242424, 0xFF484848, 0xFF6C6C6C, 0xFF909090, 0xFFB4B4B4, 0xFFD8D8D8, 0xFFFFFFFF,
    0xFF000000, 0xFF242424, 0xFF484848, 0xFF6C6C6C, 0xFF909090, 0xFFB4B4B4, 0xFFD8D8D8, 0xFFFFFFFF,
    0xFF000000, 0xFF242424, 0xFF484848, 0xFF6C6C6C, 0xFF909090, 0xFFB4B4B4, 0xFFD8D8D8, 0xFFFFFFFF,
};

const uint32_t* ScreenView_GetPaletteForPng()
{
    const uint32_t* colors;
    int viewmode = Settings_GetScreenViewMode();
    switch (viewmode)
    {
    case RGBScreen:   colors = ScreenView_RGBColorsForPNG; break;
    case GrayScreen:  colors = ScreenView_GrayColorsForPNG; break;
    case GRBScreen:   colors = ScreenView_GRBColorsForPNG; break;
    default:          colors = ScreenView_GRBColorsForPNG; break;
    }

    return colors;
}

bool ScreenView_SaveScreenshot(const char* pFileName, int screenshotMode)
{
    IM_ASSERT(pFileName != NULL);

    void* pBits = ::calloc(UKNC_SCREEN_WIDTH * UKNC_SCREEN_HEIGHT, 4);
    const uint32_t* palette = ScreenView_GetPaletteForPng();
    Emulator_PrepareScreenRGB32(pBits, (const uint32_t*)palette);

    int scrwidth, scrheight;
    ScreenView_GetScreenshotSize(screenshotMode, &scrwidth, &scrheight);
    PREPARE_SCREENSHOT_CALLBACK callback = ScreenView_GetScreenshotCallback(screenshotMode);

    void* pScrBits = ::calloc(scrwidth * scrheight, 4);
    callback(pBits, pScrBits);
    ::free(pBits);

    bool result = PngFile_SaveImage(pFileName, pScrBits, scrwidth, scrheight);

    ::free(pScrBits);

    return result;
}


void CALLBACK PrepareScreenCopy(const void * pSrcBits, void * pDestBits);
void CALLBACK PrepareScreenUpscale(const void * pSrcBits, void * pDestBits);
void CALLBACK PrepareScreenUpscale2(const void * pSrcBits, void * pDestBits);
void CALLBACK PrepareScreenUpscale2d(const void * pSrcBits, void * pDestBits);
void CALLBACK PrepareScreenUpscale3(const void * pSrcBits, void * pDestBits);
void CALLBACK PrepareScreenUpscale4(const void * pSrcBits, void * pDestBits);
void CALLBACK PrepareScreenUpscale175(const void * pSrcBits, void * pDestBits);
void CALLBACK PrepareScreenUpscale5(const void * pSrcBits, void * pDestBits);

struct ScreenshotModeStruct
{
    int width;
    int height;
    PREPARE_SCREENSHOT_CALLBACK callback;
    const char* description;
}
static ScreenshotModeReference[] =
{
    {  640,  288, PrepareScreenCopy,        "640 x 288 Minimal" },
    {  640,  432, PrepareScreenUpscale,     "640 x 432 Upscaled to 1.5" },
    {  640,  576, PrepareScreenUpscale2,    "640 x 576 Scanlines" },
    {  640,  576, PrepareScreenUpscale2d,   "640 x 576 Doubled" },
    {  960,  576, PrepareScreenUpscale3,    "960 x 576 Scanlines" },
    {  960,  720, PrepareScreenUpscale4,    "960 x 720, 4:3" },
    { 1120,  864, PrepareScreenUpscale175,  "1120 x 864 Scanlines" },
    { 1280,  864, PrepareScreenUpscale5,    "1280 x 864 Scanlines" },
};
const int ScreenshotModeCount = sizeof(ScreenshotModeReference) / sizeof(ScreenshotModeStruct);

void ScreenView_GetScreenshotSize(int screenshotMode, int* pwid, int* phei)
{
    if (screenshotMode < 0 || screenshotMode >= ScreenshotModeCount)
        screenshotMode = 1;
    ScreenshotModeStruct* pinfo = ScreenshotModeReference + screenshotMode;
    *pwid = pinfo->width;
    *phei = pinfo->height;
}

PREPARE_SCREENSHOT_CALLBACK ScreenView_GetScreenshotCallback(int screenshotMode)
{
    if (screenshotMode < 0 || screenshotMode >= ScreenshotModeCount)
        screenshotMode = 1;
    ScreenshotModeStruct* pinfo = ScreenshotModeReference + screenshotMode;
    return pinfo->callback;
}

const char * ScreenView_GetScreenshotModeName(int screenshotMode)
{
    if (screenshotMode < 0 || screenshotMode >= ScreenshotModeCount)
        return nullptr;
    ScreenshotModeStruct* pinfo = ScreenshotModeReference + screenshotMode;
    return pinfo->description;
}

#define AVERAGERGB(a, b)  ( (((a) & 0xfefefeffUL) + ((b) & 0xfefefeffUL)) >> 1 )

void CALLBACK PrepareScreenCopy(const void * pSrcBits, void * pDestBits)
{
    memcpy(pDestBits, pSrcBits, UKNC_SCREEN_WIDTH * UKNC_SCREEN_HEIGHT * 4);
}

// Upscale screen from height 288 to 432
void CALLBACK PrepareScreenUpscale(const void * pSrcBits, void * pDestBits)
{
    const uint32_t* psrcbits = static_cast<const uint32_t*>(pSrcBits);
    uint32_t* pbits = static_cast<uint32_t*>(pDestBits);
    int ukncline = UKNC_SCREEN_HEIGHT - 1;
    for (int line = 431; line > 0; line--)
    {
        uint32_t* pdest = pbits + line * UKNC_SCREEN_WIDTH;
        if (line % 3 == 1)
        {
            const uint32_t* psrc1 = psrcbits + ukncline * UKNC_SCREEN_WIDTH;
            const uint32_t* psrc2 = psrcbits + (ukncline + 1) * UKNC_SCREEN_WIDTH;
            uint32_t* pdst1 = pdest;
            for (int i = 0; i < UKNC_SCREEN_WIDTH; i++)
            {
                *pdst1 = AVERAGERGB(*psrc1, *psrc2);
                psrc1++;  psrc2++;  pdst1++;
            }
        }
        else
        {
            const uint32_t* psrc = psrcbits + ukncline * UKNC_SCREEN_WIDTH;
            memcpy(pdest, psrc, UKNC_SCREEN_WIDTH * 4);
            ukncline--;
        }
    }
}

// Upscale screen from height 288 to 576 with "interlaced" effect
void PrepareScreenUpscale2(const void* pSrcBits, void* pDestBits)
{
    const uint32_t* psrcbits = static_cast<const uint32_t*>(pSrcBits);
    uint32_t* pbits = static_cast<uint32_t*>(pDestBits);
    for (int ukncline = 287; ukncline >= 0; ukncline--)
    {
        const uint32_t* psrc = psrcbits + ukncline * UKNC_SCREEN_WIDTH;
        uint32_t* pdest = pbits + (ukncline * 2) * UKNC_SCREEN_WIDTH;
        memcpy(pdest, psrc, UKNC_SCREEN_WIDTH * 4);

        pdest += UKNC_SCREEN_WIDTH;
        for (int i = 0; i < UKNC_SCREEN_WIDTH; i++)
            *pdest++ = 0xFF000000;
    }
}

// Upscale screen to twice height
void CALLBACK PrepareScreenUpscale2d(const void * pSrcBits, void * pDestBits)
{
    const uint32_t* psrcbits = static_cast<const uint32_t*>(pSrcBits);
    uint32_t* pbits = static_cast<uint32_t*>(pDestBits);
    for (int ukncline = 287; ukncline >= 0; ukncline--)
    {
        const uint32_t* psrc = psrcbits + ukncline * UKNC_SCREEN_WIDTH;
        uint32_t* pdest = pbits + (ukncline * 2) * UKNC_SCREEN_WIDTH;
        memcpy(pdest, psrc, UKNC_SCREEN_WIDTH * 4);

        pdest += UKNC_SCREEN_WIDTH;
        memcpy(pdest, psrc, UKNC_SCREEN_WIDTH * 4);
    }
}

// Upscale screen width 640->960, height 288->576 with "interlaced" effect
void CALLBACK PrepareScreenUpscale3(const void* pSrcBits, void* pDestBits)
{
    const uint32_t* psrcbits = static_cast<const uint32_t*>(pSrcBits);
    uint32_t* pbits = static_cast<uint32_t*>(pDestBits);
    for (int ukncline = 287; ukncline >= 0; ukncline--)
    {
        const uint32_t* psrc = psrcbits + ukncline * UKNC_SCREEN_WIDTH;
        psrc += UKNC_SCREEN_WIDTH - 1;
        uint32_t* pdest = pbits + (ukncline * 2) * 960;
        pdest += 960 - 1;
        for (int i = 0; i < UKNC_SCREEN_WIDTH / 2; i++)
        {
            uint32_t c1 = *psrc;  psrc--;
            uint32_t c2 = *psrc;  psrc--;
            uint32_t c12 = 0xFF000000 |
                (((c1 & 0xff) + (c2 & 0xff)) >> 1) |
                ((((c1 & 0xff00) + (c2 & 0xff00)) >> 1) & 0xff00) |
                ((((c1 & 0xff0000) + (c2 & 0xff0000)) >> 1) & 0xff0000);
            *pdest = c1;  pdest--;
            *pdest = c12; pdest--;
            *pdest = c2;  pdest--;
        }

        pdest += 960;
        for (int i = 0; i < 960; i++)
            *pdest++ = 0xFF000000;
    }
}

// Upscale screen width 640->960, height 288->720
void CALLBACK PrepareScreenUpscale4(const void * pSrcBits, void * pDestBits)
{
    const uint32_t* psrcbits = static_cast<const uint32_t*>(pSrcBits);
    uint32_t* pbits = static_cast<uint32_t*>(pDestBits);
    for (int ukncline = 0; ukncline < 288; ukncline += 2)
    {
        const uint32_t* psrc1 = psrcbits + (286 - ukncline) * UKNC_SCREEN_WIDTH;
        const uint32_t* psrc2 = psrc1 + UKNC_SCREEN_WIDTH;
        uint32_t* pdest0 = pbits + (286 - ukncline) / 2 * 5 * 960;
        uint32_t* pdest1 = pdest0 + 960;
        uint32_t* pdest2 = pdest1 + 960;
        uint32_t* pdest3 = pdest2 + 960;
        uint32_t* pdest4 = pdest3 + 960;
        for (int i = 0; i < UKNC_SCREEN_WIDTH / 2; i++)
        {
            uint32_t c1a = *(psrc1++);  uint32_t c1b = *(psrc1++);
            uint32_t c2a = *(psrc2++);  uint32_t c2b = *(psrc2++);
            uint32_t c1 = AVERAGERGB(c1a, c1b);
            uint32_t c2 = AVERAGERGB(c2a, c2b);
            uint32_t ca = AVERAGERGB(c1a, c2a);
            uint32_t cb = AVERAGERGB(c1b, c2b);
            uint32_t c = AVERAGERGB(ca, cb);
            (*pdest0++) = c1a;  (*pdest0++) = c1;  (*pdest0++) = c1b;
            (*pdest1++) = c1a;  (*pdest1++) = c1;  (*pdest1++) = c1b;
            (*pdest2++) = ca;   (*pdest2++) = c;   (*pdest2++) = cb;
            (*pdest3++) = c2a;  (*pdest3++) = c2;  (*pdest3++) = c2b;
            (*pdest4++) = c2a;  (*pdest4++) = c2;  (*pdest4++) = c2b;
        }
    }
}

// Upscale screen width 640->1120 (x1.75), height 288->864 (x3) with "interlaced" effect
void CALLBACK PrepareScreenUpscale175(const void * pSrcBits, void * pDestBits)
{
    const uint32_t* psrcbits = static_cast<const uint32_t*>(pSrcBits);
    uint32_t* pbits = static_cast<uint32_t*>(pDestBits);
    for (int ukncline = 287; ukncline >= 0; ukncline--)
    {
        const uint32_t* psrc = psrcbits + ukncline * UKNC_SCREEN_WIDTH;
        uint32_t* pdest1 = pbits + (ukncline * 3) * 1120;
        uint32_t* pdest2 = pdest1 + 1120;
        uint32_t* pdest3 = pdest2 + 1120;
        for (int i = 0; i < UKNC_SCREEN_WIDTH / 4; i++)
        {
            uint32_t c1 = *(psrc++);
            uint32_t c2 = *(psrc++);
            uint32_t c3 = *(psrc++);
            uint32_t c4 = *(psrc++);

            *(pdest1++) = *(pdest2++) = c1;
            *(pdest1++) = *(pdest2++) = AVERAGERGB(c1, c2);
            *(pdest1++) = *(pdest2++) = c2;
            *(pdest1++) = *(pdest2++) = AVERAGERGB(c2, c3);
            *(pdest1++) = *(pdest2++) = c3;
            *(pdest1++) = *(pdest2++) = AVERAGERGB(c3, c4);
            *(pdest1++) = *(pdest2++) = c4;
        }

        for (int i = 0; i < 1120; i++)
            *pdest3++ = 0xFF000000;
    }
}

// Upscale screen width 640->1280, height 288->864 with "interlaced" effect
void CALLBACK PrepareScreenUpscale5(const void * pSrcBits, void * pDestBits)
{
    const uint32_t* psrcbits = static_cast<const uint32_t*>(pSrcBits);
    uint32_t* pbits = static_cast<uint32_t*>(pDestBits);
    for (int ukncline = 287; ukncline >= 0; ukncline--)
    {
        const uint32_t* psrc = psrcbits + ukncline * UKNC_SCREEN_WIDTH;
        uint32_t* pdest = pbits + (ukncline * 3) * 1280;
        psrc += UKNC_SCREEN_WIDTH - 1;
        pdest += 1280 - 1;
        uint32_t* pdest2 = pdest + 1280;
        uint32_t* pdest3 = pdest2 + 1280;
        for (int i = 0; i < UKNC_SCREEN_WIDTH; i++)
        {
            uint32_t color = *psrc;  psrc--;
            *pdest = color;  pdest--;
            *pdest = color;  pdest--;
            *pdest2 = color;  pdest2--;
            *pdest2 = color;  pdest2--;
            *pdest3 = 0xFF000000;  pdest3--;
            *pdest3 = 0xFF000000;  pdest3--;
        }
    }
}


//////////////////////////////////////////////////////////////////////
