
#include "stdafx.h"
#include "Main.h"
#include "Emulator.h"
#include "emubase/Emubase.h"


//////////////////////////////////////////////////////////////////////


const int DISASM_MEMWINDOW = 32;
const int MAX_DISASMLINECOUNT = 50;

uint16_t m_wDisasmBaseAddr = 0;

uint16_t m_DisasmMemory[DISASM_MEMWINDOW + 2];
int m_DisasmAddrType[DISASM_MEMWINDOW + 2];

bool  m_okDisasmJumpPredict;
TCHAR m_strDisasmHint[42] = { 0 };
TCHAR m_strDisasmHint2[42] = { 0 };


const float xPosCurrentSign = 24.0f;
const float xPosAddress = 40.0f;
const float xPosHints = 440.0f;


//////////////////////////////////////////////////////////////////////
// Forward declarations

void Disasm_DrawForRunningEmulator();
void Disasm_DrawForStoppedEmulator(CProcessor* pProc);


//////////////////////////////////////////////////////////////////////


void DisasmView_ImGuiWidget()
{
    ImGui::Begin("Disasm");

    CProcessor* pProc = (g_okDebugCpuPpu) ? g_pBoard->GetCPU() : g_pBoard->GetPPU();
    IM_ASSERT(pProc != nullptr);
    m_wDisasmBaseAddr = pProc->GetPC();

    const CMemoryController* pMemCtl = pProc->GetMemoryController();

    // Read from the processor memory to the buffer
    uint16_t current = m_wDisasmBaseAddr;
    for (int idx = 0; idx < DISASM_MEMWINDOW; idx++)
    {
        m_DisasmMemory[idx] = pMemCtl->GetWordView(
            (uint16_t)(current + idx * 2 - 10), pProc->IsHaltMode(), true, m_DisasmAddrType + idx);
    }

    if (!g_okEmulatorRunning)
    {
        if (ImGui::Button(ICON_FA_PLAY " Run  "))
            Emulator_Start();
    }
    else
    {
        if (ImGui::Button(ICON_FA_PAUSE " Stop "))
            Emulator_Stop();
    }
    ImGui::SameLine();
    ImGui::BeginDisabled(g_okEmulatorRunning);
    if (ImGui::Button("Step"))
        ConsoleView_StepInto();
    ImGui::SameLine();
    if (ImGui::Button("Step Over"))
        ConsoleView_StepOver();
    ImGui::EndDisabled();

    if (g_okEmulatorRunning)
        Disasm_DrawForRunningEmulator();
    else
        Disasm_DrawForStoppedEmulator(pProc);

    ImGui::End();
}

void Disasm_DrawForRunningEmulator()
{
    uint16_t address = m_wDisasmBaseAddr;

    for (int i = 0; i < 5; i++)
        ImGui::Text("");

    int index = 5;
    ImGui::SetCursorPosX(xPosCurrentSign);
    ImGui::TextColored(g_colorPlay, ICON_FA_PLAY);
    ImGui::SameLine(0.0f, 0.0f);
    ImGui::SetCursorPosX(xPosAddress);
    ImGui::Text("%06o ", address);
    ImGui::SameLine();
    ImGui::Text("%06o ", m_DisasmMemory[index]);

    TCHAR strInstr[8];
    TCHAR strArg[32];
    DisassembleInstruction(m_DisasmMemory + index, address, strInstr, strArg);
    ImGui::SameLine();
    ImGui::Text("%S %S", strInstr, strArg);
}

void DisasmView_DrawJump(ImDrawList* draw_list, ImVec2 lineMin, int delta, float cyLine)
{
    ImU32 col = ImGui::ColorConvertFloat4ToU32(g_colorJumpLine);

    int dist = abs(delta);
    if (dist < 2) dist = 2;
    if (dist > 20) dist = 16;

    float yTo = lineMin.y + delta * cyLine;

    ImVec2 p1(lineMin.x + cyLine * 0.2f, lineMin.y + cyLine / 2.0f);  // point from
    ImVec2 p2(lineMin.x + dist * cyLine * 0.32f, p1.y);
    ImVec2 p3(lineMin.x + dist * cyLine * 0.9f, yTo);
    ImVec2 p4(lineMin.x, yTo - cyLine * 0.1f);  // point to
    draw_list->AddBezierCubic(p1, p2, p3, p4, col, 1.0);

    // Arrow sides
    ImVec2 p5(lineMin.x + cyLine * 0.8f, yTo);  // upper side
    ImVec2 p6(lineMin.x + cyLine * 0.8f, yTo);  // down side
    if (delta > 0)  // jump down
    {
        p5.y -= cyLine * 0.27f;
        p6.y += cyLine * 0.01f;
    }
    else  // jump up
    {
        p5.y -= cyLine * 0.10f;
        p6.y += cyLine * 0.15f;
    }
    draw_list->AddTriangleFilled(p4, p5, p6, col);
}

