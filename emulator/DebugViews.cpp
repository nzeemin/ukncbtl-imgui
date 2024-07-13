
#include "stdafx.h"
#include "Main.h"
#include "Emulator.h"
#include "emubase/Emubase.h"


//////////////////////////////////////////////////////////////////////


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
            ImGui::TextDisabled("SP>");
        else
            ImGui::TextUnformatted("   ");
        ImGui::SameLine(0.0f, 0.0f);
        ImGui::Text("%06o ", address);
        ImGui::SameLine();
        ImGui::Text("%06o ", memory[index]);

        address += 2;
    }

    ImGui::End();
}


//////////////////////////////////////////////////////////////////////


void Breakpoints_ImGuiWidget()
{
    ImGui::Begin("Breakpoints");

    uint16_t addressBreakpointClick = 0177777;

    const uint16_t* pbps = g_okDebugCpuPpu ? Emulator_GetCPUBreakpointList() : Emulator_GetPPUBreakpointList();
    if (pbps != nullptr && *pbps != 0177777)
    {
        while (*pbps != 0177777)
        {
            ImVec2 lineMin = ImGui::GetCursorScreenPos();
            //ImVec2 lineMax = ImVec2(lineMin.x + ImGui::GetWindowWidth(), lineMin.y + ImGui::GetTextLineHeightWithSpacing());
            ImVec2 lineDeleteMax = ImVec2(lineMin.x + 24.0f, lineMin.y + ImGui::GetTextLineHeightWithSpacing());

            bool isDeleteHovered = ImGui::IsMouseHoveringRect(lineMin, lineDeleteMax);

            if (isDeleteHovered)
            {
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), ICON_FA_MINUS_CIRCLE);
                ImGui::SameLine(0.0f, 0.0f);
            }

            ImGui::SetCursorPosX(24.0f);
            uint16_t address = *pbps;
            ImGui::Text("%06ho", address);

            if (isDeleteHovered && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
                addressBreakpointClick = address;

            pbps++;
        }
    }

    if (addressBreakpointClick != 0177777)
    {
        if (g_okDebugCpuPpu)
            Emulator_RemoveCPUBreakpoint(addressBreakpointClick);
        else
            Emulator_RemovePPUBreakpoint(addressBreakpointClick);
    }

    ImGui::End();
}


//////////////////////////////////////////////////////////////////////
