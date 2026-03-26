#pragma once
#include "renderer.h"
#include "ping_manager.h"
#include "layout.h"

struct SettingsState {
    bool visible = false;
    float timeoutValue = 500;
    float ttlValue = 128;
    bool draggingTimeout = false;
    bool draggingTtl = false;
};

namespace UISettings {
    void Draw(Renderer& r, const LayoutRects& layout, SettingsState& state,
              PingManager& pm, float dpiScale);
    D2D1_RECT_F PanelRect(const LayoutRects& layout, float dpiScale);
    // Returns: 0=nothing, 1=close, 2=apply
    int HitTest(const LayoutRects& layout, SettingsState& state, float mx, float my, float dpiScale);
    void OnMouseDown(const LayoutRects& layout, SettingsState& state, float mx, float my, float dpiScale);
    void OnMouseMove(const LayoutRects& layout, SettingsState& state, float mx, float dpiScale);
    void OnMouseUp(SettingsState& state);
}