// В режиме останова делаем подробный дизасм
void Disasm_DrawForStoppedEmulator(CProcessor* pProc)
{
    const CMemoryController* pMemCtl = pProc->GetMemoryController();

    //TODO: Рисовать строки только до нижней границы окна
    //ImVec2 vMax = ImGui::GetWindowContentRegionMax();

    uint16_t proccurrent = m_wDisasmBaseAddr;

    uint16_t address = m_wDisasmBaseAddr - 10;
    uint16_t disasmfrom = m_wDisasmBaseAddr;
    uint16_t addressBreakpointClick = 0177777;

    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    int lineindex = 0;
    int length = 0;
    for (int index = 0; index < DISASM_MEMWINDOW; index++)
    {
        ImVec2 lineMin = ImGui::GetCursorScreenPos();
        //ImVec2 lineMax = ImVec2(lineMin.x + ImGui::GetWindowWidth(), lineMin.y + ImGui::GetTextLineHeightWithSpacing());
        ImVec2 lineBreakpointMax = ImVec2(lineMin.x + 40.0f, lineMin.y + ImGui::GetTextLineHeightWithSpacing());

        bool isBreakpoint = Emulator_IsBreakpoint(g_okDebugCpuPpu, address);
        bool isBreakpointHovered = ImGui::IsMouseHoveringRect(lineMin, lineBreakpointMax);
        if (isBreakpoint || isBreakpointHovered)
        {
            ImVec4 colorBreak = isBreakpoint
                ? (isBreakpointHovered ? g_colorDisabledRed : g_colorBreak)
                : ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled);
            ImGui::TextColored(colorBreak, ICON_FA_CIRCLE);
            ImGui::SameLine(0.0f, 0.0f);

            if (isBreakpointHovered && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
                addressBreakpointClick = address;
        }
        if (address == proccurrent)
        {
            ImGui::SetCursorPosX(xPosCurrentSign);
            ImGui::TextColored(g_colorPlay, ICON_FA_PLAY);
            ImGui::SameLine(0.0f, 0.0f);
        }

        int memorytype = m_DisasmAddrType[index];
        ImVec4 colorAddr = (memorytype == ADDRTYPE_ROM) ? g_colorDisabledBlue : ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled);
        ImGui::SetCursorPosX(xPosAddress);
        ImGui::TextColored(colorAddr, "%06o ", address);
        ImGui::SameLine();
        ImGui::TextDisabled("%06o ", m_DisasmMemory[index]);
        ImGui::SameLine();

        if (address >= disasmfrom && length == 0)
        {
            TCHAR strInstr[8];
            TCHAR strArg[32];
            length = DisassembleInstruction(m_DisasmMemory + index, address, strInstr, strArg);
            ImGui::Text("%-7S %S", strInstr, strArg);
            ImGui::SameLine(0.0f, 0.0f);

            int jumpdelta;
            if (Disasm_CheckForJump(m_DisasmMemory + index, &jumpdelta) && abs(jumpdelta) < 40)
            {
                ImVec2 vec(ImGui::GetCursorScreenPos().x, lineMin.y);
                DisasmView_DrawJump(draw_list, vec, jumpdelta, ImGui::GetTextLineHeightWithSpacing());
            }

            if (address == proccurrent)  // For current instruction, prepare the instruction hints
            {
                *m_strDisasmHint = 0; *m_strDisasmHint2 = 0;
                m_okDisasmJumpPredict = Disasm_GetJumpConditionHint(m_DisasmMemory + index, pProc, pMemCtl, m_strDisasmHint);
                ImGui::SetCursorPosX(xPosHints);
                if (*m_strDisasmHint == 0)  // we don't have the jump hint
                {
                    Disasm_GetInstructionHint(m_DisasmMemory + index, pProc, pMemCtl, m_strDisasmHint, m_strDisasmHint2);
                    ImGui::TextDisabled("%S", m_strDisasmHint);
                    ImGui::SameLine(0.0f, 0.0f);
                }
                else  // we have jump hint
                {
                    ImVec4 color = m_okDisasmJumpPredict ? g_colorDisabledGreen : g_colorDisabledRed;
                    ImGui::TextColored(color, "%S", m_strDisasmHint);
                    ImGui::SameLine(0.0f, 0.0f);
                }
            }
            if (address == proccurrent + 2 && *m_strDisasmHint2 != 0)
            {
                ImGui::SetCursorPosX(xPosHints);
                ImGui::TextDisabled("%S", m_strDisasmHint2);
                ImGui::SameLine(0.0f, 0.0f);
            }
        }
        if (length > 0) length--;

        ImGui::NewLine();
        address += 2;
        lineindex++;
        if (lineindex >= MAX_DISASMLINECOUNT)
            break;
    }

    if (addressBreakpointClick != 0177777)
    {
        bool isBreakpoint = Emulator_IsBreakpoint(g_okDebugCpuPpu, addressBreakpointClick);
        if (isBreakpoint)
        {
            if (g_okDebugCpuPpu)
                Emulator_RemoveCPUBreakpoint(addressBreakpointClick);
            else
                Emulator_RemovePPUBreakpoint(addressBreakpointClick);
        }
        else
        {
            if (g_okDebugCpuPpu)
                Emulator_AddCPUBreakpoint(addressBreakpointClick);
            else
                Emulator_AddPPUBreakpoint(addressBreakpointClick);
        }
    }
}


//////////////////////////////////////////////////////////////////////
