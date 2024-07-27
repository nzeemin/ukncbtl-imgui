
#include "stdafx.h"
#include "Main.h"
#include "Emulator.h"
#include "emubase/Emubase.h"


//////////////////////////////////////////////////////////////////////
// Processor


void ProcessorView_ImGuiWidget()
{
    const CProcessor* pProc = g_okDebugCpuPpu ? g_pBoard->GetCPU() : g_pBoard->GetPPU();

    ImGui::Begin(g_okDebugCpuPpu ? " CPU ###Processor" : " PPU ###Processor");

    TCHAR buffer[17];
    for (int regno = 0; regno <= 7; regno++)
    {
        LPCTSTR regname = REGISTER_NAME[regno];
        uint16_t regval = pProc->GetReg(regno);
        ImGui::TextDisabled("%S ", regname);
        ImGui::SameLine(0.0f, 0.0f);
        ImGui::Text("%06o %04X", regval, regval);
        ImGui::SameLine();
        PrintBinaryValue(buffer, pProc->GetReg(regno));
        ImGui::Text("%S", buffer);
    }

    uint16_t cpcval = pProc->GetCPC();
    PrintBinaryValue(buffer, cpcval);
    ImGui::TextDisabled("PC'"); ImGui::SameLine(0.0f, 0.0f);
    ImGui::Text("%06o %04X %S", cpcval, cpcval, buffer);
    ImGui::TextDisabled("                      HP  TNZVC");

    uint16_t pswval = pProc->GetPSW();
    PrintBinaryValue(buffer, pswval);
    ImGui::TextDisabled("PS "); ImGui::SameLine(0.0f, 0.0f);
    ImGui::Text("%06o", pswval); ImGui::SameLine();
    ImGui::TextUnformatted(pProc->IsHaltMode() ? "HALT" : "USER");
    ImGui::SameLine(); ImGui::Text("%S", buffer);

    uint16_t cpswval = pProc->GetCPSW();
    PrintBinaryValue(buffer, cpswval);
    ImGui::TextDisabled("PS'"); ImGui::SameLine(0.0f, 0.0f);
    ImGui::Text("%06o", cpswval); ImGui::SameLine(); ImGui::Text("    "); ImGui::SameLine(); ImGui::Text("%S", buffer);

    ImGui::Separator();
    ImGui::BeginDisabled(!g_okDebugCpuPpu);
    if (ImGui::Button(" PPU "))
    {
        g_okDebugCpuPpu = false;
        Settings_SetDebugCpuPpu(FALSE);
    }
    ImGui::EndDisabled();
    ImGui::SameLine(0.0f, 0.0f);
    ImGui::BeginDisabled(g_okDebugCpuPpu);
    if (ImGui::Button(" CPU "))
    {
        g_okDebugCpuPpu = true;
        Settings_SetDebugCpuPpu(TRUE);
    }
    ImGui::EndDisabled();

    ImGui::End();
}


//////////////////////////////////////////////////////////////////////
// Stack


void StackView_ImGuiWidget()
{
    CProcessor* pProc = g_okDebugCpuPpu ? g_pBoard->GetCPU() : g_pBoard->GetPPU();

    uint16_t current = pProc->GetReg(6) & ~1;
    bool okExec = false;

    // Reading from memory into the buffer
    uint16_t memory[14];
    int addrtype[14];
    const CMemoryController* pMemCtl = pProc->GetMemoryController();
    for (int idx = 0; idx < 14; idx++)
    {
        memory[idx] = pMemCtl->GetWordView(
            (uint16_t)(current + idx * 2 - 12), pProc->IsHaltMode(), okExec, addrtype + idx);
    }

    ImGui::Begin("Stack");

    uint16_t address = current - 12;
    for (int index = 0; index < 14; index++)
    {
        if (address == current)
            ImGui::TextDisabled(" SP ");
        else
        {
            if (index < 6)
                ImGui::TextDisabled("-%02o ", 12 - index * 2);
            else
                ImGui::TextDisabled("+%02o ", index * 2 - 12);
        }
        ImGui::SameLine(0.0f, 0.0f);
        ImGui::Text("%06o ", address);
        ImGui::SameLine();
        ImGui::Text("%06o ", memory[index]);

        address += 2;
    }

    ImGui::End();
}


//////////////////////////////////////////////////////////////////////
// Breakpoints

uint16_t m_BreakpointsContextAddress = 0177777;
char m_BreakpointBuf[7] = { 0 };

