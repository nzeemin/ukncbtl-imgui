
#include "stdafx.h"
#include "Main.h"
#include "Emulator.h"
#include "emubase/Emubase.h"
#include <string>
#include <vector>


//////////////////////////////////////////////////////////////////////


char m_ConsoleInputBuf[64];
std::vector<std::string> m_ConsoleItems;
bool m_ScrollToBottom = false;
bool m_ReturnFocus = false;

const char* MESSAGE_UNKNOWN_COMMAND = "  Unknown command.";
const char* MESSAGE_WRONG_VALUE = "  Wrong value.";
const char* MESSAGE_INVALID_REGNUM = "  Invalid register number, 0..7 expected.";


//////////////////////////////////////////////////////////////////////


int ConsoleView_TextEditCallback(ImGuiInputTextCallbackData* data);

void ConsoleView_ClearConsole();
std::string ConsoleView_GetConsolePrompt();
void ConsoleView_DoConsoleCommand();


//////////////////////////////////////////////////////////////////////


void ConsoleView_Init()
{
    ConsoleView_Print("UKNCBTL emulator TODO version information.");
    ConsoleView_Print("Use 'h' command to show help.");
    ConsoleView_Print("");
}

void ConsoleView_ImGuiWidget()
{
    ImGui::Begin("Console");

    const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
    if (ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve), ImGuiChildFlags_None,
        ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_AlwaysVerticalScrollbar))
    {
        if (ImGui::BeginPopupContextWindow())
        {
            if (ImGui::Selectable("Clear"))
                ConsoleView_ClearConsole();
            ImGui::EndPopup();
        }

        //TODO: ImGuiListClipper
        for (std::string item : m_ConsoleItems)
        {
            bool iscommand = (item.length() > 12) && (item[0] == 'P' || item[0] == 'C') && (item[1] == 'P' && item[2] == 'U' && item[3] == ':');
            if (iscommand)
                ImGui::TextDisabled(item.c_str());
            else
                ImGui::TextUnformatted(item.c_str());
        }
    }

    // Keep up at the bottom of the scroll region if we were already at the bottom at the beginning of the frame.
    // Using a scrollbar or mouse-wheel will take away from the bottom edge.
    if (m_ScrollToBottom || (/*AutoScroll &&*/ ImGui::GetScrollY() >= ImGui::GetScrollMaxY()))
        ImGui::SetScrollHereY(1.0f);
    m_ScrollToBottom = false;

    ImGui::EndChild();
    ImGui::Separator();

    auto prompt = ConsoleView_GetConsolePrompt();
    ImGui::Text(prompt.c_str());
    ImGui::SameLine();

    ImGui::BeginDisabled(g_okEmulatorRunning);
    ImGuiInputTextFlags input_text_flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_EscapeClearsAll /* | ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory*/;
    if (m_ReturnFocus)
        ImGui::SetKeyboardFocusHere();
    m_ReturnFocus = false;
    if (ImGui::InputText("###Input", m_ConsoleInputBuf, IM_ARRAYSIZE(m_ConsoleInputBuf), input_text_flags, ConsoleView_TextEditCallback, nullptr))
    {
        ConsoleView_DoConsoleCommand();
        m_ReturnFocus = true;
    }
    ImGui::EndDisabled();

    ImGui::End();
}

int ConsoleView_TextEditCallback(ImGuiInputTextCallbackData* /*data*/)
{
    return 0;
}

CProcessor* ConsoleView_GetCurrentProcessor()
{
    if (g_okDebugCpuPpu)
        return g_pBoard->GetCPU();
    else
        return g_pBoard->GetPPU();
}

void ConsoleView_PrintFormat(const char* pszFormat, ...)
{
    const size_t buffersize = 512;
    char buffer[buffersize];

    va_list ptr;
    va_start(ptr, pszFormat);
    vsnprintf_s(buffer, buffersize, buffersize - 1, pszFormat, ptr);
    va_end(ptr);

    ConsoleView_Print(buffer);
}

void ConsoleView_Print(const char* message)
{
    m_ConsoleItems.push_back(message);
    m_ScrollToBottom = true;
}

