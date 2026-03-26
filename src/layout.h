#pragma once
#include <d2d1.h>

struct LayoutRects {
    D2D1_RECT_F sidebar;
    D2D1_RECT_F mainArea;
    D2D1_RECT_F header;
    D2D1_RECT_F headerSep;
    D2D1_RECT_F targetScroll;
    D2D1_RECT_F addButton;
    D2D1_RECT_F graphArea;
    D2D1_RECT_F legendBar;
    bool isWideMode;
    float windowWidth;
    float windowHeight;
};

LayoutRects ComputeLayout(float windowWidth, float windowHeight, float dpiScale);
