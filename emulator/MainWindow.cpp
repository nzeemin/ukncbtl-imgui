
#include "stdafx.h"
#include "imgui/imgui_internal.h"
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

// Derived colors based and calculated using the current style
ImVec4 g_colorPlay;              // Play sign in Disasm window
ImVec4 g_colorBreak;             // Breakpoint
ImVec4 g_colorJumpLine;          // Jump arrow
ImVec4 g_colorDisabledRed;       // Disabled color shifted to red
ImVec4 g_colorDisabledGreen;     // Disabled color shifted to green
ImVec4 g_colorDisabledBlue;      // Disabled color shifted to blue


//////////////////////////////////////////////////////////////////////


void ImGuiSettingsPopup();
void ImGuiAboutPopup();

void MainWindow_DoEmulatorSpeed(WORD speed);
void MainWindow_DoScreenViewMode(ScreenViewMode mode);
void MainWindow_DoScreenSizeMode(ScreenSizeMode mode);
void MainWindow_DoFloppyImageSelect(int slot);
void MainWindow_DoFloppyImageEject(int slot);
void MainWindow_DoCartridgeSelect(int slot);
void MainWindow_DoCartridgeEject(int slot);
void MainWindow_DoHardImageSelect(int slot);
void MainWindow_DoHardImageEject(int slot);
void MainWindow_DoEmulatorSound(bool flag);
void MainWindow_EmulatorSoundVolume(uint16_t volume);
void MainWindow_DoFileScreenshot();


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

    // Calculate derived colors
    g_colorPlay = ImLerp(ImGui::GetStyleColorVec4(ImGuiCol_Text), ImVec4(0.0f, 0.67f, 0.0f, 1.0f), 0.67f);
    g_colorBreak = ImLerp(ImGui::GetStyleColorVec4(ImGuiCol_Text), ImVec4(1.0f, 0.0f, 0.0f, 1.0f), 0.67f);
    g_colorJumpLine = ImLerp(ImGui::GetStyleColorVec4(ImGuiCol_Text), ImVec4(0.5f, 0.7f, 1.0f, 0.67f), 0.67f);
    g_colorDisabledRed = ImLerp(ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled), ImVec4(1.0f, 0.0f, 0.0f, 1.0f), 0.33f);
    g_colorDisabledGreen = ImLerp(ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled), ImVec4(0.0f, 1.0f, 0.0f, 1.0f), 0.33f);
    g_colorDisabledBlue = ImLerp(ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled), ImVec4(0.0f, 0.0f, 1.0f, 1.0f), 0.33f);
}