void ConsoleView_ClearConsole()
{
    m_ConsoleItems.clear();
}

std::string ConsoleView_GetConsolePrompt()
{
    const CProcessor* pProc = ConsoleView_GetCurrentProcessor();
    uint16_t pcval = pProc->GetPC();

    char buffer[16];
    sprintf(buffer, "%S:%06o>", pProc->GetName(), pcval);
    return buffer;
}

void ConsoleView_StepInto()
{
    // Put command to console prompt
    strcpy(m_ConsoleInputBuf, "s");
    // Execute command
    ConsoleView_DoConsoleCommand();
}
void ConsoleView_StepOver()
{
    // Put command to console prompt
    strcpy(m_ConsoleInputBuf, "so");
    // Execute command
    ConsoleView_DoConsoleCommand();
}
void ConsoleView_DeleteBreakpoint(uint16_t address)
{
    // Put command to console prompt
    sprintf(m_ConsoleInputBuf, "bc%06ho", address);
    // Execute command
    ConsoleView_DoConsoleCommand();
}
void ConsoleView_DeleteAllBreakpoints()
{
    // Put command to console prompt
    strcpy(m_ConsoleInputBuf, "bc");
    // Execute command
    ConsoleView_DoConsoleCommand();
}

// Print disassembled instructions
// Return value: number of words disassembled
int ConsoleView_PrintDisassemble(CProcessor* pProc, WORD address, BOOL okOneInstr, BOOL okShort)
{
    CMemoryController* pMemCtl = pProc->GetMemoryController();
    bool okHaltMode = pProc->IsHaltMode();

    const int nWindowSize = 30;
    WORD memory[nWindowSize + 2];
    int addrtype;
    for (WORD i = 0; i < nWindowSize + 2; i++)
        memory[i] = pMemCtl->GetWordView(address + i * 2, okHaltMode, true, &addrtype);

    char buffer[64];

    int totalLength = 0;
    int lastLength = 0;
    int length = 0;
    for (int index = 0; index < nWindowSize; index++)  // Рисуем строки
    {
        WORD value = memory[index];

        if (length > 0)
        {
            if (!okShort)
            {
                snprintf(buffer, IM_ARRAYSIZE(buffer) - 1, "  %06o  %06o", address, value);
                ConsoleView_Print(buffer);
            }
        }
        else
        {
            if (okOneInstr && index > 0)
                break;
            TCHAR instr[8];
            TCHAR args[32];
            length = DisassembleInstruction(memory + index, address, instr, args);
            lastLength = length;
            if (index + length > nWindowSize)
                break;
            if (okShort)
                snprintf(buffer, IM_ARRAYSIZE(buffer) - 1, "  %06o  %-7S %S", address, instr, args);
            else
                snprintf(buffer, IM_ARRAYSIZE(buffer) - 1, "  %06o  %06o  %-7S %S", address, value, instr, args);
            ConsoleView_Print(buffer);
        }
        length--;
        address += 2;
        totalLength++;
    }

    return totalLength;
}


//////////////////////////////////////////////////////////////////////
// Console commands handlers

struct ConsoleCommandParams
{
    const char* commandText;
    int         paramReg1;
    uint16_t    paramOct1, paramOct2;
};

