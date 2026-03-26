#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <windows.h>
#include "renderer.h"
#include "ping_manager.h"
#include "layout.h"
#include "ui_sidebar.h"
#include "ui_settings.h"
#include "ui_add_dialog.h"

class App {
public:
    static App* g_app; // global pointer, allocated in _entry

    bool Initialize(HINSTANCE hInstance);
    void Shutdown();
    void Run();
    LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    void Update();
    void Render();

    void OnSize(int width, int height);
    void OnMouseDown(float x, float y);
    void OnMouseUp(float x, float y);
    void OnMouseMove(float x, float y);
    void OnMouseWheel(float x, float y, int delta);
    void OnChar(wchar_t ch);
    void OnKeyDown(int vk);
    void OnDpiChanged(UINT dpi, const RECT* suggested);

    HINSTANCE hInstance_ = nullptr;
    HWND hwnd_ = nullptr;
    Renderer renderer_;
    PingManager pingManager_;
    LayoutRects layout_ = {};

    SidebarState sidebarState_;
    SettingsState settingsState_;
    AddDialogState addDialogState_;

    float dpiScale_ = 1.0f;
    float windowWidth_ = 0;
    float windowHeight_ = 0;
    bool running_ = false;
};