void ImGuiMainMenu()
{
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem(ICON_FA_CAMERA " Screenshot"))
                MainWindow_DoFileScreenshot();

            ImGui::Separator();
            if (ImGui::MenuItem(ICON_FA_COG " Settings"))
                open_settings_popup = true;

            //ImGui::MenuItem("Quit");//TODO
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

            bool sound = Settings_GetSound();
            if (ImGui::MenuItem("Sound", nullptr, &sound))
                MainWindow_DoEmulatorSound(sound);

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
            bool checked120 = speed == 2;
            ImGui::BeginDisabled(checked120);
            if (ImGui::MenuItem("Speed 200%", nullptr, &checked120)) MainWindow_DoEmulatorSpeed(2);
            ImGui::EndDisabled();
            bool checked240 = speed == 3;
            ImGui::BeginDisabled(checked240);
            if (ImGui::MenuItem("Speed 200%", nullptr, &checked240)) MainWindow_DoEmulatorSpeed(3);
            ImGui::EndDisabled();
            bool checkedMax = speed == 0;
            ImGui::BeginDisabled(checkedMax);
            if (ImGui::MenuItem("Speed MAX", nullptr, &checkedMax)) MainWindow_DoEmulatorSpeed(0);
            ImGui::EndDisabled();

            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Configuration"))
        {
            for (int floppyslot = 0; floppyslot < 4; floppyslot++)
            {
                char bufname[64];
                if (g_pBoard->IsFloppyImageAttached(floppyslot))
                {
                    sprintf(bufname, "FDD %d: " ICON_FA_EJECT " Eject###MenuFloppyEject%d", floppyslot, floppyslot);
                    if (ImGui::MenuItem(bufname))
                        MainWindow_DoFloppyImageEject(floppyslot);
                    ImGui::SameLine();

                    TCHAR buffilepath[MAX_PATH];
                    Settings_GetFloppyFilePath(floppyslot, buffilepath);
                    LPCTSTR lpFileName = GetFileNameFromFilePath(buffilepath);
                    ImGui::Text("%S", lpFileName);
                }
                else
                {
                    sprintf(bufname, "FDD %d: Select...###MenuFloppySelect%d", floppyslot, floppyslot);
                    if (ImGui::MenuItem(bufname))
                        MainWindow_DoFloppyImageSelect(floppyslot);
                }
            }

            ImGui::Separator();

            for (int cartslot = 1; cartslot <= 2; cartslot++)
            {
                char bufname[64];
                if (g_pBoard->IsROMCartridgeLoaded(cartslot))
                {
                    sprintf(bufname, "Cart %d: " ICON_FA_EJECT " Eject###MenuCartEject%d", cartslot, cartslot);
                    if (ImGui::MenuItem(bufname))
                        MainWindow_DoCartridgeEject(cartslot);
                    ImGui::SameLine();

                    TCHAR buffilepath[MAX_PATH];
                    Settings_GetCartridgeFilePath(cartslot, buffilepath);
                    LPCTSTR lpFileName = GetFileNameFromFilePath(buffilepath);
                    ImGui::Text("%S", lpFileName);
                }
                else
                {
                    sprintf(bufname, "Cart %d: Select...###MenuCartSelect%d", cartslot, cartslot);
                    if (ImGui::MenuItem(bufname))
                        MainWindow_DoCartridgeSelect(cartslot);
                }
                if (g_pBoard->IsROMCartridgeLoaded(cartslot))
                {
                    if (g_pBoard->IsHardImageAttached(cartslot))
                    {
                        sprintf(bufname, "  " ICON_FA_HDD " HDD: " ICON_FA_EJECT " Eject###MenuHardEject%d", cartslot);
                        if (ImGui::MenuItem(bufname))
                            MainWindow_DoHardImageEject(cartslot);
                        ImGui::SameLine();

                        TCHAR buffilepath[MAX_PATH];
                        Settings_GetHardFilePath(cartslot, buffilepath);
                        LPCTSTR lpFileName = GetFileNameFromFilePath(buffilepath);
                        ImGui::Text("%S", lpFileName);
                    }
                    else
                    {
                        sprintf(bufname, "  " ICON_FA_HDD " HDD: Select...   ###MenuHardSelect%d", cartslot);
                        if (ImGui::MenuItem(bufname))
                            MainWindow_DoHardImageSelect(cartslot);
                    }
                }
                else
                {
                    ImGui::TextDisabled("  " ICON_FA_HDD " HDD");
                }
            }

            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Video"))
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

            ImGui::Separator();
            int sizemode = Settings_GetScreenSizeMode();
            bool checked0 = sizemode == 0;
            ImGui::BeginDisabled(checked0);
            if (ImGui::MenuItem("Fill the box", nullptr, &checked0)) MainWindow_DoScreenSizeMode(ScreenSizeFill);
            ImGui::EndDisabled();
            bool checked1 = sizemode == 1;
            ImGui::BeginDisabled(checked1);
            if (ImGui::MenuItem("Maintain 4:3 ratio", nullptr, &checked1)) MainWindow_DoScreenSizeMode(ScreenSize4to3ratio);
            ImGui::EndDisabled();
            bool checked2 = sizemode == 2;
            ImGui::BeginDisabled(checked2);
            if (ImGui::MenuItem("640 x 480", nullptr, &checked2)) MainWindow_DoScreenSizeMode(ScreenSize640x480);
            ImGui::EndDisabled();
            bool checked3 = sizemode == 3;
            ImGui::BeginDisabled(checked3);
            if (ImGui::MenuItem("800 x 600", nullptr, &checked3)) MainWindow_DoScreenSizeMode(ScreenSize800x600);
            ImGui::EndDisabled();
            bool checked4 = sizemode == 4;
            ImGui::BeginDisabled(checked4);
            if (ImGui::MenuItem("960 x 720", nullptr, &checked4)) MainWindow_DoScreenSizeMode(ScreenSize960x720);
            ImGui::EndDisabled();
            bool checked5 = sizemode == 5;
            ImGui::BeginDisabled(checked5);
            if (ImGui::MenuItem("1280 x 960", nullptr, &checked5)) MainWindow_DoScreenSizeMode(ScreenSize1280x960);
            ImGui::EndDisabled();
            bool checked6 = sizemode == 6;
            ImGui::BeginDisabled(checked6);
            if (ImGui::MenuItem("1600 x 1200", nullptr, &checked6)) MainWindow_DoScreenSizeMode(ScreenSize1600x1200);
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
        ImGui::SeparatorText("Colors");

        static int style_idx = Settings_GetColorScheme();
        const char* color_scheme_list = "ImGui Dark\0ImGui Light\0ImGui Classic\0";
        if (ImGui::Combo("Color Scheme", &style_idx, color_scheme_list))
        {
            MainWindow_SetColorSheme(style_idx);
            Settings_SetColorScheme(style_idx);
        }

        ImGui::SeparatorText("Screenshots");

        int selectedmode = Settings_GetScreenshotMode();
        const char* selmodename = ScreenView_GetScreenshotModeName(selectedmode);
        if (ImGui::BeginCombo("Screenshot Mode", selmodename))
        {
            int mode = 0;
            for (;;)
            {
                const char* modename = ScreenView_GetScreenshotModeName(mode);
                if (modename == nullptr)
                    break;

                bool selected = mode == selectedmode;
                if (ImGui::Selectable(modename, &selected))
                    Settings_SetScreenshotMode(mode);

                mode++;
            }
            ImGui::EndCombo();
        }

        ImGui::Spacing();
        if (ImGui::Button("Close", ImVec2(200.0f, 0.0f)))
            ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
    }
}