void ConsoleView_CmdShowHelp(const ConsoleCommandParams& /*params*/)
{
    ConsoleView_Print("Console command list:");
    ConsoleView_Print("  c          Clear console log");
    //ConsoleView_Print("  d          Disassemble from PC; use D for short format")
    //ConsoleView_Print("  dXXXXXX    Disassemble from address XXXXXX")
    ConsoleView_Print("  g          Go; free run");
    ConsoleView_Print("  gXXXXXX    Go; run and stop at address XXXXXX");
    //ConsoleView_Print("  m          Memory dump at current address")
    //ConsoleView_Print("  mXXXXXX    Memory dump at address XXXXXX")
    //ConsoleView_Print("  mrN        Memory dump at address from register N; N=0..7")
    //ConsoleView_Print("  mXXXXXX YYYYYY  Set memory value at address XXXXXX")
    ConsoleView_Print("  p          Switch to other processor");
    //ConsoleView_Print("  r          Show register values")
    //ConsoleView_Print("  rN         Show value of register N; N=0..7,ps")
    //ConsoleView_Print("  rN XXXXXX  Set register N to value XXXXXX; N=0..7,ps")
    ConsoleView_Print("  s          Step Into; executes one instruction");
    ConsoleView_Print("  so         Step Over; executes and stops after the current instruction");
    ConsoleView_Print("  b          List breakpoints set for the current processor");
    ConsoleView_Print("  bXXXXXX    Set breakpoint at address XXXXXX");
    ConsoleView_Print("  bcXXXXXX   Remove breakpoint at address XXXXXX");
    ConsoleView_Print("  bc         Remove all breakpoints for the current processor");
    //ConsoleView_Print("  u          Save memory dump to file memdumpXPU.bin")
    //ConsoleView_Print("  udl        Save display list dump to file displaylist.txt")
    //ConsoleView_Print("  fcXXXXXX YYYYYY  Calculate floating number for the two octal words")
    //ConsoleView_Print("  t          Tracing on/off to trace.log file")
    //ConsoleView_Print("  tXXXXXX    Set tracing flags")
    //ConsoleView_Print("  tc         Clear trace.log file")
}

void ConsoleView_CmdClearConsoleLog(const ConsoleCommandParams& /*params*/)
{
    ConsoleView_ClearConsole();
}

void ConsoleView_CmdSwitchCpuPpu(const ConsoleCommandParams& /*params*/)
{
    g_okDebugCpuPpu = !g_okDebugCpuPpu;

    LPCTSTR procName = ConsoleView_GetCurrentProcessor()->GetName();
    ConsoleView_PrintFormat("  Switched to %S\r\n", procName);
}

void ConsoleView_CmdStepInto(const ConsoleCommandParams& /*params*/)
{
    CProcessor* pProc = ConsoleView_GetCurrentProcessor();

    ConsoleView_PrintDisassemble(pProc, pProc->GetPC(), TRUE, FALSE);

    g_pBoard->DebugTicks();
}
void ConsoleView_CmdStepOver(const ConsoleCommandParams& /*params*/)
{
    CProcessor* pProc = ConsoleView_GetCurrentProcessor();
    CMemoryController* pMemCtl = pProc->GetMemoryController();

    int instrLength = ConsoleView_PrintDisassemble(pProc, pProc->GetPC(), TRUE, FALSE);

    int addrtype;
    uint16_t instr = pMemCtl->GetWordView(pProc->GetPC(), pProc->IsHaltMode(), true, &addrtype);

    // For JMP and BR use Step Into logic, not Step Over
    if ((instr & ~(uint16_t)077) == PI_JMP || (instr & ~(uint16_t)0377) == PI_BR)
    {
        g_pBoard->DebugTicks();

        return;
    }

    uint16_t bpaddress = (uint16_t)(pProc->GetPC() + instrLength * 2);

    if (g_okDebugCpuPpu)
        Emulator_SetTempCPUBreakpoint(bpaddress);
    else
        Emulator_SetTempPPUBreakpoint(bpaddress);
    Emulator_Start();
}
void ConsoleView_CmdRun(const ConsoleCommandParams& /*params*/)
{
    Emulator_Start();
}
void ConsoleView_CmdRunToAddress(const ConsoleCommandParams& params)
{
    uint16_t address = params.paramOct1;

    if (g_okDebugCpuPpu)
        Emulator_SetTempCPUBreakpoint(address);
    else
        Emulator_SetTempPPUBreakpoint(address);
    Emulator_Start();
}