void Breakpoints_ImGuiWidget()
{
    ImGui::Begin("Breakpoints");

    ImVec2 linesMin = ImGui::GetCursorScreenPos();
    ImVec2 linesMax;

    uint16_t addressBreakpointClick = 0177777;

    const uint16_t* pbps = g_okDebugCpuPpu ? Emulator_GetCPUBreakpointList() : Emulator_GetPPUBreakpointList();
    int bpcount = 0;
    if (pbps != nullptr && *pbps != 0177777)
    {
        while (*pbps != 0177777)
        {
            ImVec2 lineMin = ImGui::GetCursorScreenPos();
            ImVec2 lineMax = ImVec2(lineMin.x + ImGui::GetWindowWidth(), lineMin.y + ImGui::GetTextLineHeightWithSpacing());
            linesMax = lineMax;
            ImVec2 lineDeleteMax = ImVec2(lineMin.x + 24.0f, lineMax.y);
            bool isHovered = ImGui::IsMouseHoveringRect(lineMin, lineMax);
            bool isDeleteHovered = ImGui::IsMouseHoveringRect(lineMin, lineDeleteMax);

            if (isDeleteHovered)
            {
                ImGui::TextColored(g_colorBreak, ICON_FA_MINUS_CIRCLE);
                ImGui::SameLine();
            }

            ImGui::SetCursorPosX(26.0f);
            uint16_t address = *pbps;
            ImGui::Text("%06ho", address);

            if (isHovered && ImGui::IsMouseReleased(ImGuiMouseButton_Right))  // Context menu
            {
                m_BreakpointsContextAddress = address;
                ImGui::OpenPopup("Breakpoints_context");
            }
            if (isDeleteHovered && ImGui::IsMouseReleased(ImGuiMouseButton_Left))  // Click on Delete sign
                addressBreakpointClick = address;

            bpcount++;
            pbps++;
        }
    }

    if (addressBreakpointClick != 0177777)
        ConsoleView_DeleteBreakpoint(addressBreakpointClick);

    bool linesHovered = ImGui::IsMouseHoveringRect(linesMin, linesMax);
    if (ImGui::IsWindowHovered() && !linesHovered && ImGui::IsMouseReleased(ImGuiMouseButton_Right))  // Context menu in empty area
    {
        m_BreakpointsContextAddress = 0177777;
        ImGui::OpenPopup("Breakpoints_context");
    }

    if (ImGui::BeginPopupContextItem("Breakpoints_context"))
    {
        if (m_BreakpointsContextAddress != 0177777)
        {
            if (ImGui::Selectable("Remove"))
                ConsoleView_DeleteBreakpoint(m_BreakpointsContextAddress);
        }
        if (ImGui::Selectable("Remove All"))
            ConsoleView_DeleteAllBreakpoints();

        ImGui::EndPopup();
    }

    if (bpcount < MAX_BREAKPOINTCOUNT)
    {
        ImGui::TextDisabled(ICON_FA_PLUS_CIRCLE);
        ImGui::SameLine();
        ImGui::SetCursorPosX(26.0f);
        ImGuiInputTextFlags input_text_flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_EscapeClearsAll;
        uint16_t address;
        bool addressParsed = false;
        if (*m_BreakpointBuf != 0)
            addressParsed = ParseOctalValue(m_BreakpointBuf, &address);
        ImGui::PushItemWidth(ImGui::CalcTextSize("000000 ").x);
        if (ImGui::InputText("###BreakpointNew", m_BreakpointBuf, IM_ARRAYSIZE(m_BreakpointBuf), input_text_flags))
        {
            if (addressParsed)
            {
                ConsoleView_AddBreakpoint(address);
                *m_BreakpointBuf = 0;
            }
        }
        if (*m_BreakpointBuf != 0 && !addressParsed)
        {
            ImGui::SameLine();
            ImGui::Text(ICON_FA_EXCLAMATION_TRIANGLE);
        }
        ImGui::PopItemWidth();
    }

    ImGui::End();
}


//////////////////////////////////////////////////////////////////////
// Memory Schema


