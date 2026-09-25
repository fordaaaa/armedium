#include "renderer.h"
#include "menu.h"
#include "../rbx/Webhook.h"

ID3D11Device* g_pd3dDevice = nullptr;
ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
IDXGISwapChain* g_pSwapChain = nullptr;
bool g_SwapChainOccluded = false;
UINT g_ResizeWidth = 0, g_ResizeHeight = 0;
ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;
HWND g_overlayHwnd = nullptr;


bool IsGameOnTop(const std::string& expectedTitle) {
    HWND hwnd = GetForegroundWindow();
    if (!hwnd) return false;

    char windowTitle[256];
    int length = GetWindowTextA(hwnd, windowTitle, sizeof(windowTitle));

    if (length == 0) return false;

    return expectedTitle == std::string(windowTitle);
}

void SetTransparency(HWND hwnd, bool boolean)
{
    LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
    if (boolean)
    {
        exStyle |= WS_EX_TRANSPARENT;
        SetWindowLong(hwnd, GWL_EXSTYLE, exStyle);
    }
    else
    {
        exStyle &= ~WS_EX_TRANSPARENT;
        SetWindowLong(hwnd, GWL_EXSTYLE, exStyle);
    }
}

void SyncOverlayToRoblox()
{
    HWND robloxHwnd = FindWindowW(NULL, L"Roblox");
    if (!robloxHwnd)
    {
        if (IsWindowVisible(g_overlayHwnd))
            ShowWindow(g_overlayHwnd, SW_HIDE);
        return;
    }

    if (IsIconic(robloxHwnd))
    {
        if (IsWindowVisible(g_overlayHwnd))
            ShowWindow(g_overlayHwnd, SW_HIDE);
        return;
    }

    if (!IsWindowVisible(g_overlayHwnd))
    {
        ShowWindow(g_overlayHwnd, SW_SHOW);
        SetWindowPos(g_overlayHwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    }

    RECT rect;
    GetWindowRect(robloxHwnd, &rect);
    int w = rect.right - rect.left;
    int h = rect.bottom - rect.top;

    RECT overlayRect;
    GetWindowRect(g_overlayHwnd, &overlayRect);
    int ow = overlayRect.right - overlayRect.left;
    int oh = overlayRect.bottom - overlayRect.top;

    if (rect.left != overlayRect.left || rect.top != overlayRect.top || w != ow || h != oh)
    {
        SetWindowPos(g_overlayHwnd, HWND_TOPMOST, rect.left, rect.top, w + 1, h + 1, SWP_SHOWWINDOW);
    }
}

void DrawNode(RobloxInstance& node)
{
    const auto& children = node.GetChildren();
    if (children.empty())
    {
        ImGui::BulletText(node.Name().c_str());
    }
    else
    {
        if (ImGui::TreeNode(node.Name().c_str()))
        {
            for (auto child : children)
            {
                DrawNode(child);
            }
            ImGui::TreePop();
        }
    }
}

void RenderKeybindList(ImDrawList* drawList)
{
    if (!Options::Misc::KeybindList)
        return;

    ImGuiIO& io = ImGui::GetIO();
    std::vector<std::pair<std::string, std::string>> activeBinds;

    // Check Aimbot
    if (Options::Aimbot::Aimbot && Options::Aimbot::AimbotKey != 0)
    {
        bool isActive = false;
        if (Options::Aimbot::ToggleType == 1) // Toggle
            isActive = Options::Aimbot::Toggled;
        else // Hold
            isActive = (GetAsyncKeyState(Options::Aimbot::AimbotKey) & 0x8000) != 0;
        
        if (isActive)
            activeBinds.push_back({"Aimbot", Options::Aimbot::ToggleType == 1 ? "[Toggled]" : "[Hold]"});
    }

    // Check Triggerbot
    if (Options::Triggerbot::Enabled && Options::Triggerbot::TriggerbotKey != 0)
    {
        bool isActive = false;
        if (Options::Triggerbot::ToggleType == 1) // Toggle
            isActive = Options::Triggerbot::Toggled;
        else // Hold
            isActive = (GetAsyncKeyState(Options::Triggerbot::TriggerbotKey) & 0x8000) != 0;
        
        if (isActive)
            activeBinds.push_back({"Triggerbot", Options::Triggerbot::ToggleType == 1 ? "[Toggled]" : "[Hold]"});
    }

    // Check Fly
    if (Options::Fly::Enabled && Options::Fly::FlyKey != 0)
    {
        bool isActive = false;
        if (Options::Fly::ToggleType == 1) // Toggle
            isActive = Options::Fly::Toggled;
        else // Hold
            isActive = (GetAsyncKeyState(Options::Fly::FlyKey) & 0x8000) != 0;
        
        if (isActive)
            activeBinds.push_back({"Fly", Options::Fly::ToggleType == 1 ? "[Toggled]" : "[Hold]"});
    }

    // Check WalkSpeed
    if (Options::WalkSpeed::Enabled && Options::WalkSpeed::WalkSpeedKey != 0)
    {
        bool isActive = false;
        if (Options::WalkSpeed::ToggleType == 1) // Toggle
            isActive = Options::WalkSpeed::Toggled;
        else // Hold
            isActive = (GetAsyncKeyState(Options::WalkSpeed::WalkSpeedKey) & 0x8000) != 0;
        
        if (isActive)
            activeBinds.push_back({"WalkSpeed", Options::WalkSpeed::ToggleType == 1 ? "[Toggled]" : "[Hold]"});
    }

    // Check Fling
    if (Options::Fling::Enabled && Options::Fling::FlingKey != 0)
    {
        bool isActive = false;
        if (Options::Fling::ToggleType == 1)
            isActive = Options::Fling::Toggled;
        else
            isActive = (GetAsyncKeyState(Options::Fling::FlingKey) & 0x8000) != 0;
        if (isActive)
            activeBinds.push_back({"Fling", Options::Fling::ToggleType == 1 ? "[Toggled]" : "[Hold]"});
    }

    // Check Silent Aim
    if (Options::SilentAim::Enabled && Options::SilentAim::Key != 0)
    {
        bool isActive = false;
        if (Options::SilentAim::ToggleType == 1)
            isActive = Options::SilentAim::Toggled;
        else
            isActive = (GetAsyncKeyState(Options::SilentAim::Key) & 0x8000) != 0;
        if (isActive)
            activeBinds.push_back({"SilentAim", Options::SilentAim::ToggleType == 1 ? "[Toggled]" : "[Hold]"});
    }

    if (activeBinds.empty())
        return;

    // Calculate dimensions - much smaller and compact
    float padding = 8.0f;
    float lineHeight = 14.0f;
    float titleHeight = 20.0f;
    float minWidth = 150.0f; // Reduced minimum width
    float maxWidth = minWidth;
    
    for (const auto& bind : activeBinds)
    {
        std::string fullText = bind.first + " " + bind.second;
        float textWidth = ImGui::CalcTextSize(fullText.c_str()).x;
        if (textWidth > maxWidth)
            maxWidth = textWidth;
    }
    
    float boxWidth = maxWidth + padding * 2;
    float boxHeight = titleHeight + (activeBinds.size() * lineHeight) + padding;
    
    // Use custom position from sliders
    ImVec2 pos = ImVec2(Options::Misc::KeybindListX, Options::Misc::KeybindListY);
    
    // Draw background - fully opaque (255 alpha instead of 200)
    drawList->AddRectFilled(pos, ImVec2(pos.x + boxWidth, pos.y + boxHeight), IM_COL32(8, 8, 8, 255), 4.0f);
    drawList->AddRect(pos, ImVec2(pos.x + boxWidth, pos.y + boxHeight), IM_COL32(27, 27, 27, 255), 4.0f);
    
    // Draw title - centered
    const char* title = "Keybinds";
    float titleWidth = ImGui::CalcTextSize(title).x;
    float titleX = pos.x + (boxWidth - titleWidth) / 2.0f;
    drawList->AddText(ImVec2(titleX, pos.y + 4), IM_COL32(255, 255, 255, 255), title);
    drawList->AddLine(ImVec2(pos.x, pos.y + titleHeight), ImVec2(pos.x + boxWidth, pos.y + titleHeight), IM_COL32(27, 27, 27, 255));
    
    // Draw active binds - centered
    float yOffset = pos.y + titleHeight + 3;
    for (const auto& bind : activeBinds)
    {
        std::string fullText = bind.first + " " + bind.second;
        float textWidth = ImGui::CalcTextSize(fullText.c_str()).x;
        float textX = pos.x + (boxWidth - textWidth) / 2.0f;
        
        // Draw the full text centered
        drawList->AddText(ImVec2(textX, yOffset), IM_COL32(255, 255, 255, 255), bind.first.c_str());
        
        // Draw status in accent color right after the name
        float nameWidth = ImGui::CalcTextSize(bind.first.c_str()).x;
        drawList->AddText(ImVec2(textX + nameWidth + 5, yOffset), IM_COL32(main_color.x * 255, main_color.y * 255, main_color.z * 255, 255), bind.second.c_str());
        
        yOffset += lineHeight;
    }
}

void ShowImgui()
{
    ImGui_ImplWin32_EnableDpiAwareness();
    float main_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));

    size_t width = (size_t)GetSystemMetrics(SM_CXSCREEN);
    size_t height = (size_t)GetSystemMetrics(SM_CYSCREEN);

    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr,         L"Armedium", nullptr };
    ::RegisterClassExW(&wc);

    g_overlayHwnd = ::CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TOOLWINDOW,
        wc.lpszClassName,
        L"Armedium",
        WS_POPUP,
        0, 0, (int)width + 1, (int)height + 1,
        nullptr, nullptr, wc.hInstance, nullptr);

    HWND hwnd = g_overlayHwnd;
    SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 255, LWA_ALPHA);
    MARGINS Margin = { -1 };
    DwmExtendFrameIntoClientArea(hwnd, &Margin);
    
    // Apply streamproof if enabled (WDA_EXCLUDEFROMCAPTURE = 0x00000011)
    if (Options::Misc::StreamProof)
    {
        SetWindowDisplayAffinity(hwnd, 0x00000011);
    }

    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
    }

    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    ImGui::StyleColorsDark();

    ImFontConfig config;
    config.MergeMode = false;
    config.PixelSnapH = true;

    ImFont* baseFont = io.Fonts->AddFontDefault(&config);
    ImFont* font = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\verdana.ttf", 13.0f, &config, io.Fonts->GetGlyphRangesJapanese());

    config.MergeMode = true;
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);
    ImGui_ImplDX11_CreateDeviceObjects();

    ImVec4 clear_color = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

    bool done = false;
    bool menu_open = true;

    while (!done)
    {
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

        if (g_SwapChainOccluded && g_pSwapChain->Present(0, 0) == DXGI_STATUS_OCCLUDED)
        {
            ::Sleep(10);
            continue;
        }
        g_SwapChainOccluded = false;

        SyncOverlayToRoblox();

        if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = g_ResizeHeight = 0;
            CreateRenderTarget();
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        if (GetAsyncKeyState(VK_INSERT) & 1)
        {
            menu_open = !menu_open;
            SetTransparency(hwnd, !menu_open);
            LONG exStyle = GetWindowLong(g_overlayHwnd, GWL_EXSTYLE);
            if (menu_open)
                exStyle |= WS_EX_TOPMOST;
            else
                exStyle &= ~WS_EX_TOPMOST;
            SetWindowLong(g_overlayHwnd, GWL_EXSTYLE, exStyle);
            SetWindowPos(g_overlayHwnd, menu_open ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        }
        
        Menu::Render(hwnd, io, font, menu_open);

        if (IsGameOnTop("Roblox"))
        {
            // Clear any leftover viewport shift (no-op unless it changed);
            // RunAimbot re-applies it on the same frame when Viewport aim is active.
            ApplyViewportAim(false, { 0.f, 0.f });

            RenderESP(ImGui::GetBackgroundDrawList(), menu_open);
            RenderPartChams(ImGui::GetBackgroundDrawList(), menu_open);
            RenderHitboxChams(ImGui::GetBackgroundDrawList(), menu_open);

            if (!menu_open)
            {
                RunAimbot(ImGui::GetBackgroundDrawList());
                RunTriggerbot();

                // Silent aim 2D FOV circle (screen-space, around the crosshair)
                if (Options::SilentAim::ShowFOV)
                {
                    POINT sp;
                    GetCursorPos(&sp);
                    ImColor saColor = IM_COL32(
                        static_cast<int>(Options::Aimbot::FOVColor[0] * 255.f),
                        static_cast<int>(Options::Aimbot::FOVColor[1] * 255.f),
                        static_cast<int>(Options::Aimbot::FOVColor[2] * 255.f),
                        255);
                    ImGui::GetBackgroundDrawList()->AddCircle(
                        ImVec2(static_cast<float>(sp.x), static_cast<float>(sp.y)),
                        Options::SilentAim::FOV, saColor, 0, 1.5f);
                }
                RunMacro();

                // Teleport helpers
                auto TeleportToPos = [](const Vectors::Vector3& targetPos) {
                    try
                    {
                        auto localChar = Globals::Roblox::LocalPlayer.Character();
                        if (!localChar.address) return;
                        auto hrp = localChar.FindFirstChild("HumanoidRootPart");
                        if (!hrp.address) return;
                        uintptr_t prim = Memory->read<uintptr_t>(hrp.address + Offsets::BasePart::Primitive);
                        if (!prim) return;
                        Vectors::Vector3 pos = targetPos;
                        pos.y += 5;
                        Memory->write<Vectors::Vector3>(prim + Offsets::Primitive::Position, pos);
                    }
                    catch (...) {}
                };

                // Ctrl+Click Teleport
                if (Options::Teleport::CtrlClickTP && Options::Teleport::TPKey == 0)
                {
                    static bool wasLMB = false;
                    bool lmb = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
                    bool ctrl = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
                    if (lmb && !wasLMB && ctrl)
                    {
                        POINT cursor;
                        GetCursorPos(&cursor);
                        float cx = static_cast<float>(cursor.x);
                        float cy = static_cast<float>(cursor.y);

                        RobloxPlayer bestTarget;
                        float bestDist = 400.0f * 400.0f;
                        auto ctrlClickPlayers = SnapshotCachedPlayerObjects();
                        for (auto& player : ctrlClickPlayers)
                        {
                            if (IsLocalPlayerEntry(player)) continue;
                            auto part = player.HumanoidRootPart;
                            if (!part.address) continue;

                            auto w2s = WorldToScreen(part.Position());
                            if (w2s.x < 0 || w2s.y < 0) continue;
                            float dx = w2s.x - cx;
                            float dy = w2s.y - cy;
                            float dist = dx * dx + dy * dy;
                            if (dist < bestDist)
                            {
                                bestDist = dist;
                                bestTarget = player;
                            }
                        }

                        if (bestTarget.address && bestTarget.HumanoidRootPart.address)
                        {
                            TeleportToPos(bestTarget.HumanoidRootPart.Position());
                        }
                    }
                    wasLMB = lmb;
                }

                // TP keybind
                if (Options::Teleport::TPKey != 0 && Options::Teleport::TPToPlayers && Options::Teleport::SelectedPlayer >= 0)
                {
                    static bool tpWasPressed = false;
                    bool tpPressed = (GetAsyncKeyState(Options::Teleport::TPKey) & 0x8000) != 0;
                    bool shouldTP = false;
                    if (Options::Teleport::ToggleType == 1)
                    {
                        if (tpPressed && !tpWasPressed)
                            Options::Teleport::Toggled = !Options::Teleport::Toggled;
                        shouldTP = Options::Teleport::Toggled;
                    }
                    else
                    {
                        shouldTP = tpPressed;
                    }
                    tpWasPressed = tpPressed;

                    if (shouldTP)
                    {
                        int idx = Options::Teleport::SelectedPlayer;
                        auto players = SnapshotCachedPlayerObjects();
                        if (idx >= 0 && idx < (int)players.size())
                        {
                            auto target = players[idx];
                            if (!IsLocalPlayerEntry(target) && target.HumanoidRootPart.address)
                            {
                                TeleportToPos(target.HumanoidRootPart.Position());
                            }
                        }
                    }
                }
            }
            
            // Render advanced FOV visualization even when menu is open
            RenderAdvancedFOV(ImGui::GetBackgroundDrawList());
            
            // Render crosshair even when menu is open
            RenderCrosshair(ImGui::GetBackgroundDrawList());
            
            // Render keybind list
            RenderKeybindList(ImGui::GetBackgroundDrawList());

            std::string str = std::to_string(static_cast<int>(io.Framerate)) + " FPS";
            ImVec2 textSize = ImGui::CalcTextSize(str.c_str());
            ImVec2 pos = ImVec2(io.DisplaySize.x - textSize.x - 10.0f, 10.0f);
            ImDrawList* drawList = ImGui::GetBackgroundDrawList();
            drawList->AddText(pos, IM_COL32(255, 255, 255, 200), str.c_str());
        }

        static float lastAutoSave = 0;
        if (Globals::Initialized && ImGui::GetTime() - lastAutoSave > 30.0f)
        {
            CreateConfig("autosave.json");
            lastAutoSave = (float)ImGui::GetTime();
        }

        ImGui::Render();
        const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        HRESULT hr = g_pSwapChain->Present(0, 0);
        g_SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
}

bool CreateDeviceD3D(HWND hWnd)
{
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 4;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK) return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED) return 0;
        g_ResizeWidth = (UINT)LOWORD(lParam);
        g_ResizeHeight = (UINT)HIWORD(lParam);
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}