void ConsoleView_CmdPrintAllBreakpoints(const ConsoleCommandParams& /*params*/)
{
    const uint16_t* pbps = g_okDebugCpuPpu ? Emulator_GetCPUBreakpointList() : Emulator_GetPPUBreakpointList();
    if (pbps == nullptr || *pbps == 0177777)
    {
        ConsoleView_Print("  No breakpoints.");
    }
    else
    {
        while (*pbps != 0177777)
        {
            ConsoleView_PrintFormat("  %06ho", *pbps);
            pbps++;
        }
    }
}
void ConsoleView_CmdRemoveAllBreakpoints(const ConsoleCommandParams& /*params*/)
{
    Emulator_RemoveAllBreakpoints(g_okDebugCpuPpu);
}
void ConsoleView_CmdSetBreakpointAtAddress(const ConsoleCommandParams& params)
{
    uint16_t address = params.paramOct1;

    bool result = g_okDebugCpuPpu ? Emulator_AddCPUBreakpoint(address) : Emulator_AddPPUBreakpoint(address);
    if (!result)
        ConsoleView_Print("  Failed to add breakpoint.");
}
void ConsoleView_CmdRemoveBreakpointAtAddress(const ConsoleCommandParams& params)
{
    uint16_t address = params.paramOct1;

    bool result = g_okDebugCpuPpu ? Emulator_RemoveCPUBreakpoint(address) : Emulator_RemovePPUBreakpoint(address);
    if (!result)
        ConsoleView_Print("  Failed to remove breakpoint.");
}


//////////////////////////////////////////////////////////////////////

enum ConsoleCommandArgInfo
{
    ARGINFO_NONE,     // No parameters
    ARGINFO_REG,      // Register number 0..7
    ARGINFO_OCT,      // Octal value
    ARGINFO_REG_OCT,  // Register number, octal value
    ARGINFO_OCT_OCT,  // Octal value, octal value
};

typedef void(*CONSOLE_COMMAND_CALLBACK)(const ConsoleCommandParams& params);

struct ConsoleCommandStruct
{
    const char* pattern;
    ConsoleCommandArgInfo arginfo;
    CONSOLE_COMMAND_CALLBACK callback;
}
static ConsoleCommands[] =
{
    // IMPORTANT! First list more complex forms with more arguments, then less complex forms
    { "h", ARGINFO_NONE, ConsoleView_CmdShowHelp },
    { "c", ARGINFO_NONE, ConsoleView_CmdClearConsoleLog },
    { "p", ARGINFO_NONE, ConsoleView_CmdSwitchCpuPpu },
    //{ "r%d=%ho", ARGINFO_REG_OCT, ConsoleView_CmdSetRegisterValue },
    //{ "r%d %ho", ARGINFO_REG_OCT, ConsoleView_CmdSetRegisterValue },
    //{ "r%d", ARGINFO_REG, ConsoleView_CmdPrintRegister },
    //{ "r", ARGINFO_NONE, ConsoleView_CmdPrintAllRegisters },
    //{ "rps=%ho", ARGINFO_OCT, ConsoleView_CmdSetRegisterPSW },
    //{ "rps %ho", ARGINFO_OCT, ConsoleView_CmdSetRegisterPSW },
    //{ "rps", ARGINFO_NONE, ConsoleView_CmdPrintRegisterPSW },
    { "s", ARGINFO_NONE, ConsoleView_CmdStepInto },
    { "so", ARGINFO_NONE, ConsoleView_CmdStepOver },
    //{ "d%ho", ARGINFO_OCT, ConsoleView_CmdPrintDisassembleAtAddress },
    //{ "D%ho", ARGINFO_OCT, ConsoleView_CmdPrintDisassembleAtAddress },
    //{ "d", ARGINFO_NONE, ConsoleView_CmdPrintDisassembleAtPC },
    //{ "D", ARGINFO_NONE, ConsoleView_CmdPrintDisassembleAtPC },
    //{ "u", ARGINFO_NONE, ConsoleView_CmdSaveMemoryDump },
    //{ "udl", ARGINFO_NONE, ConsoleView_CmdSaveDisplayListDump },
    //{ "m%ho %ho", ARGINFO_OCT_OCT, ConsoleView_CmdSetMemoryAtAddress },
    //{ "m%ho=%ho", ARGINFO_OCT_OCT, ConsoleView_CmdSetMemoryAtAddress },
    //{ "m%ho", ARGINFO_OCT, ConsoleView_CmdPrintMemoryDumpAtAddress },
    //{ "mr%d", ARGINFO_REG, ConsoleView_CmdPrintMemoryDumpAtRegister },
    //{ "m", ARGINFO_NONE, ConsoleView_CmdPrintMemoryDumpAtPC },
    { "g%ho", ARGINFO_OCT, ConsoleView_CmdRunToAddress },
    { "g", ARGINFO_NONE, ConsoleView_CmdRun },
    { "b%ho", ARGINFO_OCT, ConsoleView_CmdSetBreakpointAtAddress },
    { "b", ARGINFO_NONE, ConsoleView_CmdPrintAllBreakpoints },
    { "bc%ho", ARGINFO_OCT, ConsoleView_CmdRemoveBreakpointAtAddress },
    { "bc", ARGINFO_NONE, ConsoleView_CmdRemoveAllBreakpoints },
    //{ "fc%ho %ho", ARGINFO_OCT_OCT, ConsoleView_CmdCalculateFloatNumber },
    //{ "t%ho", ARGINFO_OCT, ConsoleView_CmdTraceLogWithMask },
    //{ "t", ARGINFO_NONE, ConsoleView_CmdTraceLogOnOff },
    //{ "tc", ARGINFO_NONE, ConsoleView_CmdClearTraceLog },
};
const size_t ConsoleCommandsCount = sizeof(ConsoleCommands) / sizeof(ConsoleCommands[0]);

