#include "renderer.h"
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
    int tab = 0;
    int tab2 = 0;

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
        
        // Fade animation
        static float menuAlpha = 0.0f;
        static float backgroundAlpha = 0.0f;
        float fadeSpeed = 0.08f; // Adjust for faster/slower fade
        
        if (menu_open)
        {
            if (menuAlpha < 1.0f) menuAlpha += fadeSpeed;
            if (menuAlpha > 1.0f) menuAlpha = 1.0f;
            
            if (backgroundAlpha < 0.7f) backgroundAlpha += fadeSpeed;
            if (backgroundAlpha > 0.7f) backgroundAlpha = 0.7f;
        }
        else
        {
            if (menuAlpha > 0.0f) menuAlpha -= fadeSpeed;
            if (menuAlpha < 0.0f) menuAlpha = 0.0f;
            
            if (backgroundAlpha > 0.0f) backgroundAlpha -= fadeSpeed;
            if (backgroundAlpha < 0.0f) backgroundAlpha = 0.0f;
        }
        
        // Dynamic streamproof toggle
        static bool lastStreamProofState = Options::Misc::StreamProof;
        if (lastStreamProofState != Options::Misc::StreamProof)
        {
            if (Options::Misc::StreamProof)
            {
                SetWindowDisplayAffinity(hwnd, 0x00000011); // WDA_EXCLUDEFROMCAPTURE
            }
            else
            {
                SetWindowDisplayAffinity(hwnd, 0x00000000); // WDA_NONE
            }
            lastStreamProofState = Options::Misc::StreamProof;
        }

        static bool lastShowConsole = Options::Misc::ShowConsole;
        if (lastShowConsole != Options::Misc::ShowConsole)
        {
            ShowWindow(GetConsoleWindow(), Options::Misc::ShowConsole ? SW_SHOW : SW_HIDE);
            lastShowConsole = Options::Misc::ShowConsole;
        }
        
        // Update main_color from options
        main_color = ImVec4(Options::Misc::MenuAccentColor[0], Options::Misc::MenuAccentColor[1], Options::Misc::MenuAccentColor[2], 1.0f);

        if (menu_open || menuAlpha > 0.0f)
        {
            // Draw dark background overlay
            if (Options::Misc::DimBackground && backgroundAlpha > 0.0f)
            {
                ImGui::GetBackgroundDrawList()->AddRectFilled(
                    ImVec2(0, 0),
                    ImVec2(io.DisplaySize.x, io.DisplaySize.y),
                    IM_COL32(0, 0, 0, static_cast<int>(backgroundAlpha * 180))
                );
            }
            auto s = ImVec2{}, p = ImVec2{}, gs = ImVec2{ 750, 500 };
            ImGui::SetNextWindowSize(gs);
            ImGui::SetNextWindowBgAlpha(menuAlpha);
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, menuAlpha);
            ImGui::SetNextWindowPos(ImVec2((io.DisplaySize.x - gs.x) * 0.5f, (io.DisplaySize.y - gs.y) * 0.5f), ImGuiCond_Once);
            ImGui::Begin("##GUI", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground);
            {
                ImGui::PushFont(font);

                s = ImVec2(ImGui::GetWindowSize().x - ImGui::GetStyle().WindowPadding.x * 2, ImGui::GetWindowSize().y - ImGui::GetStyle().WindowPadding.y * 2);
                p = ImVec2(ImGui::GetWindowPos().x + ImGui::GetStyle().WindowPadding.x, ImGui::GetWindowPos().y + ImGui::GetStyle().WindowPadding.y);
                auto draw = ImGui::GetWindowDrawList();

                // Main background with rounded corners
                draw->AddRectFilled(ImVec2(p.x, p.y), ImVec2(p.x + s.x, p.y + s.y), IM_COL32(12, 12, 14, 245), 10.0f);
                draw->AddRect(ImVec2(p.x + 1, p.y + 1), ImVec2(p.x + s.x - 1, p.y + s.y - 1), IM_COL32(30, 30, 35, 255), 10.0f);

                // ============ SIDEBAR ============
                const float sbWidth = 72.0f;
                draw->AddRectFilled(ImVec2(p.x + 4, p.y + 4), ImVec2(p.x + sbWidth + 4, p.y + s.y - 4), IM_COL32(18, 18, 22, 255), 8.0f);
                draw->AddRect(ImVec2(p.x + 4, p.y + 4), ImVec2(p.x + sbWidth + 4, p.y + s.y - 4), IM_COL32(28, 28, 33, 255), 8.0f);

                draw->AddRectFilledMultiColor(
                    ImVec2(p.x + 6, p.y + 4),
                    ImVec2(p.x + sbWidth + 2, p.y + 26),
                    IM_COL32(main_color.x * 255, main_color.y * 255, main_color.z * 255, 80),
                    IM_COL32(main_color.x * 255, main_color.y * 255, main_color.z * 255, 10),
                    IM_COL32(18, 18, 22, 0),
                    IM_COL32(18, 18, 22, 0)
                );

                // Sidebar child for proper ImGui layout
                ImGui::SetCursorPos(ImVec2(4, 4));
                ImGui::BeginChild("##sidebar", ImVec2(sbWidth, s.y - 8), false, ImGuiWindowFlags_NoBackground);
                {
                    ImGui::SetCursorPosY(28);
                    const char* sbLabels[] = { "Aim", "Visuals", "Movement", "Misc" };
                    for (int i = 0; i < 4; i++)
                    {
                        ImVec2 btnSize(sbWidth - 8, 44);
                        ImGui::SetCursorPosX(4);
                        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, IM_COL32(24, 24, 30, 255));
                        if (tab == i)
                            ImGui::PushStyleColor(ImGuiCol_Header, IM_COL32(30, 30, 38, 255));
                        ImGui::PushStyleColor(ImGuiCol_Text, tab == i ? IM_COL32(255, 255, 255, 255) : IM_COL32(140, 140, 150, 255));
                        bool clicked = ImGui::Selectable(sbLabels[i], false, ImGuiSelectableFlags_None, btnSize);
                        ImGui::PopStyleColor(2 + (tab == i ? 1 : 0));
                        if (tab == i)
                        {
                            ImVec2 rMin = ImGui::GetItemRectMin();
                            ImVec2 rMax = ImGui::GetItemRectMax();
                            draw->AddRectFilled(ImVec2(rMin.x, rMin.y + 6), ImVec2(rMin.x + 3, rMax.y - 6),
                                IM_COL32(main_color.x * 255, main_color.y * 255, main_color.z * 255, 255), 2.0f);
                        }
                        if (clicked && tab != i) { tab = i; tab2 = 0; }
                    }
                }
                ImGui::EndChild();

                // Divider
                draw->AddLine(ImVec2(p.x + sbWidth + 8, p.y + 8), ImVec2(p.x + sbWidth + 8, p.y + s.y - 8), IM_COL32(28, 28, 33, 255));

                // ============ CONTENT AREA ============
                float cw = (s.x - (sbWidth + 36)) / 2;
                float contentX = p.x + sbWidth + 20;
                float contentY = p.y + 10;
                float contentW = s.x - (sbWidth + 28);
                float contentH = s.y - 44;

                draw->AddText(ImVec2(contentX, contentY), IM_COL32(255, 255, 255, 200),
                    tab == 0 ? "Aimbot" : tab == 1 ? "Visuals" : tab == 2 ? "Movement" : "Miscellaneous");
                draw->AddRectFilledMultiColor(
                    ImVec2(contentX, contentY + 18), ImVec2(contentX + contentW, contentY + 20),
                    IM_COL32(main_color.x * 255, main_color.y * 255, main_color.z * 255, 100),
                    IM_COL32(main_color.x * 255, main_color.y * 255, main_color.z * 255, 30),
                    IM_COL32(main_color.x * 255, main_color.y * 255, main_color.z * 255, 30),
                    IM_COL32(main_color.x * 255, main_color.y * 255, main_color.z * 255, 100)
                );

                // Content area child
                ImGui::SetCursorPos(ImVec2(sbWidth + 20, 32));
                ImGui::BeginChild("##content", ImVec2(contentW, contentH + 6), false, ImGuiWindowFlags_NoBackground);
                {
                    auto beginStyledChild = [&](const char* id, ImVec2 size) {
                        ImVec2 cPos = ImGui::GetCursorScreenPos();
                        draw->AddRectFilled(cPos, ImVec2(cPos.x + size.x, cPos.y + size.y), IM_COL32(18, 18, 22, 230), 8.0f);
                        draw->AddRect(cPos, ImVec2(cPos.x + size.x, cPos.y + size.y), IM_COL32(30, 30, 35, 255), 8.0f);
                        ImGui::BeginChild(id, size, false, ImGuiWindowFlags_NoBackground);
                    };
                    auto endStyledChild = [&]() {
                        ImGui::EndChild();
                    };

                    // ============ TAB CONTENT ============
                    static const char* aimSubtabNames[] = { "Aimbot", "Triggerbot", "Hitbox" };
                    static const char* visSubtabNames[] = { "ESP", "Colours" };
                    static const char* moveSubtabNames[] = { "Fly", "WalkSpeed", "Fling" };
                    static const char* miscSubtabNames[] = { "Local", "Silent Aim", "Config" };

                    auto renderSubtabBar = [&](const char** names, int count) {
                        ImGui::BeginGroup();
                        for (int i = 0; i < count; i++)
                        {
                            if (i > 0) ImGui::SameLine(0, 4);
                            bool sel = tab2 == i;
                            ImVec2 textSize = ImGui::CalcTextSize(names[i]);
                            ImVec2 subtabPos = ImGui::GetCursorScreenPos();
                            float sbw = textSize.x + 20;

                            ImU32 sbBg = sel ? IM_COL32(30, 30, 38, 255) : IM_COL32(0, 0, 0, 0);
                            ImU32 sbBorder = sel ? IM_COL32(main_color.x * 255, main_color.y * 255, main_color.z * 255, 80) : IM_COL32(35, 35, 40, 100);

                            draw->AddRectFilled(subtabPos, ImVec2(subtabPos.x + sbw, subtabPos.y + 24), sbBg, 4.0f);
                            if (sel)
                                draw->AddRect(subtabPos, ImVec2(subtabPos.x + sbw, subtabPos.y + 24), sbBorder, 4.0f);

                            ImGui::InvisibleButton(names[i], ImVec2(sbw, 24));
                            if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0))
                                tab2 = i;

                            ImVec2 textPos(subtabPos.x + (sbw - textSize.x) / 2, subtabPos.y + (24 - textSize.y) / 2);
                            draw->AddText(textPos,
                                sel ? IM_COL32(255, 255, 255, 255) : IM_COL32(140, 140, 150, 200),
                                names[i]);

                            ImGui::SameLine(0, 0);
                        }
                        ImGui::EndGroup();
                        ImGui::Dummy(ImVec2(0, 0));
                    };

                    if (tab == 0)
                    {
                        renderSubtabBar(aimSubtabNames, 3);
                        ImGui::Dummy(ImVec2(0, 6));

                        if (tab2 == 0)
                    {
                        beginStyledChild("##aimMain", ImVec2(cw, contentH - 58));
                        {
                            ImGui::PushStyleColor(ImGuiCol_CheckMark, main_color);
                            ImGui::Checkbox("Enabled", &Options::Aimbot::Aimbot);
                            ImGui::Checkbox("Team Check", &Options::Aimbot::TeamCheck);
                            ImGui::Checkbox("Knocked Check", &Options::Aimbot::DownedCheck);
                            ImGui::Checkbox("Sticky Aim", &Options::Aimbot::StickyAim);
                            ImGui::Checkbox("Prediction", &Options::Aimbot::Prediction);
                            ImGui::Checkbox("Shake", &Options::Aimbot::Shake);
                            ImGui::Checkbox("Stutter", &Options::Aimbot::Stutter);
                            ImGui::Checkbox("Show FOV", &Options::Aimbot::ShowFOV);
                            ImGui::Checkbox("Show FOV Fill", &Options::Aimbot::ShowFOVFill);
                            ImGui::Checkbox("Nearest Aim", &Options::Aimbot::NearestAim);
                            if (Options::Aimbot::NearestAim)
                            {
                                ImGui::Indent(14.0f);
                                ImGui::Checkbox("Head", &Options::Aimbot::NearestHead);
                                ImGui::Checkbox("Chest", &Options::Aimbot::NearestChest);
                                ImGui::Checkbox("Legs", &Options::Aimbot::NearestLegs);
                                ImGui::Unindent(14.0f);
                            }
                            ImGui::PopStyleColor(1);

                            ImGui::Dummy(ImVec2(0, 8));

                            // Curve graph (adapted)
                            float panelW = cw - 16;
                            ImVec2 graphSize = ImVec2(panelW, 90);
                            float offsetX = 0;

                            ImGui::Text("Smoothness Curve:");
                            ImGui::Dummy(ImVec2(0, 3));

                            ImVec2 graphPos = ImGui::GetCursorScreenPos();
                            auto gDraw = ImGui::GetWindowDrawList();

                            gDraw->AddRectFilled(graphPos, ImVec2(graphPos.x + graphSize.x, graphPos.y + graphSize.y), IM_COL32(10, 10, 14, 255), 4.0f);
                            gDraw->AddRect(graphPos, ImVec2(graphPos.x + graphSize.x, graphPos.y + graphSize.y), IM_COL32(28, 28, 33, 255), 4.0f);

                            for (int i = 1; i < 4; i++)
                            {
                                float y = graphPos.y + (graphSize.y / 4.0f) * i;
                                gDraw->AddLine(ImVec2(graphPos.x, y), ImVec2(graphPos.x + graphSize.x, y), IM_COL32(22, 22, 26, 255), 1.0f);
                            }
                            for (int i = 1; i < 4; i++)
                            {
                                float x = graphPos.x + (graphSize.x / 4.0f) * i;
                                gDraw->AddLine(ImVec2(x, graphPos.y), ImVec2(x, graphPos.y + graphSize.y), IM_COL32(22, 22, 26, 255), 1.0f);
                            }

                            ImVec2 prevPt = ImVec2(graphPos.x, graphPos.y + graphSize.y);
                            for (int i = 1; i <= 100; i++)
                            {
                                float t = i / 100.0f;
                                float val;
                                switch (Options::Aimbot::SmoothnessCurve)
                                {
                                    case 0: val = t; break;
                                    case 1: val = t * t; break;
                                    case 2: val = sqrt(t); break;
                                    case 3: val = t * t * (3.0f - 2.0f * t); break;
                                    case 4:
                                    {
                                        float p1 = Options::Aimbot::CustomCurveP1[1];
                                        float p2 = Options::Aimbot::CustomCurveP2[1];
                                        float u = 1.0f - t;
                                        val = u * u * u * 0 + 3 * u * u * t * p1 + 3 * u * t * t * p2 + t * t * t;
                                        break;
                                    }
                                    default: val = t;
                                }
                                ImVec2 pt(graphPos.x + t * graphSize.x, graphPos.y + graphSize.y - val * graphSize.y);
                                gDraw->AddLine(prevPt, pt, IM_COL32(main_color.x * 255, main_color.y * 255, main_color.z * 255, 255), 2.0f);
                                prevPt = pt;
                            }

                            if (Options::Aimbot::SmoothnessCurve == 4)
                            {
                                Options::Aimbot::CustomCurveEnabled = true;
                                ImVec2 cp1Pos(graphPos.x + Options::Aimbot::CustomCurveP1[0] * graphSize.x, graphPos.y + graphSize.y - Options::Aimbot::CustomCurveP1[1] * graphSize.y);
                                ImVec2 cp2Pos(graphPos.x + Options::Aimbot::CustomCurveP2[0] * graphSize.x, graphPos.y + graphSize.y - Options::Aimbot::CustomCurveP2[1] * graphSize.y);
                                gDraw->AddLine(ImVec2(graphPos.x, graphPos.y + graphSize.y), cp1Pos, IM_COL32(100, 100, 110, 150), 1.0f);
                                gDraw->AddLine(cp2Pos, ImVec2(graphPos.x + graphSize.x, graphPos.y), IM_COL32(100, 100, 110, 150), 1.0f);
                                float cpr = 5.0f;
                                gDraw->AddCircleFilled(cp1Pos, cpr, IM_COL32(main_color.x * 255, main_color.y * 255, main_color.z * 255, 255));
                                gDraw->AddCircle(cp1Pos, cpr, IM_COL32(255, 255, 255, 255), 0, 1.5f);
                                gDraw->AddCircleFilled(cp2Pos, cpr, IM_COL32(main_color.x * 255, main_color.y * 255, main_color.z * 255, 255));
                                gDraw->AddCircle(cp2Pos, cpr, IM_COL32(255, 255, 255, 255), 0, 1.5f);
                                ImVec2 mp = ImGui::GetMousePos();
                                static int draggedPt = -1;
                                if (ImGui::IsMouseDown(0))
                                {
                                    if (draggedPt == -1)
                                    {
                                        float d1 = sqrtf((mp.x - cp1Pos.x) * (mp.x - cp1Pos.x) + (mp.y - cp1Pos.y) * (mp.y - cp1Pos.y));
                                        if (d1 <= cpr + 3) draggedPt = 0;
                                        float d2 = sqrtf((mp.x - cp2Pos.x) * (mp.x - cp2Pos.x) + (mp.y - cp2Pos.y) * (mp.y - cp2Pos.y));
                                        if (d2 <= cpr + 3) draggedPt = 1;
                                    }
                                    if (draggedPt == 0)
                                    {
                                        Options::Aimbot::CustomCurveP1[0] = std::clamp((mp.x - graphPos.x) / graphSize.x, 0.0f, 1.0f);
                                        Options::Aimbot::CustomCurveP1[1] = std::clamp((graphPos.y + graphSize.y - mp.y) / graphSize.y, 0.0f, 1.0f);
                                    }
                                    else if (draggedPt == 1)
                                    {
                                        Options::Aimbot::CustomCurveP2[0] = std::clamp((mp.x - graphPos.x) / graphSize.x, 0.0f, 1.0f);
                                        Options::Aimbot::CustomCurveP2[1] = std::clamp((graphPos.y + graphSize.y - mp.y) / graphSize.y, 0.0f, 1.0f);
                                    }
                                }
                                else { draggedPt = -1; }
                            }
                            else { Options::Aimbot::CustomCurveEnabled = false; }

                            gDraw->AddText(ImVec2(graphPos.x + 2, graphPos.y + graphSize.y + 2), IM_COL32(120, 120, 130, 200), "0.0");
                            gDraw->AddText(ImVec2(graphPos.x + graphSize.x - 18, graphPos.y + graphSize.y + 2), IM_COL32(120, 120, 130, 200), "1.0");
                            ImGui::Dummy(ImVec2(graphSize.x, graphSize.y + 12));
                        }
                        endStyledChild();

                        ImGui::SameLine(0, 8);

                        beginStyledChild("##aimMod", ImVec2(cw, contentH - 58));
                        {
                            static const char* aimingMethods[]{ "Camera", "Mouse" };
                            ImGui::Combo("Method", &Options::Aimbot::AimingType, aimingMethods, IM_ARRAYSIZE(aimingMethods));
                            static const char* hitParts[]{ "Head", "Torso", "Left Arm", "Right Arm", "Left Leg", "Right Leg" };
                            ImGui::Combo("Hit Part", &Options::Aimbot::TargetBone, hitParts, IM_ARRAYSIZE(hitParts));
                            ImGui::Combo("Air Hit Part", &Options::Aimbot::AirTargetBone, hitParts, IM_ARRAYSIZE(hitParts));
                            static const char* smoothnessCurves[]{ "Linear", "Ease In", "Ease Out", "Ease In-Out", "Custom" };
                            ImGui::Combo("Curve", &Options::Aimbot::SmoothnessCurve, smoothnessCurves, IM_ARRAYSIZE(smoothnessCurves));

                            ImGui::PushStyleColor(ImGuiCol_SliderGrab, main_color);
                            ImGui::SliderFloat("Smoothness", &Options::Aimbot::Smoothness, 0.f, 1.f, "%.3f");
                            if (Options::Aimbot::Shake)
                                ImGui::SliderFloat("Shake Intensity", &Options::Aimbot::ShakeIntensity, 0.1f, 10.0f, "%.1f");
                            if (Options::Aimbot::Stutter)
                                ImGui::SliderInt("Stutter Ticks", &Options::Aimbot::StutterTicks, 1, 20);
                            ImGui::SliderFloat("Range", &Options::Aimbot::Range, 1.f, 1000.f, "%.0f");
                            ImGui::SliderFloat("FOV", &Options::Aimbot::FOV, 10.f, 360.f, "%.0f");
                            ImGui::SliderFloat("FOV Thickness", &Options::Aimbot::FOVThickness, 1.0f, 10.0f, "%.1f");
                            if (Options::Aimbot::Prediction)
                            {
                                ImGui::SliderFloat("Prediction X", &Options::Aimbot::PredictionX, 0.1f, 10.0f, "%.1f");
                                ImGui::SliderFloat("Prediction Y", &Options::Aimbot::PredictionY, 0.1f, 10.0f, "%.1f");
                            }
                            ImGui::PopStyleColor(1);

                            ImGui::Dummy(ImVec2(0, 8));

                            float modPanelW = cw - 16;
                            ImDrawList* kd = ImGui::GetWindowDrawList();
                            ImVec2 kp = ImGui::GetCursorScreenPos();
                            kd->AddRectFilled(ImVec2(kp.x, kp.y), ImVec2(kp.x + modPanelW, kp.y + 1), ImColor(main_color));
                            ImGui::Dummy(ImVec2(0, 4));

                            ImGui::PushStyleColor(ImGuiCol_Text, main_color);
                            KeybindSelector(" Aimbot Key", &Options::Aimbot::AimbotKey);
                            ImGui::PopStyleColor(1);
                        }
                        endStyledChild();
                    }
                    else if (tab2 == 1)
                    {
                        beginStyledChild("##trigMain", ImVec2(cw, contentH - 58));
                        {
                            ImGui::PushStyleColor(ImGuiCol_CheckMark, main_color);
                            ImGui::Checkbox("Enabled", &Options::Triggerbot::Enabled);
                            ImGui::Checkbox("Team Check", &Options::Triggerbot::TeamCheck);
                            ImGui::Checkbox("Knocked Check", &Options::Triggerbot::DownedCheck);
                            ImGui::Checkbox("Advanced FOV", &Options::Triggerbot::AdvancedFOV);
                            if (Options::Triggerbot::AdvancedFOV)
                                ImGui::Checkbox("Show FOV", &Options::Triggerbot::ShowAdvancedFOV);
                            ImGui::PopStyleColor(1);

                            ImGui::Dummy(ImVec2(0, 8));

                            ImDrawList* tbd = ImGui::GetWindowDrawList();
                            ImVec2 tbp = ImGui::GetCursorScreenPos();
                            tbd->AddRectFilled(ImVec2(tbp.x, tbp.y), ImVec2(tbp.x + cw - 16, tbp.y + 1), ImColor(main_color));
                            ImGui::Dummy(ImVec2(0, 4));

                            ImGui::PushStyleColor(ImGuiCol_Text, main_color);
                            KeybindSelector(" Triggerbot Key", &Options::Triggerbot::TriggerbotKey);
                            ImGui::PopStyleColor(1);
                        }
                        endStyledChild();

                        ImGui::SameLine(0, 8);

                        beginStyledChild("##trigSet", ImVec2(cw, contentH - 58));
                        {
                            ImGui::PushStyleColor(ImGuiCol_SliderGrab, main_color);
                            if (!Options::Triggerbot::AdvancedFOV)
                                ImGui::SliderFloat("Radius", &Options::Triggerbot::Radius, 5.f, 50.f, "%.0f");
                            ImGui::SliderFloat("Range", &Options::Triggerbot::Range, 1.f, 1000.f, "%.0f");
                            ImGui::SliderInt("Delay (ms)", &Options::Triggerbot::Delay, 0, 500);

                            if (Options::Triggerbot::AdvancedFOV)
                            {
                                ImGui::Text(" HEAD");
                                ImGui::SliderFloat("Head FOV X", &Options::Triggerbot::HeadFOV_X, 0.f, 100.f, "%.1f");
                                ImGui::SliderFloat("Head FOV Y", &Options::Triggerbot::HeadFOV_Y, 0.f, 100.f, "%.1f");
                                ImGui::Text(" TORSO");
                                ImGui::SliderFloat("Torso FOV X", &Options::Triggerbot::TorsoFOV_X, 0.f, 100.f, "%.1f");
                                ImGui::SliderFloat("Torso FOV Y", &Options::Triggerbot::TorsoFOV_Y, 0.f, 100.f, "%.1f");
                                ImGui::Text(" UPPER TORSO");
                                ImGui::SliderFloat("U Torso FOV X", &Options::Triggerbot::UpperTorsoFOV_X, 0.f, 100.f, "%.1f");
                                ImGui::SliderFloat("U Torso FOV Y", &Options::Triggerbot::UpperTorsoFOV_Y, 0.f, 100.f, "%.1f");
                                ImGui::Text(" LOWER TORSO");
                                ImGui::SliderFloat("L Torso FOV X", &Options::Triggerbot::LowerTorsoFOV_X, 0.f, 100.f, "%.1f");
                                ImGui::SliderFloat("L Torso FOV Y", &Options::Triggerbot::LowerTorsoFOV_Y, 0.f, 100.f, "%.1f");
                                ImGui::Text(" LEFT ARM");
                                ImGui::SliderFloat("L U Arm FOV X", &Options::Triggerbot::LeftUpperArmFOV_X, 0.f, 100.f, "%.1f");
                                ImGui::SliderFloat("L U Arm FOV Y", &Options::Triggerbot::LeftUpperArmFOV_Y, 0.f, 100.f, "%.1f");
                                ImGui::SliderFloat("L L Arm FOV X", &Options::Triggerbot::LeftLowerArmFOV_X, 0.f, 100.f, "%.1f");
                                ImGui::SliderFloat("L L Arm FOV Y", &Options::Triggerbot::LeftLowerArmFOV_Y, 0.f, 100.f, "%.1f");
                                ImGui::SliderFloat("L Hand FOV X", &Options::Triggerbot::LeftHandFOV_X, 0.f, 100.f, "%.1f");
                                ImGui::SliderFloat("L Hand FOV Y", &Options::Triggerbot::LeftHandFOV_Y, 0.f, 100.f, "%.1f");
                                ImGui::Text(" RIGHT ARM");
                                ImGui::SliderFloat("R U Arm FOV X", &Options::Triggerbot::RightUpperArmFOV_X, 0.f, 100.f, "%.1f");
                                ImGui::SliderFloat("R U Arm FOV Y", &Options::Triggerbot::RightUpperArmFOV_Y, 0.f, 100.f, "%.1f");
                                ImGui::SliderFloat("R L Arm FOV X", &Options::Triggerbot::RightLowerArmFOV_X, 0.f, 100.f, "%.1f");
                                ImGui::SliderFloat("R L Arm FOV Y", &Options::Triggerbot::RightLowerArmFOV_Y, 0.f, 100.f, "%.1f");
                                ImGui::SliderFloat("R Hand FOV X", &Options::Triggerbot::RightHandFOV_X, 0.f, 100.f, "%.1f");
                                ImGui::SliderFloat("R Hand FOV Y", &Options::Triggerbot::RightHandFOV_Y, 0.f, 100.f, "%.1f");
                                ImGui::Text(" LEFT LEG");
                                ImGui::SliderFloat("L U Leg FOV X", &Options::Triggerbot::LeftUpperLegFOV_X, 0.f, 100.f, "%.1f");
                                ImGui::SliderFloat("L U Leg FOV Y", &Options::Triggerbot::LeftUpperLegFOV_Y, 0.f, 100.f, "%.1f");
                                ImGui::SliderFloat("L L Leg FOV X", &Options::Triggerbot::LeftLowerLegFOV_X, 0.f, 100.f, "%.1f");
                                ImGui::SliderFloat("L L Leg FOV Y", &Options::Triggerbot::LeftLowerLegFOV_Y, 0.f, 100.f, "%.1f");
                                ImGui::SliderFloat("L Foot FOV X", &Options::Triggerbot::LeftFootFOV_X, 0.f, 100.f, "%.1f");
                                ImGui::SliderFloat("L Foot FOV Y", &Options::Triggerbot::LeftFootFOV_Y, 0.f, 100.f, "%.1f");
                                ImGui::Text(" RIGHT LEG");
                                ImGui::SliderFloat("R U Leg FOV X", &Options::Triggerbot::RightUpperLegFOV_X, 0.f, 100.f, "%.1f");
                                ImGui::SliderFloat("R U Leg FOV Y", &Options::Triggerbot::RightUpperLegFOV_Y, 0.f, 100.f, "%.1f");
                                ImGui::SliderFloat("R L Leg FOV X", &Options::Triggerbot::RightLowerLegFOV_X, 0.f, 100.f, "%.1f");
                                ImGui::SliderFloat("R L Leg FOV Y", &Options::Triggerbot::RightLowerLegFOV_Y, 0.f, 100.f, "%.1f");
                                ImGui::SliderFloat("R Foot FOV X", &Options::Triggerbot::RightFootFOV_X, 0.f, 100.f, "%.1f");
                                ImGui::SliderFloat("R Foot FOV Y", &Options::Triggerbot::RightFootFOV_Y, 0.f, 100.f, "%.1f");
                            }
                            ImGui::PopStyleColor(1);
                        }
                        endStyledChild();
                    }
                    else if (tab2 == 2)
                    {
                        beginStyledChild("##hitMain", ImVec2(cw, contentH - 58));
                        {
                            ImGui::PushStyleColor(ImGuiCol_CheckMark, main_color);
                            ImGui::Checkbox("Enabled", &Options::HitboxExpander::Enabled);
                            ImGui::Checkbox("Show Hitbox", &Options::HitboxExpander::ShowHitbox);
                            ImGui::Checkbox("Walk Through", &Options::HitboxExpander::WalkThrough);
                            ImGui::PopStyleColor(1);
                        }
                        endStyledChild();

                        ImGui::SameLine(0, 8);

                        beginStyledChild("##hitSet", ImVec2(cw, contentH - 58));
                        {
                            ImGui::PushStyleColor(ImGuiCol_SliderGrab, main_color);
                            ImGui::SliderFloat("Horizontal Size", &Options::HitboxExpander::HorizontalSize, 1.0f, 50.0f, "%.1f");
                            ImGui::SliderFloat("Vertical Size", &Options::HitboxExpander::VerticalSize, 1.0f, 50.0f, "%.1f");
                            ImGui::SliderFloat("Transparency", &Options::HitboxExpander::HitboxTransparency, 0.0f, 1.0f, "%.2f");
                            ImGui::PopStyleColor(1);
                        }
                        endStyledChild();
                    }
                }
                else if (tab == 1)
                {
                    renderSubtabBar(visSubtabNames, 2);
                    ImGui::Dummy(ImVec2(0, 6));

                    if (tab2 == 0)
                    {
                        beginStyledChild("##espMain", ImVec2(cw, contentH - 58));
                        {
                            ImGui::PushStyleColor(ImGuiCol_CheckMark, main_color);
                            CheckboxWithColorPicker("Names", &Options::ESP::Name, Options::ESP::Color);
                            CheckboxWithColorPicker("Distance", &Options::ESP::Distance, Options::ESP::DistanceColor);
                            ImGui::Checkbox("Health", &Options::ESP::Health);
                            CheckboxWithColorPicker("Tracers", &Options::ESP::Tracers, Options::ESP::TracerColor);
                            CheckboxWithColorPicker("Skeleton", &Options::ESP::Skeleton, Options::ESP::SkeletonColor);
                            CheckboxWithColorPicker("Offscreen Arrows", &Options::ESP::OffscreenArrows, Options::ESP::OffscreenArrowColor);
                            ImGui::Checkbox("Team Check", &Options::ESP::TeamCheck);
                            ImGui::PopStyleColor(1);
                        }
                        endStyledChild();

                        ImGui::SameLine(0, 8);

                        beginStyledChild("##espSet", ImVec2(cw, contentH - 58));
                        {
                            static const char* boxTypes[]{ "None", "Normal Box", "3D Box" };
                            ImGui::Combo("Box", &Options::ESP::BoxType, boxTypes, IM_ARRAYSIZE(boxTypes));
                            ImGui::PushStyleColor(ImGuiCol_CheckMark, main_color);
                            ImGui::Checkbox("Outline", &Options::ESP::OutlineEnabled);
                            ImGui::PopStyleColor(1);
                            ImGui::PushStyleColor(ImGuiCol_SliderGrab, main_color);
                            ImGui::SliderFloat("Box Thickness", &Options::ESP::BoxThickness, 1.0f, 10.0f);
                            ImGui::SliderFloat("Tracer Thickness", &Options::ESP::TracerThickness, 1.0f, 10.0f);
                            ImGui::SliderFloat("3D ESP Thickness", &Options::ESP::ESP3DThickness, 1.0f, 10.0f);
                            ImGui::SliderFloat("Skeleton Thickness", &Options::ESP::SkeletonThickness, 1.0f, 10.0f);
                            ImGui::PopStyleColor(1);
                        }
                        endStyledChild();
                    }
                    else if (tab2 == 1)
                    {
                        beginStyledChild("##espCol", ImVec2(cw, contentH - 58));
                        {
                            ImGui::ColorEdit3("Box Color", Options::ESP::BoxColor, ImGuiColorEditFlags_NoInputs);
                            ImGui::ColorEdit3("3D Box Color", Options::ESP::ESP3DColor, ImGuiColorEditFlags_NoInputs);
                            ImGui::ColorEdit3("Name Color", Options::ESP::Color, ImGuiColorEditFlags_NoInputs);
                            ImGui::ColorEdit3("Distance Color", Options::ESP::DistanceColor, ImGuiColorEditFlags_NoInputs);
                            ImGui::ColorEdit3("Tracer Color", Options::ESP::TracerColor, ImGuiColorEditFlags_NoInputs);
                            ImGui::ColorEdit3("Chams Color", Options::ESP::ChamsColor, ImGuiColorEditFlags_NoInputs);
                        }
                        endStyledChild();

                        ImGui::SameLine(0, 8);

                        beginStyledChild("##fovCol", ImVec2(cw, contentH - 58));
                        {
                            ImGui::ColorEdit3("FOV Color", Options::Aimbot::FOVColor, ImGuiColorEditFlags_NoInputs);
                            if (ImGui::ColorEdit3("Menu Accent", Options::Misc::MenuAccentColor, ImGuiColorEditFlags_NoInputs))
                            {
                                main_color = ImVec4(Options::Misc::MenuAccentColor[0], Options::Misc::MenuAccentColor[1], Options::Misc::MenuAccentColor[2], 1.0f);
                            }
                            ImGui::ColorEdit4("FOV Fill Color", Options::Aimbot::FOVFillColor, ImGuiColorEditFlags_NoInputs);
                        }
                        endStyledChild();
                    }
                }
                else if (tab == 2)
                {
                    renderSubtabBar(moveSubtabNames, 3);
                    ImGui::Dummy(ImVec2(0, 6));

                    if (tab2 == 0)
                    {
                        beginStyledChild("##flyMain", ImVec2(cw, contentH - 58));
                        {
                            ImGui::PushStyleColor(ImGuiCol_CheckMark, main_color);
                            ImGui::Checkbox("Enabled", &Options::Fly::Enabled);
                            ImGui::Checkbox("Hold Key", &Options::Fly::HoldKey);
                            ImGui::PopStyleColor(1);
                        }
                        endStyledChild();

                        ImGui::SameLine(0, 8);

                        beginStyledChild("##flySet", ImVec2(cw, contentH - 58));
                        {
                            ImGui::PushStyleColor(ImGuiCol_SliderGrab, main_color);
                            ImGui::SliderFloat("Fly Speed", &Options::Fly::Speed, 10.f, 500.f, "%.0f");
                            ImGui::PopStyleColor(1);

                            ImGui::Dummy(ImVec2(0, 6));
                            ImGui::Text("Toggle Type:");
                            ImGui::SameLine();
                            const char* toggleTypes[] = { "Hold", "Toggle" };
                            ImGui::Combo("##flyToggle", &Options::Fly::ToggleType, toggleTypes, 2);
                            KeybindSelector(" Fly Key", &Options::Fly::FlyKey);
                        }
                        endStyledChild();
                    }
                    else if (tab2 == 1)
                    {
                        beginStyledChild("##wsMain", ImVec2(cw, contentH - 58));
                        {
                            ImGui::PushStyleColor(ImGuiCol_CheckMark, main_color);
                            ImGui::Checkbox("Enabled", &Options::WalkSpeed::Enabled);
                            ImGui::PopStyleColor(1);
                        }
                        endStyledChild();

                        ImGui::SameLine(0, 8);

                        beginStyledChild("##wsSet", ImVec2(cw, contentH - 58));
                        {
                            ImGui::PushStyleColor(ImGuiCol_SliderGrab, main_color);
                            ImGui::SliderFloat("Walk Speed", &Options::WalkSpeed::Speed, 16.f, 1000.f, "%.0f");
                            ImGui::PopStyleColor(1);

                            ImGui::Dummy(ImVec2(0, 6));
                            KeybindSelector(" WalkSpeed Key", &Options::WalkSpeed::WalkSpeedKey);
                        }
                        endStyledChild();
                    }
                    else if (tab2 == 2)
                    {
                        beginStyledChild("##flingMain", ImVec2(cw, contentH - 58));
                        {
                            ImGui::PushStyleColor(ImGuiCol_CheckMark, main_color);
                            ImGui::Checkbox("Enabled", &Options::Fling::Enabled);
                            ImGui::Checkbox("Hold Key", &Options::Fling::HoldKey);
                            ImGui::Checkbox("Anti-Fling", &Options::AntiFling::Enabled);
                            ImGui::PopStyleColor(1);
                        }
                        endStyledChild();

                        ImGui::SameLine(0, 8);

                        beginStyledChild("##flingSet", ImVec2(cw, contentH - 58));
                        {
                            ImGui::PushStyleColor(ImGuiCol_SliderGrab, main_color);
                            ImGui::SliderFloat("Fling Speed", &Options::Fling::Speed, 100.f, 20000.f, "%.0f");
                            ImGui::PopStyleColor(1);

                            ImGui::Dummy(ImVec2(0, 6));
                            const char* toggleTypes[] = { "Hold", "Toggle" };
                            ImGui::Combo("Toggle Type", &Options::Fling::ToggleType, toggleTypes, 2);
                            KeybindSelector(" Fling Key", &Options::Fling::FlingKey);

                            ImGui::Dummy(ImVec2(0, 10));
                            ImGui::Separator();
                            ImGui::Text("Anti-Fling Mode:");
                            const char* afModes[] = { "Disable Collision", "Stop Velocity" };
                            ImGui::Combo("##afMode", &Options::AntiFling::Mode, afModes, 2);
                        }
                        endStyledChild();
                    }
                }
                else if (tab == 3)
                {
                    renderSubtabBar(miscSubtabNames, 3);
                    ImGui::Dummy(ImVec2(0, 6));

                    if (tab2 == 0)
                    {
                        beginStyledChild("##locMain", ImVec2(cw, contentH - 58));
                        {
                            ImGui::PushStyleColor(ImGuiCol_CheckMark, main_color);
                            ImGui::Checkbox("Headless", &Options::ESP::Headless);
                            ImGui::Checkbox("Crosshair", &Options::Crosshair::Enabled);
                            ImGui::Checkbox("Camera FOV", &Options::Misc::FOVEnabled);
                            ImGui::Checkbox("Cache NPCs", &Options::Misc::CacheNPCs);
                            ImGui::Checkbox("Keybind List", &Options::Misc::KeybindList);
                            ImGui::Checkbox("Stream Proof", &Options::Misc::StreamProof);
                            ImGui::Checkbox("Show Console", &Options::Misc::ShowConsole);
                            ImGui::Checkbox("Dim Background", &Options::Misc::DimBackground);
                            ImGui::PopStyleColor(1);
                        }
                        endStyledChild();

                        ImGui::SameLine(0, 8);

                        beginStyledChild("##locSet", ImVec2(cw, contentH - 58));
                        {
                            ImGui::PushStyleColor(ImGuiCol_SliderGrab, main_color);
                            if (Options::Misc::FOVEnabled)
                                ImGui::SliderFloat("Camera FOV", &Options::Misc::FOV, 70.f, 120.f, "%.0f");

                            ImGui::Dummy(ImVec2(0, 6));
                            ImGui::Text("Keybind List Position:");
                            ImGui::SliderFloat("Position X", &Options::Misc::KeybindListX, 0.0f, 1920.f, "%.0f");
                            ImGui::SliderFloat("Position Y", &Options::Misc::KeybindListY, 0.0f, 1080.f, "%.0f");
                            ImGui::PopStyleColor(1);

                            ImGui::Dummy(ImVec2(0, 10));
                            ImGui::Separator();
                            ImGui::Text("Teleport");
                            ImGui::Checkbox("Ctrl+Click TP", &Options::Teleport::CtrlClickTP);
                            ImGui::Checkbox("TP to Players", &Options::Teleport::TPToPlayers);
                            if (Options::Teleport::TPToPlayers)
                            {
                                auto& players = Globals::Caches::CachedPlayerObjects;
                                static std::vector<std::string> tpNames;
                                static std::vector<int> tpNameToPlayer;
                                tpNames.clear();
                                tpNameToPlayer.clear();
                                int displayIdx = 0;
                                for (int i = 0; i < (int)players.size(); i++)
                                {
                                    if (players[i].address != Globals::Roblox::LocalPlayer.address)
                                    {
                                        if (i == Options::Teleport::SelectedPlayer)
                                            displayIdx = (int)tpNames.size();
                                        tpNames.push_back(players[i].Name);
                                        tpNameToPlayer.push_back(i);
                                    }
                                }
                                if (tpNames.empty())
                                {
                                    tpNames.push_back("No players");
                                    tpNameToPlayer.push_back(-1);
                                }
                                ImGui::Combo("##tpPlayer", &displayIdx,
                                    [](void* data, int idx, const char** out) -> bool {
                                        auto& names = *(std::vector<std::string>*)data;
                                        if (idx < 0 || idx >= (int)names.size()) return false;
                                        *out = names[idx].c_str();
                                        return true;
                                    }, &tpNames, (int)tpNames.size());
                                if (displayIdx >= 0 && displayIdx < (int)tpNameToPlayer.size())
                                    Options::Teleport::SelectedPlayer = tpNameToPlayer[displayIdx];
                            }
                        }
                        endStyledChild();
                    }
                    else if (tab2 == 1)
                    {
                        beginStyledChild("##silentMain", ImVec2(cw, contentH - 58));
                        {
                            ImGui::PushStyleColor(ImGuiCol_CheckMark, main_color);
                            ImGui::Checkbox("Enabled", &Options::SilentAim::Enabled);
                            ImGui::Checkbox("Team Check", &Options::SilentAim::TeamCheck);
                            ImGui::Checkbox("Downed Check", &Options::SilentAim::DownedCheck);
                            ImGui::Checkbox("Prediction", &Options::SilentAim::Prediction);
                            ImGui::Checkbox("Hitbox on Fire", &Options::SilentAim::HitboxOnFire);
                            ImGui::PopStyleColor(1);
                        }
                        endStyledChild();

                        ImGui::SameLine(0, 8);

                        beginStyledChild("##silentSet", ImVec2(cw, contentH - 58));
                        {
                            ImGui::PushStyleColor(ImGuiCol_SliderGrab, main_color);
                            ImGui::SliderFloat("FOV", &Options::SilentAim::FOV, 1.f, 500.f, "%.0f");
                            ImGui::SliderFloat("Range", &Options::SilentAim::Range, 10.f, 1000.f, "%.0f");
                            if (Options::SilentAim::Prediction)
                            {
                                ImGui::SliderFloat("Prediction X", &Options::SilentAim::PredictionX, 0.1f, 10.f, "%.1f");
                                ImGui::SliderFloat("Prediction Y", &Options::SilentAim::PredictionY, 0.1f, 10.f, "%.1f");
                            }
                            if (Options::SilentAim::HitboxOnFire)
                            {
                                ImGui::SliderFloat("Hitbox Mult", &Options::SilentAim::HitboxMult, 1.f, 20.f, "%.1f");
                                ImGui::SliderInt("Hitbox Frames", &Options::SilentAim::HitboxFrames, 1, 10);
                            }
                            ImGui::PopStyleColor(1);

                            ImGui::Dummy(ImVec2(0, 6));
                            const char* aimMethods[] = { "Mouse Hit (Stable)", "Unit Ray (Unstable)" };
                            ImGui::Combo("Method", &Options::SilentAim::Method, aimMethods, 2);
                            KeybindSelector(" Silent Key", &Options::SilentAim::Key);
                        }
                        endStyledChild();
                    }
                    else if (tab2 == 2)
                    {
                        static char configName[64] = "default";
                        static std::vector<std::string> configList;
                        static int selectedConfig = -1;
                        static float lastConfigScan = 0;

                        if (ImGui::GetTime() - lastConfigScan > 2.0f)
                        {
                            configList.clear();
                            try {
                                for (const auto& entry : std::filesystem::directory_iterator(Globals::configsPath))
                                {
                                    if (entry.path().extension() == ".json")
                                        configList.push_back(entry.path().filename().string());
                                }
                            } catch (...) {}
                            lastConfigScan = (float)ImGui::GetTime();
                        }

                        beginStyledChild("##cfgCtrl", ImVec2(cw, contentH - 58));
                        {
                            ImGui::PushStyleColor(ImGuiCol_CheckMark, main_color);
                            ImGui::Text("Save / Load");
                            ImGui::Separator();
                            ImGui::InputText("##configname", configName, sizeof(configName));
                            if (ImGui::Button("Save Config", ImVec2(cw - 24, 30)))
                            {
                                std::string filename(configName);
                                if (!filename.empty())
                                {
                                    if (filename.find(".json") == std::string::npos)
                                        filename += ".json";
                                    CreateConfig(filename);
                                }
                            }
                            ImGui::Dummy(ImVec2(0, 4));
                            if (ImGui::Button("Refresh List", ImVec2(cw - 24, 25)))
                            {
                                configList.clear();
                                try {
                                    for (const auto& entry : std::filesystem::directory_iterator(Globals::configsPath))
                                    {
                                        if (entry.path().extension() == ".json")
                                            configList.push_back(entry.path().filename().string());
                                    }
                                } catch (...) {}
                            }
                            ImGui::Dummy(ImVec2(0, 4));
                            ImGui::Text("Auto-save every 30s");

                            ImGui::Dummy(ImVec2(0, 8));
                            ImGui::Separator();
                            ImGui::Text("Discord Webhook");
                            ImGui::Checkbox("Enabled", &Options::Misc::WebhookEnabled);
                            static char webhookBuf[512] = "";
                            if (webhookBuf[0] == '\0' && !Options::Misc::WebhookURL.empty())
                                strcpy_s(webhookBuf, Options::Misc::WebhookURL.c_str());
                            ImGui::InputText("##webhookurl", webhookBuf, sizeof(webhookBuf));
                            if (ImGui::Button("Set Webhook", ImVec2(cw - 24, 25)))
                                Options::Misc::WebhookURL = webhookBuf;
                            if (ImGui::Button("Test Webhook", ImVec2(cw - 24, 25)))
                                SendWebhookAsync("Webhook test");
                            ImGui::PopStyleColor(1);
                        }
                        endStyledChild();

                        ImGui::SameLine(0, 8);

                        beginStyledChild("##cfgList", ImVec2(cw, contentH - 58));
                        {
                            ImGui::PushStyleColor(ImGuiCol_CheckMark, main_color);
                            if (configList.empty())
                            {
                                ImGui::Text("No configs found.");
                            }
                            else
                            {
                                for (int i = 0; i < configList.size(); i++)
                                {
                                    if (ImGui::Selectable(configList[i].c_str(), selectedConfig == i))
                                        selectedConfig = i;
                                }
                            }
                            ImGui::Dummy(ImVec2(0, 8));
                            if (selectedConfig >= 0 && selectedConfig < configList.size())
                            {
                                if (ImGui::Button("Load Selected", ImVec2(cw - 24, 30)))
                                    LoadConfig(configList[selectedConfig]);
                                ImGui::Dummy(ImVec2(0, 4));
                                if (ImGui::Button("Overwrite Selected", ImVec2(cw - 24, 30)))
                                    CreateConfig(configList[selectedConfig]);
                            }
                            ImGui::PopStyleColor(1);
                        }
                        endStyledChild();
                    }
                    }
                    }
                    ImGui::EndChild();

                // ============ FOOTER ============
                float footerY = p.y + s.y - 22;
                draw->AddLine(ImVec2(p.x + sbWidth + 12, footerY - 4), ImVec2(p.x + s.x - 6, footerY - 4), IM_COL32(28, 28, 33, 255));

                std::string fpsRight = std::to_string(static_cast<int>(io.Framerate)) + " FPS";
                ImVec2 fpsSize = font->CalcTextSizeA(13.0f, FLT_MAX, 0.f, fpsRight.c_str());
                draw->AddText(ImVec2(p.x + s.x - fpsSize.x - 10, footerY + 2), IM_COL32(100, 100, 110, 200), fpsRight.c_str());

                ImGui::PopFont();
            }
            ImGui::PopStyleVar();
            ImGui::End();
        }

        if (IsGameOnTop("Roblox"))
        {
            RenderESP(ImGui::GetBackgroundDrawList(), menu_open);

            if (!menu_open)
            {
                RunAimbot(ImGui::GetBackgroundDrawList());
                RunTriggerbot();
                RunMacro();

                // Ctrl+Click Teleport
                if (Options::Teleport::CtrlClickTP && (GetAsyncKeyState(VK_CONTROL) & 0x8000))
                {
                    static bool wasLMB = false;
                    bool lmb = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
                    if (lmb && !wasLMB)
                    {
                        POINT cursor;
                        GetCursorPos(&cursor);
                        float cx = static_cast<float>(cursor.x);
                        float cy = static_cast<float>(cursor.y);

                        auto localChar = Globals::Roblox::LocalPlayer.Character();
                        for (auto& player : Globals::Caches::CachedPlayerObjects)
                        {
                            if (player.address == Globals::Roblox::LocalPlayer.address) continue;
                            auto part = player.HumanoidRootPart;
                            if (!part.address) continue;

                            auto w2s = WorldToScreen(part.Position());
                            if (w2s.x < 0 || w2s.y < 0) continue;
                            float dx = w2s.x - cx;
                            float dy = w2s.y - cy;
                            if (dx * dx + dy * dy < 50 * 50)
                            {
                                auto hrp = localChar.FindFirstChild("HumanoidRootPart");
                                if (hrp.address)
                                {
                                    Vectors::Vector3 targetPos = part.Position();
                                    targetPos.y += 5;
                                    Memory->write<Vectors::Vector3>(hrp.address + Offsets::BasePart::Position, targetPos);
                                }
                                break;
                            }
                        }
                    }
                    wasLMB = lmb;
                }

                // TP to players dropdown
                if (Options::Teleport::TPToPlayers && Options::Teleport::SelectedPlayer >= 0)
                {
                    int idx = Options::Teleport::SelectedPlayer;
                    auto& players = Globals::Caches::CachedPlayerObjects;
                    if (idx >= 0 && idx < (int)players.size())
                    {
                        auto target = players[idx];
                        if (target.address != Globals::Roblox::LocalPlayer.address && target.HumanoidRootPart.address)
                        {
                            auto localChar = Globals::Roblox::LocalPlayer.Character();
                            auto hrp = localChar.FindFirstChild("HumanoidRootPart");
                            if (hrp.address)
                            {
                                Vectors::Vector3 targetPos = target.HumanoidRootPart.Position();
                                targetPos.y += 5;
                                Memory->write<Vectors::Vector3>(hrp.address + Offsets::BasePart::Position, targetPos);
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
