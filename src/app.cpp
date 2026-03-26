#include "app.h"
#include "theme.h"
#include "graph.h"
#include "math_util.h"

App* App::g_app = nullptr;

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    return App::g_app->HandleMessage(hwnd, msg, wParam, lParam);
}

bool App::Initialize(HINSTANCE hInstance) {
    hInstance_ = hInstance;

    // Register window class
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = L"PingyWindow";
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    RegisterClassExW(&wc);

    // Create window
    hwnd_ = CreateWindowExW(
        0, L"PingyWindow", L"Pingy",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1920, 1080,
        nullptr, nullptr, hInstance, nullptr);

    if (!hwnd_) return false;

    // Get DPI before showing
    UINT dpi = GetDpiForWindow(hwnd_);
    dpiScale_ = dpi / 96.0f;

    ShowWindow(hwnd_, SW_SHOWMAXIMIZED);
    UpdateWindow(hwnd_);

    // Initialize renderer
    if (!renderer_.Initialize(hwnd_)) return false;
    renderer_.SetDpiScale(dpiScale_);

    // Initialize ping manager
    pingManager_.Initialize();
    settingsState_.timeoutValue = (float)pingManager_.Settings().timeoutMs;
    settingsState_.ttlValue = (float)pingManager_.Settings().ttl;

    // Compute initial layout
    RECT rc;
    GetClientRect(hwnd_, &rc);
    windowWidth_ = (float)(rc.right - rc.left);
    windowHeight_ = (float)(rc.bottom - rc.top);
    layout_ = ComputeLayout(windowWidth_, windowHeight_, dpiScale_);

    return true;
}

void App::Shutdown() {
    pingManager_.Shutdown();
    renderer_.Shutdown();
    WSACleanup();
}

void App::Run() {
    running_ = true;
    MSG msg = {};
    while (running_) {
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                running_ = false;
                break;
            }
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        if (!running_) break;

        Update();
        Render();
        Sleep(1); // ~1000fps cap. Your ping graph runs smoother than most video games.
    }
}

void App::Update() {
    pingManager_.Update();
}

void App::Render() {
    if (!renderer_.RT()) return;

    renderer_.BeginDraw();
    renderer_.Clear(Theme::Background);

    // Draw main area background
    renderer_.FillRect(layout_.mainArea, Theme::Background);

    // Draw graph
    Graph::Draw(renderer_, pingManager_, layout_, dpiScale_);
    Graph::DrawLegend(renderer_, pingManager_, layout_, dpiScale_);

    // Draw sidebar
    UISidebar::Draw(renderer_, pingManager_, layout_, sidebarState_, dpiScale_);

    // Draw settings panel (over sidebar and graph)
    UISettings::Draw(renderer_, layout_, settingsState_, pingManager_, dpiScale_);

    // Draw add dialog (over everything)
    UIAddDialog::Draw(renderer_, addDialogState_, windowWidth_, windowHeight_,
                      dpiScale_, GetTickCount64());

    if (!renderer_.EndDraw()) {
        renderer_.RecreateTarget(hwnd_);
    }
}

void App::OnSize(int width, int height) {
    windowWidth_ = (float)width;
    windowHeight_ = (float)height;
    if (width > 0 && height > 0) {
        renderer_.Resize(width, height);
        layout_ = ComputeLayout(windowWidth_, windowHeight_, dpiScale_);
    }
}

void App::OnMouseDown(float x, float y) {
    // Add dialog takes priority when visible
    if (addDialogState_.visible) {
        int result = UIAddDialog::HitTest(addDialogState_, renderer_, x, y,
                                           windowWidth_, windowHeight_, dpiScale_);
        if (result == 1) UIAddDialog::TryAdd(addDialogState_, pingManager_);
        else if (result == 2 || result == 3) addDialogState_.Hide();
        return;
    }

    // Settings panel
    if (settingsState_.visible) {
        auto panelR = UISettings::PanelRect(layout_, dpiScale_);
        if (PointInRect(x, y, panelR)) {
            int result = UISettings::HitTest(layout_, settingsState_, x, y, dpiScale_);
            if (result == 1) settingsState_.visible = false;
            else if (result == 2) {
                pingManager_.UpdateSettings((int)settingsState_.timeoutValue, (int)settingsState_.ttlValue);
            }
            UISettings::OnMouseDown(layout_, settingsState_, x, y, dpiScale_);
            return;
        }
    }

    // Sidebar
    if (PointInRect(x, y, layout_.sidebar)) {
        WStr removeHost;
        int result = UISidebar::HitTest(layout_, pingManager_, sidebarState_, x, y, dpiScale_, removeHost);
        if (result == 1) {
            settingsState_.visible = !settingsState_.visible;
            if (settingsState_.visible) {
                settingsState_.timeoutValue = (float)pingManager_.Settings().timeoutMs;
                settingsState_.ttlValue = (float)pingManager_.Settings().ttl;
            }
        }
        else if (result == 2) pingManager_.ToggleRunning();
        else if (result == 3) addDialogState_.Show();
        else if (result == 4 && !removeHost.empty()) pingManager_.RemoveTarget(removeHost);
    }
}

void App::OnMouseUp(float x, float y) {
    (void)x; (void)y;
    UISettings::OnMouseUp(settingsState_);
}