void ConsoleView_DoConsoleCommand()
{
    char command[64];
    strcpy(command, m_ConsoleInputBuf);
    *m_ConsoleInputBuf = 0;  // Clear command

    if (command[0] == 0) return;  // Nothing to do

    auto strPromtAndCommand = ConsoleView_GetConsolePrompt();
    strPromtAndCommand.append(" ");
    strPromtAndCommand.append(command);

    // Echo command to the log
    ConsoleView_Print(strPromtAndCommand.c_str());

    ConsoleCommandParams params;
    params.commandText = command;
    params.paramReg1 = -1;
    params.paramOct1 = 0;
    params.paramOct2 = 0;

    // Find matching console command from the list, parse and execute the command
    bool parsedOkay = false, parseError = false;
    for (size_t i = 0; i < ConsoleCommandsCount; i++)
    {
        ConsoleCommandStruct& cmd = ConsoleCommands[i];

        int paramsParsed = 0;
        switch (cmd.arginfo)
        {
        case ARGINFO_NONE:
            parsedOkay = (strcmp(command, cmd.pattern) == 0);
            break;
        case ARGINFO_REG:
            paramsParsed = _snscanf_s(command, IM_ARRAYSIZE(command), cmd.pattern, &params.paramReg1);
            parsedOkay = (paramsParsed == 1);
            if (parsedOkay && (params.paramReg1 < 0 || params.paramReg1 > 7))
            {
                ConsoleView_Print(MESSAGE_INVALID_REGNUM);
                parseError = true;
            }
            break;
        case ARGINFO_OCT:
            paramsParsed = _snscanf_s(command, IM_ARRAYSIZE(command), cmd.pattern, &params.paramOct1);
            parsedOkay = (paramsParsed == 1);
            break;
        case ARGINFO_REG_OCT:
            paramsParsed = _snscanf_s(command, IM_ARRAYSIZE(command), cmd.pattern, &params.paramReg1, &params.paramOct1);
            parsedOkay = (paramsParsed == 2);
            if (parsedOkay && (params.paramReg1 < 0 || params.paramReg1 > 7))
            {
                ConsoleView_Print(MESSAGE_INVALID_REGNUM);
                parseError = true;
            }
            break;
        case ARGINFO_OCT_OCT:
            paramsParsed = _snscanf_s(command, IM_ARRAYSIZE(command), cmd.pattern, &params.paramOct1, &params.paramOct2);
            parsedOkay = (paramsParsed == 2);
            break;
        }

        if (parseError)
            break;  // Validation detected error and printed the message already

        if (parsedOkay)
        {
            cmd.callback(params);  // Execute the command
            break;
        }
    }

    if (!parsedOkay && !parseError)
        ConsoleView_Print(MESSAGE_UNKNOWN_COMMAND);

    //TODO: Return focus to prompt
}


//////////////////////////////////////////////////////////////////////
