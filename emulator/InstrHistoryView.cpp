
#include "stdafx.h"
#include "Main.h"
#include "Emulator.h"
#include "emubase/Emubase.h"


//////////////////////////////////////////////////////////////////////


static bool m_HistoryScrollToEnd = true;  // Scroll to the most recent instruction after Stop or Step


//////////////////////////////////////////////////////////////////////


// Details of the hovered interrupt record: words are vector, new PC, new PSW
static void InstrHistoryView_DrawInterruptTooltip(ImVec2 pos, const InstructionRecord& record)
{
    ImGui::SetNextWindowPos(pos);
    if (!ImGui::BeginTooltip())
        return;

    if (ImGui::BeginTable("HistoryTooltip", 2))
    {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();  ImGui::TextDisabled("PC");
        ImGui::TableNextColumn();  ImGui::Text("%06o", record.pc);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();  ImGui::TextDisabled("Vector");
        ImGui::TableNextColumn();  ImGui::Text("%06o %S", record.words[0], Emulator_GetInterruptName(record.words[0]));

        ImGui::TableNextRow();
        ImGui::TableNextColumn();  ImGui::TextDisabled("New PC");
        ImGui::TableNextColumn();  ImGui::Text("%06o", record.words[1]);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();  ImGui::TextDisabled("New PSW");
        ImGui::TableNextColumn();  ImGui::Text("%06o", record.words[2]);

        ImGui::EndTable();
    }

    ImGui::EndTooltip();
}

// Details of the hovered instruction
static void InstrHistoryView_DrawTooltip(ImVec2 pos, const InstructionRecord& record, int instrLength, const TCHAR* strInstr, const TCHAR* strArg)
{
    ImGui::SetNextWindowPos(pos);  // Right under the line, where the disasm text starts
    if (!ImGui::BeginTooltip())
        return;

    if (ImGui::BeginTable("HistoryTooltip", 2))
    {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();  ImGui::TextDisabled("PC");
        ImGui::TableNextColumn();  ImGui::Text("%06o", record.pc);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();  ImGui::TextDisabled("Opcode");
        ImGui::TableNextColumn();
        for (int i = 0; i < instrLength && i < 3; i++)
        {
            if (i > 0)
                ImGui::SameLine();
            ImGui::Text("%06o", record.words[i]);
        }

        ImGui::TableNextRow();
        ImGui::TableNextColumn();  ImGui::TextDisabled("Disasm");
        ImGui::TableNextColumn();  ImGui::Text("%S %S", strInstr, strArg);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();  ImGui::TextDisabled("PSW");
        ImGui::TableNextColumn();  ImGui::Text("%06o", record.psw);

        ImGui::EndTable();
    }

    ImGui::EndTooltip();
}

// Instruction history of the current processor, shown only when the emulator is stopped
void InstrHistoryView_ImGuiWidget()
{
    ImGui::Begin("Instr History");

    if (g_okEmulatorRunning)
        m_HistoryScrollToEnd = true;
    else
    {
        int length = Emulator_GetHistoryLength(g_okDebugCpuPpu);

        // After Step the most recent instruction changes; scroll to it again
        static int lastLength = -1;
        static uint16_t lastPc = 0;
        uint16_t newestPc = (length > 0) ? Emulator_GetHistoryRecord(g_okDebugCpuPpu, 0).pc : 0;
        if (length != lastLength || newestPc != lastPc)
            m_HistoryScrollToEnd = true;
        lastLength = length;
        lastPc = newestPc;

        // At most INSTR_HISTORY_SIZE rows, so no clipper: rows have the real height and scrolling stays consistent
        for (int line = 0; line < length; line++)
        {
            // Oldest instruction on top, the most recent one at the bottom
            const InstructionRecord& record = Emulator_GetHistoryRecord(g_okDebugCpuPpu, length - 1 - line);

            TCHAR strInstr[8] = { 0 };
            TCHAR strArg[32] = { 0 };
            int instrLength = 0;
            if (!record.interrupt)  // For an interrupt record the words are not an instruction
                instrLength = DisassembleInstruction(record.words, record.pc, strInstr, strArg);

            // Selectable row gives the hover highlight; the text is drawn over it
            ImVec2 lineMin = ImGui::GetCursorScreenPos();
            ImGui::PushID(line);
            if (ImGui::Selectable("##row", false)) {}
            ImGui::PopID();
            bool isHovered = ImGui::IsItemHovered();
            ImGui::SetCursorScreenPos(lineMin);

            ImGui::TextDisabled("%3d ", length - 1 - line);
            ImGui::SameLine();
            ImGui::TextDisabled("%06o ", record.pc);
            ImGui::SameLine();
            ImVec2 disasmPos = ImGui::GetCursorScreenPos();
            if (record.interrupt)  // words: vector, new PC, new PSW
                ImGui::TextColored(g_colorJumpLine, "INT %06o %S -> %06o",
                        record.words[0], Emulator_GetInterruptName(record.words[0]), record.words[1]);
            else
                ImGui::Text("%-7S %S", strInstr, strArg);

            if (isHovered)
            {
                ImVec2 tooltipPos(disasmPos.x, lineMin.y + ImGui::GetTextLineHeightWithSpacing());
                if (record.interrupt)
                    InstrHistoryView_DrawInterruptTooltip(tooltipPos, record);
                else
                    InstrHistoryView_DrawTooltip(tooltipPos, record, instrLength, strInstr, strArg);
            }
        }

        // Show the most recent instruction at the bottom
        if (m_HistoryScrollToEnd)
        {
            ImGui::SetScrollHereY(1.0f);
            m_HistoryScrollToEnd = false;
        }
    }

    ImGui::End();
}


//////////////////////////////////////////////////////////////////////