void App::OnMouseMove(float x, float y) {
    UISidebar::UpdateHover(layout_, pingManager_, sidebarState_, x, y, dpiScale_);

    if (settingsState_.visible) {
        UISettings::OnMouseMove(layout_, settingsState_, x, dpiScale_);
    }

    // Update add dialog hover states
    if (addDialogState_.visible) {
        auto panel = D2D1_RECT_F{};
        float dw = Theme::DialogWidth * dpiScale_;
        float dh = Theme::DialogHeight * dpiScale_;
        float cx = windowWidth_ / 2, cy = windowHeight_ / 2;
        panel = {cx - dw/2, cy - dh/2, cx + dw/2, cy + dh/2};

        // Compute button rects inline
        float bw = Theme::DialogBtnWidth * dpiScale_;
        float bh = Theme::DialogBtnHeight * dpiScale_;
        float bcx = (panel.left + panel.right) / 2;
        float by = panel.top + 220 * dpiScale_;
        D2D1_RECT_F cancelR = {bcx - 8*dpiScale_ - bw, by, bcx - 8*dpiScale_, by + bh};
        D2D1_RECT_F addR = {bcx + 8*dpiScale_, by, bcx + 8*dpiScale_ + bw, by + bh};
        addDialogState_.hoverAdd = PointInRect(x, y, addR);
        addDialogState_.hoverCancel = PointInRect(x, y, cancelR);
    }

    // Set cursor
    bool overButton = sidebarState_.hoverGear || sidebarState_.hoverStartStop ||
                      sidebarState_.hoverAddBtn || sidebarState_.hoverRemoveIndex >= 0 ||
                      addDialogState_.hoverAdd || addDialogState_.hoverCancel;
    SetCursor(LoadCursor(nullptr, overButton ? IDC_HAND : IDC_ARROW));
}

void App::OnMouseWheel(float x, float y, int delta) {
    if (PointInRect(x, y, layout_.targetScroll)) {
        UISidebar::Scroll(sidebarState_, delta, layout_,
                          (int)pingManager_.Targets().size(), dpiScale_);
    }
}

void App::OnChar(wchar_t ch) {
    if (addDialogState_.visible) {
        auto* input = UIAddDialog::FocusedInput(addDialogState_);
        if (input && ch >= 32) {
            input->InsertChar(ch);
        }
    }
}

void App::OnKeyDown(int vk) {
    if (addDialogState_.visible) {
        if (vk == VK_ESCAPE) {
            addDialogState_.Hide();
            return;
        }
        if (vk == VK_RETURN) {
            UIAddDialog::TryAdd(addDialogState_, pingManager_);
            return;
        }
        if (vk == VK_TAB) {
            UIAddDialog::TabFocus(addDialogState_);
            return;
        }

        auto* input = UIAddDialog::FocusedInput(addDialogState_);
        if (input) {
            if (vk == VK_BACK) input->Backspace();
            else if (vk == VK_DELETE) input->Delete();
            else if (vk == VK_LEFT) input->MoveCursor(-1);
            else if (vk == VK_RIGHT) input->MoveCursor(1);
            else if (vk == VK_HOME) input->Home();
            else if (vk == VK_END) input->End();
            else if (vk == 'V' && (GetKeyState(VK_CONTROL) & 0x8000)) input->PasteFromClipboard(hwnd_);
            else if (vk == 'A' && (GetKeyState(VK_CONTROL) & 0x8000)) input->SelectAll();
        }
        return;
    }

    if (vk == VK_ESCAPE && settingsState_.visible) {
        settingsState_.visible = false;
    }
}

void App::OnDpiChanged(UINT dpi, const RECT* suggested) {
    dpiScale_ = dpi / 96.0f;
    renderer_.SetDpiScale(dpiScale_);

    if (suggested) {
        SetWindowPos(hwnd_, nullptr,
            suggested->left, suggested->top,
            suggested->right - suggested->left,
            suggested->bottom - suggested->top,
            SWP_NOZORDER | SWP_NOACTIVATE);
    }

    // Recreate renderer resources for new DPI
    renderer_.Shutdown();
    renderer_.Initialize(hwnd_);
    renderer_.SetDpiScale(dpiScale_);
}

LRESULT App::HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_SIZE:
        OnSize(LOWORD(lParam), HIWORD(lParam));
        return 0;
    case WM_LBUTTONDOWN:
        OnMouseDown((float)LOWORD(lParam), (float)HIWORD(lParam));
        return 0;
    case WM_LBUTTONUP:
        OnMouseUp((float)LOWORD(lParam), (float)HIWORD(lParam));
        return 0;
    case WM_MOUSEMOVE:
        OnMouseMove((float)LOWORD(lParam), (float)HIWORD(lParam));
        return 0;
    case WM_MOUSEWHEEL: {
        POINT pt = {LOWORD(lParam), HIWORD(lParam)};
        ScreenToClient(hwnd, &pt);
        OnMouseWheel((float)pt.x, (float)pt.y, GET_WHEEL_DELTA_WPARAM(wParam));
        return 0;
    }
    case WM_CHAR:
        OnChar((wchar_t)wParam);
        return 0;
    case WM_KEYDOWN:
        OnKeyDown((int)wParam);
        return 0;
    case WM_DPICHANGED:
        OnDpiChanged(HIWORD(wParam), (RECT*)lParam);
        return 0;
    case WM_DESTROY:
        Shutdown();
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}
