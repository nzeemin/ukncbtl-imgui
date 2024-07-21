
#include "stdafx.h"
#include "imgui/imgui_internal.h"
#include "Emulator.h"
#include "emubase/Emubase.h"

// Data stored per platform window
struct WGL_WindowData { HDC hDC; };

// Data
static HGLRC            g_hRC;
static WGL_WindowData   g_MainWindow;
static int              g_Width;
static int              g_Height;


//////////////////////////////////////////////////////////////////////
// Forward declarations

bool CreateDeviceWGL(HWND hWnd, WGL_WindowData* data);
void CleanupDeviceWGL(HWND hWnd, WGL_WindowData* data);
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


//////////////////////////////////////////////////////////////////////
// Global Variables

HINSTANCE g_hInst = NULL; // current instance
HWND g_hwnd = NULL;

LPCTSTR g_szTitle = _T("UKNCBTL");

bool g_okDefaultLayout = true;

double g_dFramesPercent = 0.0;

bool g_okDebugCpuPpu = false;

bool g_okVsyncSwitchable = false;


//////////////////////////////////////////////////////////////////////


// Support function for multi-viewports
// Unlike most other backend combination, we need specific hooks to combine Win32+OpenGL.
// We could in theory decide to support Win32-specific code in OpenGL backend via e.g. an hypothetical ImGui_ImplOpenGL3_InitForRawWin32().
static void Hook_Renderer_CreateWindow(ImGuiViewport* viewport)
{
    assert(viewport->RendererUserData == NULL);

    WGL_WindowData* data = IM_NEW(WGL_WindowData);
    CreateDeviceWGL((HWND)viewport->PlatformHandle, data);
    viewport->RendererUserData = data;
}

static void Hook_Renderer_DestroyWindow(ImGuiViewport* viewport)
{
    if (viewport->RendererUserData != NULL)
    {
        WGL_WindowData* data = (WGL_WindowData*)viewport->RendererUserData;
        CleanupDeviceWGL((HWND)viewport->PlatformHandle, data);
        IM_DELETE(data);
        viewport->RendererUserData = NULL;
    }
}

static void Hook_Platform_RenderWindow(ImGuiViewport* viewport, void*)
{
    // Activate the platform window DC in the OpenGL rendering context
    if (WGL_WindowData* data = (WGL_WindowData*)viewport->RendererUserData)
        wglMakeCurrent(data->hDC, g_hRC);
}

static void Hook_Renderer_SwapBuffers(ImGuiViewport* viewport, void*)
{
    if (WGL_WindowData* data = (WGL_WindowData*)viewport->RendererUserData)
        ::SwapBuffers(data->hDC);
}


void ImGuiFrame()
{
    ImGui::NewFrame();

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    // Set the parent window's position, size, and viewport to match that of the main viewport.
    // This is so the parent window completely covers the main viewport, giving it a "full-screen" feel.
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    // Set the parent window's styles to match that of the main viewport:

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking |
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
        ImGuiWindowFlags_NoBackground;
    ImGui::Begin("Root", nullptr, window_flags);

    ImGui::PopStyleVar();

    ImGuiID dockspace_id = ImGui::GetID("Root");
    ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

    if (g_okDefaultLayout)  // This is first run without any layout, so let's define default one
    {
        g_okDefaultLayout = false;

        ImGui::DockBuilderRemoveNode(dockspace_id);
        ImGui::DockBuilderAddNode(dockspace_id, dockspace_flags /*| ImGuiDockNodeFlags_DockSpace*/);
        ImGuiID dockid_left = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.125f, nullptr, &dockspace_id);
        ImGuiID dockid_right = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Right, 0.55f, nullptr, &dockspace_id);
        ImGuiID dockid_right_up = ImGui::DockBuilderSplitNode(dockid_right, ImGuiDir_Up, 0.33f, nullptr, &dockid_right);
        ImGuiID dockid_right_down = ImGui::DockBuilderSplitNode(dockid_right, ImGuiDir_Down, 0.5f, nullptr, &dockid_right);
        ImGuiID dockid_right_up2 = ImGui::DockBuilderSplitNode(dockid_right_up, ImGuiDir_Right, 0.58f, nullptr, &dockid_right_up);
        ImGuiID dockid_down = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Down, 0.5f, nullptr, &dockspace_id);
        ImGuiID dockid_center = dockspace_id;

        ImGui::DockBuilderDockWindow("Control", dockid_left);
        ImGui::DockBuilderDockWindow("Video", dockid_center);
        ImGui::DockBuilderDockWindow("Console", dockid_down);
        ImGui::DockBuilderDockWindow("###Processor", dockid_right_up);
        ImGui::DockBuilderDockWindow("Stack", dockid_right_up2);
        ImGui::DockBuilderDockWindow("Breakpoints", dockid_right_up2);
        ImGui::DockBuilderDockWindow("Memory Schema", dockid_right_up2);
        ImGui::DockBuilderDockWindow("Disasm", dockid_right);
        ImGui::DockBuilderDockWindow("Memory", dockid_right_down);
    }

    ControlView_ImGuiWidget();

    ScreenView_ImGuiWidget();

    ConsoleView_ImGuiWidget();

    ProcessorView_ImGuiWidget();
    StackView_ImGuiWidget();
    Breakpoints_ImGuiWidget();
    MemorySchema_ImGuiWidget();
    DisasmView_ImGuiWidget();
    MemoryView_ImGuiWidget();

    ImGuiMainMenu();

    ImGui::End();  // Root
}

