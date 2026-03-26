#pragma once
#include "renderer.h"
#include "ping_manager.h"
#include "layout.h"

struct SidebarState {
    float scrollOffset = 0.0f;
    float scrollTarget = 0.0f;
    int hoverItemIndex = -1; // -1 = none
    bool hoverGear = false;
    bool hoverStartStop = false;
    bool hoverAddBtn = false;
    int hoverRemoveIndex = -1;
};

namespace UISidebar {
    void Draw(Renderer& r, PingManager& pm, const LayoutRects& layout,
              SidebarState& state, float dpiScale);

    // Returns: 0=nothing, 1=gear clicked, 2=start/stop clicked, 3=add clicked
    // Sets removeHost if a remove button was clicked
    int HitTest(const LayoutRects& layout, PingManager& pm, SidebarState& state,
                float mx, float my, float dpiScale, WStr& removeHost);

    void UpdateHover(const LayoutRects& layout, PingManager& pm, SidebarState& state,
                     float mx, float my, float dpiScale);

    void Scroll(SidebarState& state, int delta, const LayoutRects& layout,
                int targetCount, float dpiScale);
}
