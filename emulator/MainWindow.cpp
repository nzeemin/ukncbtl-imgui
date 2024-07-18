
#include "stdafx.h"
#include "Main.h"
#include "Emulator.h"
#include "emubase/Emubase.h"
#include <cmath>


//////////////////////////////////////////////////////////////////////


#ifdef _DEBUG
bool show_demo_window = false;
#endif
bool open_settings_popup = false;
bool open_about_popup = false;


//////////////////////////////////////////////////////////////////////


void ImGuiSettingsPopup();
void ImGuiAboutPopup();

void MainWindow_DoEmulatorSpeed(WORD speed);
void MainWindow_DoScreenViewMode(ScreenViewMode mode);
void MainWindow_DoFloppyImageSelect(int slot);
void MainWindow_DoFloppyImageEject(int slot);
void MainWindow_DoCartridgeSelect(int slot);
void MainWindow_DoCartridgeEject(int slot);
void MainWindow_DoHardImageSelect(int slot);
void MainWindow_DoHardImageEject(int slot);


//////////////////////////////////////////////////////////////////////


// Re-attach disk images, cartridge images, HDD images
void MainWindow_RestoreImages()
{
    TCHAR buf[MAX_PATH];

    // Reattach floppy images
    for (int slot = 0; slot < 4; slot++)
    {
        buf[0] = _T('\0');
        Settings_GetFloppyFilePath(slot, buf);
        if (buf[0] != _T('\0'))
        {
            if (!g_pBoard->AttachFloppyImage(slot, buf))
                Settings_SetFloppyFilePath(slot, NULL);
        }
    }

    // Reattach cartridge and HDD images
    for (int slot = 1; slot <= 2; slot++)
    {
        buf[0] = _T('\0');
        Settings_GetCartridgeFilePath(slot, buf);
        if (buf[0] != _T('\0'))
        {
            if (!Emulator_LoadROMCartridge(slot, buf))
            {
                Settings_SetCartridgeFilePath(slot, NULL);
                Settings_SetHardFilePath(slot, NULL);
            }

            Settings_GetHardFilePath(slot, buf);
            if (buf[0] != _T('\0'))
            {
                if (!g_pBoard->AttachHardImage(slot, buf))
                {
                    Settings_SetHardFilePath(slot, NULL);
                }
            }
        }
    }
}

void MainWindow_SetColorSheme(int scheme)
{
    switch (scheme)
    {
    default: ImGui::StyleColorsDark(); break;
    case 1:  ImGui::StyleColorsLight(); break;
    case 2:  ImGui::StyleColorsClassic(); break;
    }
}

