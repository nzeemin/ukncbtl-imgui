
#include "stdafx.h"
#include "Main.h"
#include "Emulator.h"
#include "emubase/Emubase.h"


//////////////////////////////////////////////////////////////////////


int     m_Mode = (MemoryViewMode)0;
int     m_NumeralMode = MEMMODENUM_OCT;
uint16_t m_wBaseAddress = 0;
bool    m_okMemoryByteMode = false;


//////////////////////////////////////////////////////////////////////


void MemoryView_ImGuiDraw();
uint16_t MemoryView_GetWordFromMemory(uint16_t address, bool& okValid, int& addrtype, uint16_t& wChanged);


//////////////////////////////////////////////////////////////////////


void MemoryView_ImGuiWidget()
{
    ImGui::Begin("Memory");

    ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
    if (ImGui::BeginTabBar("MyTabBar", tab_bar_flags))
    {
        if (ImGui::BeginTabItem(" RAM0 "))
        {
            m_Mode = MEMMODE_RAM0;
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem(" RAM1 "))
        {
            m_Mode = MEMMODE_RAM1;
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem(" RAM2 "))
        {
            m_Mode = MEMMODE_RAM2;
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem(" ROM "))
        {
            m_Mode = MEMMODE_ROM;
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem(" CPU "))
        {
            m_Mode = MEMMODE_CPU;
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem(" PPU "))
        {
            m_Mode = MEMMODE_PPU;
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
        ImGui::SameLine();
        ImGui::TextUnformatted("     ");
        //ImGui::SameLine();
        //ImGui::SmallButton("Goto");
        ImGui::SameLine();
        if (ImGui::SmallButton("Options"))
            ImGui::OpenPopup("Memory_context");
    }

    if (g_pBoard != nullptr)
    {
        MemoryView_ImGuiDraw();
    }

    if (ImGui::BeginPopupContextItem("Memory_context"))
    {
        //ImGui::Selectable("Goto Address...");
        //ImGui::Separator();
        bool selectedWords = !m_okMemoryByteMode;
        if (ImGui::MenuItem("Words", nullptr, &selectedWords)) m_okMemoryByteMode = false;
        bool selectedBytes = m_okMemoryByteMode;
        if (ImGui::MenuItem("Bytes", nullptr, &selectedBytes)) m_okMemoryByteMode = true;
        ImGui::Separator();

        bool selectedOctal = (m_NumeralMode == MEMMODENUM_OCT);
        if (ImGui::MenuItem("Octal", nullptr, &selectedOctal)) m_NumeralMode = MEMMODENUM_OCT;
        bool selectedHex = (m_NumeralMode == MEMMODENUM_HEX);
        if (ImGui::MenuItem("Hex", nullptr, &selectedHex)) m_NumeralMode = MEMMODENUM_HEX;

        ImGui::EndPopup();
    }

    ImGui::End();
}

const char* ADDRESS_LINE_OCT_WORDS = "  addr   0      2      4      6      10     12     14     16     ";
const char* ADDRESS_LINE_OCT_BYTES = "  addr   0   1   2   3   4   5   6   7   10  11  12  13  14  15  16  17   ";
const char* ADDRESS_LINE_HEX_WORDS = " addr  0    2    4    6    8    a    c    e    ";
const char* ADDRESS_LINE_HEX_BYTES = " addr  0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f   ";

void MemoryView_ImGuiDraw()
{
    ImVec2 vMin = ImGui::GetWindowContentRegionMin();
    ImVec2 vMax = ImGui::GetWindowContentRegionMax();

    const char* addressline;
    if (m_NumeralMode == MEMMODENUM_OCT && !m_okMemoryByteMode)
        addressline = ADDRESS_LINE_OCT_WORDS;
    else if (m_NumeralMode == MEMMODENUM_OCT && m_okMemoryByteMode)
        addressline = ADDRESS_LINE_OCT_BYTES;
    else if (m_okMemoryByteMode)
        addressline = ADDRESS_LINE_HEX_BYTES;
    else
        addressline = ADDRESS_LINE_HEX_WORDS;
    ImGui::TextUnformatted(addressline);

    ImVec2 remainingSize = ImGui::GetContentRegionAvail();
    //ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(2.0f, 0));
    ImGui::BeginChild("MemoryScrollPane", remainingSize, ImGuiChildFlags_Border, ImGuiWindowFlags_AlwaysVerticalScrollbar);

    ImVec2 spaceSize = ImGui::CalcTextSize(" ");
    float space = spaceSize.x;

    ImGuiListClipper clipper;
    clipper.Begin(65536 / 16, ImGui::GetTextLineHeightWithSpacing());
    while (clipper.Step())
    {
        for (int line = clipper.DisplayStart; line < clipper.DisplayEnd; line++)
        {
            uint16_t address = line * 16; //m_wBaseAddress;
            uint16_t lineAddress = address;
            if (m_NumeralMode == MEMMODENUM_OCT)
                ImGui::TextDisabled("%06o", address);
            else
                ImGui::TextDisabled("%04x", address);
            ImGui::SameLine(0.0f, space * 2.0f);

            TCHAR wchars[17] = { 0 };
            for (int j = 0; j < 8; j++)    // Draw words
            {
                int addrtype;
                bool okValid;
                uint16_t wChanged;
                uint16_t word = MemoryView_GetWordFromMemory(address, okValid, addrtype, wChanged);

                //TODO if (address == m_wCurrentAddress)

                if (okValid)
                {
                    //if (addrtype == ADDRTYPE_ROM)
                    //    ::SetTextColor(hdc, colorMemoryRom);
                    //else
                    //    ::SetTextColor(hdc, (wChanged != 0) ? colorChanged : colorText);

                    if (m_NumeralMode == MEMMODENUM_OCT && !m_okMemoryByteMode)
                    {
                        ImGui::Text("%06o", word); ImGui::SameLine(0.0f, space);
                    }
                    else if (m_NumeralMode == MEMMODENUM_OCT && m_okMemoryByteMode)
                    {
                        ImGui::Text("%03o", (word & 0xff)); ImGui::SameLine(0.0f, space);
                        ImGui::Text("%03o", (word >> 8)); ImGui::SameLine(0.0f, space);
                    }
                    else if (m_NumeralMode == MEMMODENUM_HEX && !m_okMemoryByteMode)
                    {
                        ImGui::Text("%04x", word); ImGui::SameLine(0.0f, space);
                    }
                    else if (m_NumeralMode == MEMMODENUM_HEX && m_okMemoryByteMode)
                    {
                        ImGui::Text("%02x", (word & 0xff)); ImGui::SameLine(0.0f, space);
                        ImGui::Text("%02x", (word >> 8)); ImGui::SameLine(0.0f, space);
                    }
                }
                else  // !okValid
                {
                    //::SetTextColor(hdc, colorMemoryNA);
                    const char* pretext;
                    const char* posttext;
                    if (m_NumeralMode == MEMMODENUM_OCT && !m_okMemoryByteMode)
                    {
                        pretext = "  ";  posttext = "   ";
                    }
                    else if (m_NumeralMode == MEMMODENUM_OCT && m_okMemoryByteMode)
                    {
                        pretext = "  ";  posttext = "    ";
                    }
                    else if (m_NumeralMode == MEMMODENUM_HEX && !m_okMemoryByteMode)
                    {
                        pretext = " ";  posttext = "  ";
                    }
                    else
                    {
                        pretext = "  ";  posttext = "  ";
                    }

                    ImGui::TextUnformatted(pretext);
                    ImGui::SameLine(0.0f, 0.0f);
                    if (addrtype == ADDRTYPE_IO)
                    {
                        //::SetTextColor(hdc, colorMemoryIO);
                        ImGui::TextUnformatted("IO");
                    }
                    else
                    {
                        ImGui::TextUnformatted("NA");
                    }
                    ImGui::SameLine(0.0f, 0.0f);
                    ImGui::TextUnformatted(posttext);
                    ImGui::SameLine(0.0f, 0.0f);
                }

                // Prepare characters to draw at right
                uint8_t ch1 = (word & 0xff);
                TCHAR wch1 = Translate_KOI8R(ch1);
                if (ch1 < 32) wch1 = _T('·');
                wchars[j * 2] = wch1;
                uint8_t ch2 = (word >> 8);
                TCHAR wch2 = Translate_KOI8R(ch2);
                if (ch2 < 32) wch2 = _T('·');
                wchars[j * 2 + 1] = wch2;

                address += 2;
            }
            ImGui::NewLine();

            //ImGui::Text("%S", wchars); //ImGui::SameLine();

            //TODO
            //// Draw characters at right
            //char ascii[33] = { 0 };
            //WideCharToMultiByte(CP_UTF8, 0, wchars, 16, ascii, 32, NULL, NULL); //TODO
            //ImGui::TextUnformatted(ascii);

            //y += ImGui::GetTextLineHeightWithSpacing();
            //if (y >= vMax.y - ImGui::GetTextLineHeightWithSpacing())
            //    break;
        }
    }

    ImGui::EndChild();
    //ImGui::PopStyleVar();
}

uint16_t MemoryView_GetWordFromMemory(uint16_t address, bool& okValid, int& addrtype, uint16_t& wChanged)
{
    uint16_t word = 0;
    okValid = TRUE;
    addrtype = ADDRTYPE_NONE;
    wChanged = 0;
    bool okHalt = false;

    switch (m_Mode)
    {
    case MEMMODE_RAM0:
    case MEMMODE_RAM1:
    case MEMMODE_RAM2:
        word = g_pBoard->GetRAMWord(m_Mode, address);
        wChanged = Emulator_GetChangeRamStatus(m_Mode, address);
        break;
    case MEMMODE_ROM:  // ROM - only 32 Kbytes
        if (address < 0100000)
            okValid = false;
        else
        {
            addrtype = ADDRTYPE_ROM;
            word = g_pBoard->GetROMWord(address - 0100000);
        }
        break;
    case MEMMODE_CPU:
        okHalt = g_pBoard->GetCPU()->IsHaltMode();
        word = g_pBoard->GetCPUMemoryController()->GetWordView(address, okHalt, FALSE, &addrtype);
        okValid = (addrtype != ADDRTYPE_IO) && (addrtype != ADDRTYPE_DENY);
        wChanged = Emulator_GetChangeRamStatus(ADDRTYPE_RAM12, address);
        break;
    case MEMMODE_PPU:
        okHalt = g_pBoard->GetPPU()->IsHaltMode();
        word = g_pBoard->GetPPUMemoryController()->GetWordView(address, okHalt, FALSE, &addrtype);
        okValid = (addrtype != ADDRTYPE_IO) && (addrtype != ADDRTYPE_DENY);
        if (address < 0100000)
            wChanged = Emulator_GetChangeRamStatus(ADDRTYPE_RAM0, address);
        break;
    }

    return word;
}


//////////////////////////////////////////////////////////////////////
