#include "stdafx.h"
#include "Main.h"
#include "Emulator.h"
#include "emubase/Emubase.h"


//////////////////////////////////////////////////////////////////////


uint32_t* m_bits = nullptr;  // Screen buffer

ImTextureID g_ScreenTextureID = ImTextureID();

bool m_okScreenViewFocused = false;

BYTE m_ScreenKeyState[256];
const int KEYEVENT_QUEUE_SIZE = 32;
WORD m_ScreenKeyQueue[KEYEVENT_QUEUE_SIZE];
int m_ScreenKeyQueueTop = 0;
int m_ScreenKeyQueueBottom = 0;
int m_ScreenKeyQueueCount = 0;
void ScreenView_PutKeyEventToQueue(WORD keyevent);
WORD ScreenView_GetKeyEventFromQueue();

BOOL bEnter = FALSE;
BOOL bNumpadEnter = FALSE;
BOOL bEntPressed = FALSE;


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
    ImVec2 size(vMax.x - vMin.x, vMax.y - vMin.y);
    ImGui::Image(g_ScreenTextureID, size);

    if (ImGui::IsWindowFocused() && ImGui::IsItemHovered())
        ImGui::SetMouseCursor(ImGuiMouseCursor_TextInput);

    m_okScreenViewFocused = ImGui::IsWindowFocused();

    ImGui::End();
}

void ScreenView_PutKeyEventToQueue(WORD keyevent)
{
    if (m_ScreenKeyQueueCount == KEYEVENT_QUEUE_SIZE) return;  // Full queue

    m_ScreenKeyQueue[m_ScreenKeyQueueTop] = keyevent;
    m_ScreenKeyQueueTop++;
    if (m_ScreenKeyQueueTop >= KEYEVENT_QUEUE_SIZE)
        m_ScreenKeyQueueTop = 0;
    m_ScreenKeyQueueCount++;
}

WORD ScreenView_GetKeyEventFromQueue()
{
    if (m_ScreenKeyQueueCount == 0) return 0;  // Empty queue

    WORD keyevent = m_ScreenKeyQueue[m_ScreenKeyQueueBottom];
    m_ScreenKeyQueueBottom++;
    if (m_ScreenKeyQueueBottom >= KEYEVENT_QUEUE_SIZE)
        m_ScreenKeyQueueBottom = 0;
    m_ScreenKeyQueueCount--;

    return keyevent;
}

const BYTE arrPcscan2UkncscanLat[256] =    // ЛАТ
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
const BYTE arrPcscan2UkncscanRus[256] =    // РУС
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
        BYTE keys[256];
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
            BYTE newstate = keys[scan];
            BYTE oldstate = m_ScreenKeyState[scan];
            if ((newstate & 128) != (oldstate & 128))  // Key state changed - key pressed or released
            {
                const BYTE* pTable = nullptr;
                BYTE pcscan = (BYTE)scan;
                BYTE ukncscan;
                if (oldstate & 128)
                {
                    pTable = ((oldstate & KEYB_LAT) != 0) ? arrPcscan2UkncscanLat : arrPcscan2UkncscanRus;
                    m_ScreenKeyState[scan] = 0;
                }
                else
                {
                    pTable = ((ukncRegister & KEYB_LAT) != 0) ? arrPcscan2UkncscanLat : arrPcscan2UkncscanRus;
                    m_ScreenKeyState[scan] = (BYTE)((newstate & 128) | ukncRegister);
                }
                ukncscan = pTable[pcscan];
                if (ukncscan != 0)
                {
                    BYTE pressed = newstate & 128;
                    WORD keyevent = MAKEWORD(ukncscan, pressed);
                    ScreenView_PutKeyEventToQueue(keyevent);
                    //KeyboardView_KeyEvent(ukncscan, pressed);
                }
            }
        }
    }

    // Process the keyboard queue
    WORD keyevent;
    while ((keyevent = ScreenView_GetKeyEventFromQueue()) != 0)
    {
        bool pressed = ((keyevent & 0x8000) != 0);
        BYTE ukncscan = LOBYTE(keyevent);
        g_pBoard->KeyboardEvent(ukncscan, pressed);
    }
}


//////////////////////////////////////////////////////////////////////