void MemorySchema_ImGuiWidget()
{
    CProcessor* pProc = g_okDebugCpuPpu ? g_pBoard->GetCPU() : g_pBoard->GetPPU();

    const float schemah = 13.4f;  // Schema height in text lines
    const float schemah8 = schemah / 8.0f; // Height of 1/8 of the schema
    const float schemaw = 10.5f;  // Schema width in font character sizes

    ImGui::Begin("Memory Schema");

    ImVec2 spaceSize = ImGui::CalcTextSize(" ");
    float space = spaceSize.x;
    float lineh = ImGui::GetTextLineHeightWithSpacing();
    ImVec2 pos0 = ImGui::GetCursorPos();
    ImVec2 spos0 = ImGui::GetCursorScreenPos();
    float ypos4 = pos0.y + lineh * 13.0f;  // Y for last line of text
    float xpost = pos0.x + space * 9.0f;  // X pos for signs on memory blocks
    float xpospcsp = pos0.x + space * (6.67f + schemaw + 1.0f);  // X pos for PC and SP signs

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 p1(spos0.x + space * 6.67f, spos0.y);  // left top
    ImVec2 p2(p1.x + space * schemaw, p1.y);  // right top
    ImVec2 p3(p2.x, p2.y + lineh * schemah);  // right bottom
    ImVec2 p4(p1.x, p3.y);  // left bottom
    ImU32 col = ImGui::ColorConvertFloat4ToU32(ImVec4(0.67f, 0.67f, 0.67f, 1.0f));
    draw_list->AddQuad(p1, p2, p3, p4, col);

    for (int i = 1; i < 8; i++)
    {
        bool fullline = (g_okDebugCpuPpu && i >= 7) || (!g_okDebugCpuPpu && i >= 4);

        float y = p4.y - lineh * i * schemah8;
        if (fullline)
            draw_list->AddLine({ p1.x, y }, { p2.x, y }, col);
        else
            draw_list->AddLine({ p1.x, y }, { p1.x + space, y }, col);
        uint16_t addr = (uint16_t)i * 020000;
        ImGui::SetCursorPos({ pos0.x, pos0.y + lineh * (13.4f - i * schemah8 - 0.33f) });
        ImGui::Text("%06ho", addr);
    }

    ImGui::SetCursorPos({ pos0.x, pos0.y + lineh * 13.0f });
    ImGui::TextUnformatted("000000");

    if (g_okDebugCpuPpu)  // CPU
    {
        ImGui::SetCursorPos({ xpost, ypos4 - lineh * schemah8 * 3.5f });
        ImGui::TextUnformatted("RAM12");

        ImGui::SetCursorPos({ xpost, ypos4 - lineh * schemah8 * 7.5f });
        if (pProc->IsHaltMode())
            ImGui::TextUnformatted("RAM12");
        else
            ImGui::TextUnformatted("I/O");
    }
    else  // PPU
    {
        const CMemoryController* pMemCtl = pProc->GetMemoryController();
        uint16_t value177054 = pMemCtl->GetPortView(0177054);

        float y = p1.y + lineh * 0.2f;
        draw_list->AddLine({ p1.x, y }, { p2.x, y }, col);
        ImGui::SetCursorPos({ pos0.x, pos0.y });
        ImGui::TextUnformatted("177000");

        ImGui::SetCursorPos({ xpost, ypos4 - lineh * schemah8 * 2.0f });
        ImGui::TextUnformatted("RAM0");

        // 100000-117777 - Window block 0
        ImGui::SetCursorPos({ xpost, ypos4 - lineh * schemah8 * 4.5f });
        if ((value177054 & 16) != 0)  // Port 177054 bit 4 set => RAM selected
            ImGui::TextUnformatted("RAM0");
        else if ((value177054 & 1) != 0)  // ROM selected
            ImGui::TextUnformatted("ROM");
        else if ((value177054 & 14) != 0)  // ROM cartridge selected
        {
            int slot = ((value177054 & 8) == 0) ? 1 : 2;
            int bank = (value177054 & 6) >> 1;
            const size_t buffersize = 10;
            ImGui::Text("Cart %d/%d", slot, bank);
        }

        // 120000-137777 - Window block 1
        ImGui::SetCursorPos({ xpost, ypos4 - lineh * schemah8 * 5.5f });
        if ((value177054 & 32) != 0)  // Port 177054 bit 5 set => RAM selected
            ImGui::TextUnformatted("RAM0");
        else
            ImGui::TextUnformatted("ROM");

        // 140000-157777 - Window block 2
        ImGui::SetCursorPos({ xpost, ypos4 - lineh * schemah8 * 6.5f });
        if ((value177054 & 64) != 0)  // Port 177054 bit 6 set => RAM selected
            ImGui::TextUnformatted("RAM0");
        else
            ImGui::TextUnformatted("ROM");

        // 160000-176777 - Window block 3
        ImGui::SetCursorPos({ xpost, ypos4 - lineh * schemah8 * 7.5f });
        if ((value177054 & 128) != 0)  // Port 177054 bit 7 set => RAM selected
            ImGui::TextUnformatted("RAM0");
        else
            ImGui::TextUnformatted("ROM");
    }

    uint16_t sp = pProc->GetSP();
    float ysp = (lineh * schemah) * sp / 65536.0f;
    draw_list->AddLine({ p2.x, p4.y - ysp }, { p2.x + space, p4.y - ysp }, col);
    ImGui::SetCursorPos({ xpospcsp, ypos4 - ysp });
    ImGui::TextUnformatted("SP");

    uint16_t pc = pProc->GetPC();
    float ypc = (lineh * schemah) * pc / 65536.0f;
    draw_list->AddLine({ p2.x, p4.y - ypc }, { p2.x + space, p4.y - ypc }, col);
    ImGui::SetCursorPos({ xpospcsp, ypos4 - ypc });
    ImGui::TextUnformatted("PC");

    ImGui::End();
}


//////////////////////////////////////////////////////////////////////