void RenderNotifications(ImDrawList* drawList) {}
void RenderRadar(ImDrawList* drawList) {}
void RenderFPSCounter(ImDrawList* drawList) {}
void RenderPerformanceMetrics(ImDrawList* drawList) {}
void ApplyTheme(int themeId) {}
void SetDarkTheme() {}
void SetLightTheme() {}
void SetPinkTheme() {}
void SetPurpleTheme() {}
void SetBlueTheme() {}
void SetGreenTheme() {}
float EaseInOutQuad(float t) { return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t; }
float EaseInQuad(float t) { return t * t; }
float EaseOutQuad(float t) { return 1.0f - (1.0f - t) * (1.0f - t); }
ImU32 InterpolateColor(ImU32 colA, ImU32 colB, float t)
{
    ImU32 a = colA, b = colB;
    ImU8 r = (ImU8)(ImU32((a >> IM_COL32_R_SHIFT) & 0xFF) * (1.0f - t) + ImU32((b >> IM_COL32_R_SHIFT) & 0xFF) * t);
    ImU8 g = (ImU8)(ImU32((a >> IM_COL32_G_SHIFT) & 0xFF) * (1.0f - t) + ImU32((b >> IM_COL32_G_SHIFT) & 0xFF) * t);
    ImU8 bch = (ImU8)(ImU32((a >> IM_COL32_B_SHIFT) & 0xFF) * (1.0f - t) + ImU32((b >> IM_COL32_B_SHIFT) & 0xFF) * t);
    ImU8 alpha = (ImU8)(ImU32((a >> IM_COL32_A_SHIFT) & 0xFF) * (1.0f - t) + ImU32((b >> IM_COL32_A_SHIFT) & 0xFF) * t);
    return IM_COL32(r, g, bch, alpha);
}
float GetPulseValue(float speed, float intensity) { return 0.5f + 0.5f * sinf(speed * (float)ImGui::GetTime()) * intensity; }
void DrawGradientRect(ImDrawList* drawList, const ImVec2& p_min, const ImVec2& p_max, ImU32 col_upr_left, ImU32 col_upr_right, ImU32 col_btm_right, ImU32 col_btm_left) {}
void DrawHorizontalGradient(ImDrawList* drawList, const ImVec2& p_min, const ImVec2& p_max, ImU32 col_left, ImU32 col_right) {}
void DrawVerticalGradient(ImDrawList* drawList, const ImVec2& p_min, const ImVec2& p_max, ImU32 col_top, ImU32 col_bottom) {}
void DrawPulsingGradientRect(ImDrawList* drawList, const ImVec2& p_min, const ImVec2& p_max, ImU32 col_primary, ImU32 col_secondary) {}
void StartTabTransition() {}
float GetTabTransitionAlpha() { return 1.0f; }