bool IsFileExist(const char * fileName)
{
    FILE* fp = fopen(fileName, "r");
    if (fp)
    {
        fclose(fp);
        return true;
    }
    return errno != ENOENT;
}

void SetVSync()
{
    bool sync = Settings_GetScreenVsync();

    typedef BOOL(APIENTRY* PFNWGLSWAPINTERVALPROC)(int);
    PFNWGLSWAPINTERVALPROC wglSwapIntervalEXT = nullptr;

    const char* extensions = (char*)glGetString(GL_EXTENSIONS);

    wglSwapIntervalEXT = (PFNWGLSWAPINTERVALPROC)wglGetProcAddress("wglSwapIntervalEXT");

    if (wglSwapIntervalEXT == nullptr)
    {
        g_okVsyncSwitchable = false;
        return;
    }

    wglSwapIntervalEXT(sync);
    g_okVsyncSwitchable = true;
}

// Main code
#ifdef _WIN32
int WinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPSTR /*lpCmdLine*/, int /*nShowCmd*/)
#else
int main(int, char**)
#endif
{
    g_hInst = hInstance; // Store instance handle in our global variable

    // Create application window
    //ImGui_ImplWin32_EnableDpiAwareness();
    WNDCLASSEXW wc = {
        sizeof(wc), CS_OWNDC, WndProc, 0L, 0L,
        GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr,
        L"ImGui UKNCBTL", nullptr };
    ::RegisterClassExW(&wc);

    Settings_Init();
    g_okDebugCpuPpu = Settings_GetDebugCpuPpu();

    if (!Emulator_Init())
        return 255;
    Emulator_SetSound(Settings_GetSound() != 0);
    Emulator_SetSoundAY(Settings_GetSoundAY() != 0);
    Emulator_SetSoundCovox(Settings_GetSoundCovox() != 0);

    ScreenView_Init();  // Creates m_bits
    ConsoleView_Init();

    //TODO: Restore main window settings

    HWND hwnd = ::CreateWindowW(
        wc.lpszClassName, L"ImGui UKNCBTL Win32+OpenGL3", WS_OVERLAPPEDWINDOW,
        0, 0, 1280, 800, nullptr, nullptr,
        wc.hInstance, nullptr);

    // Initialize OpenGL
    if (!CreateDeviceWGL(hwnd, &g_MainWindow))
    {
        CleanupDeviceWGL(hwnd, &g_MainWindow);
        ::DestroyWindow(hwnd);
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }
    wglMakeCurrent(g_MainWindow.hDC, g_hRC);

    // Show the window
    ::ShowWindow(hwnd, SW_SHOWMAXIMIZED);
    ::UpdateWindow(hwnd);

    MainWindow_RestoreImages();

    if (Settings_GetAutostart())
        Emulator_Start();

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;   // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;    // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;       // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;     // Enable Multi-Viewport / Platform Windows

    g_okDefaultLayout = !IsFileExist(io.IniFilename);

    // Setup Dear ImGui style
    MainWindow_SetColorSheme(Settings_GetColorScheme());

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }
    style.FrameBorderSize = 0.6f;

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_InitForOpenGL(hwnd);
    ImGui_ImplOpenGL3_Init();

    // Create a OpenGL texture identifier
    GLuint screen_texture;
    glGenTextures(1, &screen_texture);
    glBindTexture(GL_TEXTURE_2D, screen_texture);
    // Setup filtering parameters for display
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, UKNC_SCREEN_WIDTH, UKNC_SCREEN_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_bits);
    // Unbind texture
    glBindTexture(GL_TEXTURE_2D, 0);

    g_ScreenTextureID = (void*)(intptr_t) screen_texture;

    SetVSync();  // Set initial Vsync flag value

    // Win32+GL needs specific hooks for viewport, as there are specific things needed to tie Win32 and GL api.
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
        IM_ASSERT(platform_io.Renderer_CreateWindow == NULL);
        IM_ASSERT(platform_io.Renderer_DestroyWindow == NULL);
        IM_ASSERT(platform_io.Renderer_SwapBuffers == NULL);
        IM_ASSERT(platform_io.Platform_RenderWindow == NULL);
        platform_io.Renderer_CreateWindow = Hook_Renderer_CreateWindow;
        platform_io.Renderer_DestroyWindow = Hook_Renderer_DestroyWindow;
        platform_io.Renderer_SwapBuffers = Hook_Renderer_SwapBuffers;
        platform_io.Platform_RenderWindow = Hook_Platform_RenderWindow;
    }

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    float baseFontSize = 14.0f;
    io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\lucon.ttf", baseFontSize);
    //io.Fonts->AddFontFromFileTTF("UbuntuMono-R.ttf", baseFontSize);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != nullptr);
    float iconFontSize = baseFontSize; //*2.0f / 3.0f; // FontAwesome fonts need to have their sizes reduced by 2.0f/3.0f in order to align correctly
    static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_16_FA, 0 };
    ImFontConfig icons_config;
    icons_config.MergeMode = true;
    icons_config.PixelSnapH = true;
    icons_config.GlyphMinAdvanceX = iconFontSize;
    io.Fonts->AddFontFromFileTTF(FONT_ICON_FILE_NAME_FAS, iconFontSize, &icons_config, icons_ranges);

    // Our state
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    LARGE_INTEGER nFrameStartTime;
    nFrameStartTime.QuadPart = 0;
    LARGE_INTEGER nPerformanceFrequency;
    ::QueryPerformanceFrequency(&nPerformanceFrequency);

    // Main loop
    bool done = false;
    while (!done)
    {
        ::QueryPerformanceCounter(&nFrameStartTime);

        // Poll and handle messages (inputs, window resize, etc.)
        // See the WndProc() function below for our to dispatch events to the Win32 backend.
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        if (g_okEmulatorRunning)
        {
            if (!Emulator_SystemFrame())  // Breakpoint hit
            {
                Emulator_Stop();
            }

            IM_ASSERT(m_bits != nullptr);
            const uint32_t* palette = ScreenView_GetPalette();
            Emulator_PrepareScreenRGB32(m_bits, palette);

            glBindTexture(GL_TEXTURE_2D, screen_texture);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, UKNC_SCREEN_WIDTH, UKNC_SCREEN_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, m_bits);
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGuiFrame();

        // Rendering
        ImGui::Render();
        glViewport(0, 0, g_Width, g_Height);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Update and Render additional Platform Windows
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();

            // Restore the OpenGL rendering context to the main window DC, since platform windows might have changed it.
            wglMakeCurrent(g_MainWindow.hDC, g_hRC);
        }

        // Present
        ::SwapBuffers(g_MainWindow.hDC);

        if (!g_okEmulatorRunning)
            ::Sleep(1);
        else
        {
            if (Settings_GetRealSpeed() == 0)
                ::Sleep(0);  // We should not consume 100% of CPU
            else
            {
                LONGLONG nFrameDelay = 1000ll / 25 - 8;  // 1000 millisec / 25 = 40 millisec
                if (Settings_GetRealSpeed() == 0x7ffe)  // Speed 25%
                    nFrameDelay = 1000ll / 25 * 4 - 3;
                else if (Settings_GetRealSpeed() == 0x7fff)  // Speed 50%
                    nFrameDelay = 1000ll / 25 * 2 - 3;
                else if (Settings_GetRealSpeed() == 2)  // Speed 200%
                    nFrameDelay = 1000ll / 25 / 2 - 4;

                for (;;)
                {
                    LARGE_INTEGER nFrameFinishTime;  // Frame start time
                    ::QueryPerformanceCounter(&nFrameFinishTime);
                    LONGLONG nTimeElapsed = (nFrameFinishTime.QuadPart - nFrameStartTime.QuadPart)
                        * 1000ll / nPerformanceFrequency.QuadPart;
                    if (nTimeElapsed <= 0 || nTimeElapsed >= nFrameDelay)
                        break;
                    DWORD nDelayRemaining = (DWORD)(nFrameDelay - nTimeElapsed);
                    if (nDelayRemaining == 1)
                        ::Sleep(0);
                    else
                        ::Sleep(nDelayRemaining / 2);
                }
            }
        }
    }

    Emulator_Done();
    Settings_Done();
    ScreenView_Done();

    glDeleteTextures(1, &screen_texture);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceWGL(hwnd, &g_MainWindow);
    wglDeleteContext(g_hRC);
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

// Helper functions
bool CreateDeviceWGL(HWND hWnd, WGL_WindowData* data)
{
    HDC hDc = ::GetDC(hWnd);
    PIXELFORMATDESCRIPTOR pfd = { 0 };
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;

    const int pf = ::ChoosePixelFormat(hDc, &pfd);
    if (pf == 0)
        return false;
    if (::SetPixelFormat(hDc, pf, &pfd) == FALSE)
        return false;
    ::ReleaseDC(hWnd, hDc);

    data->hDC = ::GetDC(hWnd);
    if (!g_hRC)
        g_hRC = wglCreateContext(data->hDC);
    return true;
}

void CleanupDeviceWGL(HWND hWnd, WGL_WindowData* data)
{
    wglMakeCurrent(nullptr, nullptr);
    ::ReleaseDC(hWnd, data->hDC);
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (wParam != SIZE_MINIMIZED)
        {
            g_Width = LOWORD(lParam);
            g_Height = HIWORD(lParam);
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