void ImGuiMainMenu()
{
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Settings"))
                open_settings_popup = true;

            //ImGui::MenuItem("Screenshot");
            //ImGui::Separator();
            //ImGui::MenuItem("Quit");//TODO
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View"))
        {
            int viewmode = Settings_GetScreenViewMode();
            bool checkedRGB = viewmode == 0;
            ImGui::BeginDisabled(checkedRGB);
            if (ImGui::MenuItem("RGB Screen", nullptr, &checkedRGB)) MainWindow_DoScreenViewMode(RGBScreen);
            ImGui::EndDisabled();
            bool checkedGRB = viewmode == 1;
            ImGui::BeginDisabled(checkedGRB);
            if (ImGui::MenuItem("GRB Screen", nullptr, &checkedGRB)) MainWindow_DoScreenViewMode(GRBScreen);
            ImGui::EndDisabled();
            bool checkedGray = viewmode == 2;
            ImGui::BeginDisabled(checkedGray);
            if (ImGui::MenuItem("Grayscale Screen", nullptr, &checkedGray)) MainWindow_DoScreenViewMode(GrayScreen);
            ImGui::EndDisabled();

            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Emulator"))
        {
            if (!g_okEmulatorRunning)
            {
                if (ImGui::MenuItem(ICON_FA_PLAY " Run"))
                    Emulator_Start();
            }
            else
            {
                if (ImGui::MenuItem(ICON_FA_PAUSE " Stop"))
                    Emulator_Stop();
            }
            if (ImGui::MenuItem(ICON_FA_REDO " Reset"))
                Emulator_Reset();

            bool autostart = Settings_GetAutostart();
            if (ImGui::MenuItem("Autostart", nullptr, &autostart))
            {
                Settings_SetAutostart(!Settings_GetAutostart());
            }

            ImGui::Separator();

            WORD speed = Settings_GetRealSpeed();
            bool checked25 = speed == 0x7ffe;
            ImGui::BeginDisabled(checked25);
            if (ImGui::MenuItem("Speed 25%", nullptr, &checked25)) MainWindow_DoEmulatorSpeed(0x7ffe);
            ImGui::EndDisabled();
            bool checked50 = speed == 0x7fff;
            ImGui::BeginDisabled(checked50);
            if (ImGui::MenuItem("Speed 50%", nullptr, &checked50)) MainWindow_DoEmulatorSpeed(0x7fff);
            ImGui::EndDisabled();
            bool checked100 = speed == 1;
            ImGui::BeginDisabled(checked100);
            if (ImGui::MenuItem("Speed 100%", nullptr, &checked100)) MainWindow_DoEmulatorSpeed(1);
            ImGui::EndDisabled();
            bool checked200 = speed == 200;
            ImGui::BeginDisabled(checked200);
            if (ImGui::MenuItem("Speed 200%", nullptr, &checked200)) MainWindow_DoEmulatorSpeed(2);
            ImGui::EndDisabled();
            bool checkedMax = speed == 0;
            ImGui::BeginDisabled(checkedMax);
            if (ImGui::MenuItem("Speed MAX", nullptr, &checkedMax)) MainWindow_DoEmulatorSpeed(0);
            ImGui::EndDisabled();

            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Debug"))
        {
            ImGui::BeginDisabled(g_okEmulatorRunning);
            if (ImGui::MenuItem("Step"))
                ConsoleView_StepInto();
            if (ImGui::MenuItem("Step Over"))
                ConsoleView_StepOver();
            ImGui::EndDisabled();
            ImGui::Separator();

            if (ImGui::MenuItem("Remove All Breakpoints"))
                ConsoleView_DeleteAllBreakpoints();

            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help"))
        {
            if (ImGui::MenuItem("About"))
                open_about_popup = true;

            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }

    if (open_settings_popup)
    {
        open_settings_popup = false;
        ImGui::OpenPopup("settings_popup");
    }
    ImGuiSettingsPopup();

    if (open_about_popup)
    {
        open_about_popup = false;
        ImGui::OpenPopup("about_popup");
    }
    ImGuiAboutPopup();
}

void ImGuiSettingsPopup()
{
    if (ImGui::BeginPopupModal("settings_popup", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        //
        static int style_idx = Settings_GetColorScheme();
        const char* color_scheme_list = "ImGui Dark\0ImGui Light\0ImGui Classic\0";
        if (ImGui::Combo("Color Scheme", &style_idx, color_scheme_list))
        {
            MainWindow_SetColorSheme(style_idx);
            Settings_SetColorScheme(style_idx);
        }

        if (ImGui::Button("Close", ImVec2(200.0f, 0.0f)))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
}

void ImGuiAboutPopup()
{
    if (ImGui::BeginPopupModal("about_popup", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("UKNCBTL ImGui version");
        ImGui::TextLinkOpenURL("Source code", "https://github.com/nzeemin/ukncbtl-imgui");
        ImGui::Spacing();

        ImGui::Separator();
        ImGui::Text("Dear ImGui version %s %d", IMGUI_VERSION, IMGUI_VERSION_NUM);
        ImGui::Spacing();

        if (ImGui::Button("Close", ImVec2(200.0f, 0.0f)))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
}

void ControlView_ImGuiWidget()
{
#ifdef _DEBUG
    // Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
    if (show_demo_window)
        ImGui::ShowDemoWindow(&show_demo_window);//TODO
#endif
    ImGui::Begin("Control");

    if (!g_okEmulatorRunning)
    {
        if (ImGui::Button(ICON_FA_PLAY " Run     "))
            Emulator_Start();
    }
    else
    {
        if (ImGui::Button(ICON_FA_PAUSE " Stop    "))
            Emulator_Stop();
    }
    if (ImGui::Button(ICON_FA_REDO " Reset   "))
        Emulator_Reset();

    float uptimef = Emulator_GetUptime();
    uint32_t uptime = (uint32_t)std::floorf(uptimef);
    int minutes = (int)(uptime / 60 % 60);
    int hours = (int)(uptime / 3600 % 60);
    float secondsf = uptimef - (uptime / 60 * 60);
    ImGui::TextDisabled(ICON_FA_CLOCK); ImGui::SameLine();
    if (hours > 0)
    {
        ImGui::Text("%dh", hours);
        ImGui::SameLine(0.0f, 0.0f);
    }
    ImGui::Text("%02d:%05.2f", minutes, secondsf);

    ImGui::TextDisabled(ICON_FA_RUNNING "  ");
    if (g_okEmulatorRunning && g_dFramesPercent != 0.0)
    {
        ImGui::SameLine();
        ImGui::Text("%03.f%%", g_dFramesPercent);
    }

    WORD speed = Settings_GetRealSpeed();
    ImGui::BeginDisabled(speed == 0x7ffe);
    if (ImGui::Button("25")) MainWindow_DoEmulatorSpeed(0x7ffe);
    ImGui::EndDisabled();
    ImGui::SameLine(0.0f, 0.0f);
    ImGui::BeginDisabled(speed == 0x7fff);
    if (ImGui::Button("50")) MainWindow_DoEmulatorSpeed(0x7fff);
    ImGui::EndDisabled();
    ImGui::SameLine(0.0f, 0.0f);
    ImGui::BeginDisabled(speed == 1);
    if (ImGui::Button("100")) MainWindow_DoEmulatorSpeed(1);
    ImGui::EndDisabled();
    ImGui::SameLine(0.0f, 0.0f);
    ImGui::BeginDisabled(speed == 2);
    if (ImGui::Button("200")) MainWindow_DoEmulatorSpeed(2);
    ImGui::EndDisabled();
    ImGui::SameLine(0.0f, 0.0f);
    ImGui::BeginDisabled(speed == 0);
    if (ImGui::Button("Max")) MainWindow_DoEmulatorSpeed(0);
    ImGui::EndDisabled();

    ImGui::SeparatorText("Video");
    int viewmode = Settings_GetScreenViewMode();
    ImGui::BeginDisabled(viewmode == 0);
    if (ImGui::Button("RGB")) MainWindow_DoScreenViewMode(RGBScreen);
    ImGui::EndDisabled();
    ImGui::SameLine(0.0f, 0.0f);
    ImGui::BeginDisabled(viewmode == 1);
    if (ImGui::Button("GRB")) MainWindow_DoScreenViewMode(GRBScreen);
    ImGui::EndDisabled();
    ImGui::SameLine(0.0f, 0.0f);
    ImGui::BeginDisabled(viewmode == 2);
    if (ImGui::Button("Gray")) MainWindow_DoScreenViewMode(GrayScreen);
    ImGui::EndDisabled();

    //ImGui::TextDisabled("X:   ");
    //ImGui::SameLine();
    //ImGui::TextDisabled("Y:   ");

    ImGui::SeparatorText("Floppies");
    for (int floppyslot = 0; floppyslot < 4; floppyslot++)
    {
        char bufname[32];
        ImGui::TextDisabled("%d:", floppyslot); ImGui::SameLine();
        if (g_pBoard->IsFloppyImageAttached(floppyslot))
        {
            sprintf(bufname, ICON_FA_EJECT "###FloppyEject%d", floppyslot);
            if (ImGui::SmallButton(bufname))
                MainWindow_DoFloppyImageEject(floppyslot);
            ImGui::SameLine();

            TCHAR buffilepath[MAX_PATH];
            Settings_GetFloppyFilePath(floppyslot, buffilepath);
            LPCTSTR lpFileName = GetFileNameFromFilePath(buffilepath);
            ImGui::Text("%S", lpFileName);
        }
        else
        {
            sprintf(bufname, "   " ICON_FA_ELLIPSIS_H "   ###FloppySelect%d", floppyslot);
            if (ImGui::SmallButton(bufname))
                MainWindow_DoFloppyImageSelect(floppyslot);
        }
    }
    ImGui::TextDisabled("Motor:"); ImGui::SameLine();
    if (g_pBoard->IsFloppyEngineOn())
        ImGui::Text("ON");
    else
        ImGui::TextDisabled("OFF");

    ImGui::SeparatorText("Cartridges");
    for (int cartslot = 1; cartslot <= 2; cartslot++)
    {
        char bufname[32];
        ImGui::TextDisabled("%d:", cartslot); ImGui::SameLine();
        if (g_pBoard->IsROMCartridgeLoaded(cartslot))
        {
            sprintf(bufname, ICON_FA_EJECT "###CartEject%d", cartslot);
            if (ImGui::SmallButton(bufname))
                MainWindow_DoCartridgeEject(cartslot);
            ImGui::SameLine();

            TCHAR buffilepath[MAX_PATH];
            Settings_GetCartridgeFilePath(cartslot, buffilepath);
            LPCTSTR lpFileName = GetFileNameFromFilePath(buffilepath);
            ImGui::Text("%S", lpFileName);
        }
        else
        {
            sprintf(bufname, "   " ICON_FA_ELLIPSIS_H "   ###CartSelect%d", cartslot);
            if (ImGui::SmallButton(bufname))
                MainWindow_DoCartridgeSelect(cartslot);
        }
        ImGui::TextDisabled("  " ICON_FA_HDD ":");
        if (g_pBoard->IsROMCartridgeLoaded(cartslot))
        {
            ImGui::SameLine();
            if (g_pBoard->IsHardImageAttached(cartslot))
            {
                sprintf(bufname, ICON_FA_EJECT "###HardEject%d", cartslot);
                if (ImGui::SmallButton(bufname))
                    MainWindow_DoHardImageEject(cartslot);
                ImGui::SameLine();

                TCHAR buffilepath[MAX_PATH];
                Settings_GetHardFilePath(cartslot, buffilepath);
                LPCTSTR lpFileName = GetFileNameFromFilePath(buffilepath);
                ImGui::Text("%S", lpFileName);
            }
            else
            {
                sprintf(bufname, "   " ICON_FA_ELLIPSIS_H "   ###HardSelect%d", cartslot);
                if (ImGui::SmallButton(bufname))
                    MainWindow_DoHardImageSelect(cartslot);
            }
        }
    }

    //ImGui::SeparatorText("");
    ImGui::BeginDisabled();//TODO
    //ImGui::Button("Screenshot");
    //ImGui::SeparatorText("");
    //bool sound = Settings_GetSound();
    //ImGui::Checkbox(ICON_FA_VOLUME_MUTE " Sound", &sound);
    //if (Emulator_IsSound())
    //{
    //    ImGui::SameLine();
    //    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "))");
    //}
    //int volume = Settings_GetSoundVolume() / 655;
    //ImGui::SliderInt("Volume", &volume, 0, 100);
    ImGui::EndDisabled();

    ImGui::SeparatorText("ImGui");
    ImGui::TextDisabled("FPS: %.1f", ImGui::GetIO().Framerate);
#ifdef _DEBUG
    ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
#endif
    ImGui::End();
}

void MainWindow_DoEmulatorSpeed(WORD speed)
{
    Settings_SetRealSpeed(speed);
    Emulator_SetSpeed(speed);
}

void MainWindow_DoScreenViewMode(ScreenViewMode mode)
{
    Settings_SetScreenViewMode(mode);
}

void MainWindow_DoFloppyImageSelect(int slot)
{
    // File Open dialog
    TCHAR bufFileName[MAX_PATH];
    BOOL okResult = ShowOpenDialog(g_hwnd,
        _T("Open floppy image to load"),
        _T("UKNC floppy images (*.dsk, *.rtd)\0*.dsk;*.rtd\0All Files (*.*)\0*.*\0\0"),
        bufFileName);
    if (!okResult) return;

    if (!g_pBoard->AttachFloppyImage(slot, bufFileName))
    {
        AlertWarning(_T("Failed to attach floppy image."));
        return;
    }
    Settings_SetFloppyFilePath(slot, bufFileName);
}

void MainWindow_DoFloppyImageEject(int slot)
{
    g_pBoard->DetachFloppyImage(slot);
    Settings_SetFloppyFilePath(slot, NULL);
}

void MainWindow_DoCartridgeSelect(int slot)
{
    // File Open dialog
    TCHAR bufFileName[MAX_PATH];
    BOOL okResult = ShowOpenDialog(g_hwnd,
        _T("Open ROM cartridge image to load"),
        _T("UKNC ROM cartridge images (*.bin)\0*.bin\0All Files (*.*)\0*.*\0\0"),
        bufFileName);
    if (!okResult) return;

    if (!Emulator_LoadROMCartridge(slot, bufFileName))
    {
        AlertWarning(_T("Failed to attach the ROM cartridge image."));
        return;
    }

    Settings_SetCartridgeFilePath(slot, bufFileName);
}

void MainWindow_DoCartridgeEject(int slot)
{
    g_pBoard->UnloadROMCartridge(slot);
    Settings_SetCartridgeFilePath(slot, NULL);
}

void MainWindow_DoHardImageSelect(int slot)
{
    // Check if cartridge (HDD ROM image) already selected
    if (!g_pBoard->IsROMCartridgeLoaded(slot))
    {
        AlertWarning(_T("Please select HDD ROM image as cartridge first."));
        return;
    }

    // Select HDD disk image
    TCHAR bufFileName[MAX_PATH];
    BOOL okResult = ShowOpenDialog(g_hwnd,
        _T("Open HDD image"),
        _T("UKNC HDD images (*.img)\0*.img\0All Files (*.*)\0*.*\0\0"),
        bufFileName);
    if (!okResult) return;

    // Attach HDD disk image
    if (!g_pBoard->AttachHardImage(slot, bufFileName))
    {
        AlertWarning(_T("Failed to attach the HDD image."));
        return;
    }

    Settings_SetHardFilePath(slot, bufFileName);
}

void MainWindow_DoHardImageEject(int slot)
{
    g_pBoard->DetachHardImage(slot);
    Settings_SetHardFilePath(slot, NULL);
}


//////////////////////////////////////////////////////////////////////