void ImGuiAboutPopup()
{
    if (ImGui::BeginPopupModal("about_popup", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::SeparatorText("UKNCBTL ImGui");
        ImGui::Text("UKNCBTL ImGui version %s revision %d", APP_VERSION_STRING, APP_REVISION);
        ImGui::Text("Build date: %s %s", __DATE__, __TIME__);

        ImGui::Text("Source code: "); ImGui::SameLine();
        ImGui::TextLinkOpenURL("github.com/nzeemin/ukncbtl-imgui", "https://github.com/nzeemin/ukncbtl-imgui");
        ImGui::Spacing();

        ImGui::SeparatorText("ImGui");
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
    if (ImGui::Button("120")) MainWindow_DoEmulatorSpeed(2);
    ImGui::EndDisabled();
    ImGui::SameLine(0.0f, 0.0f);
    ImGui::BeginDisabled(speed == 3);
    if (ImGui::Button("240")) MainWindow_DoEmulatorSpeed(3);
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

    ImVec2 vMouse = ScreenView_GetMousePos();
    ImGui::TextDisabled("X:");
    ImGui::SameLine();
    if (vMouse.x != vMouse.x)
        ImGui::TextUnformatted("   ");
    else
        ImGui::Text("%3.f", floor(vMouse.x));
    ImGui::SameLine();
    ImGui::TextDisabled("Y:");
    ImGui::SameLine();
    if (vMouse.y != vMouse.y)
        ImGui::TextUnformatted("   ");
    else
        ImGui::Text("%3.f", floor(vMouse.y));

    if (ImGui::Button(ICON_FA_CAMERA " Screenshot"))
        MainWindow_DoFileScreenshot();

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

    ImGui::SeparatorText("Sound");
    bool sound = Settings_GetSound();
    if (ImGui::Checkbox("Sound", &sound))
        MainWindow_DoEmulatorSound(sound);
    if (Emulator_IsSound())
    {
        ImGui::SameLine();
        ImGui::TextUnformatted(ICON_FA_VOLUME_UP);
    }
    int volume = Settings_GetSoundVolume() / 655;
    if (ImGui::SliderInt("Volume", &volume, 0, 100))
        MainWindow_EmulatorSoundVolume(volume * 655);

    ImGui::SeparatorText("ImGui");
    ImGui::TextDisabled("FPS: %.1f", ImGui::GetIO().Framerate);
    if (g_okVsyncSwitchable)
    {
        bool vsync = Settings_GetScreenVsync();
        if (ImGui::Checkbox("VSync", &vsync))
        {
            Settings_SetScreenVsync(vsync);
            SetVSync();
        }
    }

#ifdef _DEBUG
    ImGui::Spacing();
    ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
#endif

    ImGui::Spacing();
    if (ImGui::Button(ICON_FA_COG " Settings"))
        open_settings_popup = true;

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

void MainWindow_DoScreenSizeMode(ScreenSizeMode mode)
{
    Settings_SetScreenSizeMode(mode);
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

void MainWindow_DoEmulatorSound(bool flag)
{
    Settings_SetSound(flag);

    Emulator_SetSound(flag);
}

void MainWindow_EmulatorSoundVolume(uint16_t volume)
{
    Settings_SetSoundVolume((WORD)volume);
}

void MainWindow_DoFileScreenshot()
{
    char bufFileName[MAX_PATH];
    SYSTEMTIME st;
    ::GetSystemTime(&st);
    _snprintf(bufFileName, IM_ARRAYSIZE(bufFileName),
        "%04d%02d%02d%02d%02d%02d%03d.png",
        st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

    int screenshotMode = Settings_GetScreenshotMode();
    if (!ScreenView_SaveScreenshot(bufFileName, screenshotMode))
    {
        AlertWarning(_T("Failed to save screenshot image."));
    }
}


//////////////////////////////////////////////////////////////////////
