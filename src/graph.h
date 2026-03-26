#pragma once
#include "renderer.h"
#include "ping_manager.h"
#include "layout.h"

namespace Graph {
    void Draw(Renderer& r, PingManager& pm, const LayoutRects& layout, float dpiScale);
    void DrawLegend(Renderer& r, PingManager& pm, const LayoutRects& layout, float dpiScale);
}
