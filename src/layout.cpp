#include "layout.h"
#include "theme.h"

LayoutRects ComputeLayout(float windowWidth, float windowHeight, float dpiScale) {
    LayoutRects L = {};
    L.windowWidth = windowWidth;
    L.windowHeight = windowHeight;

    float aspect = windowWidth / (windowHeight > 0 ? windowHeight : 1);
    L.isWideMode = aspect >= Theme::AspectThreshold;

    float sw = Theme::SidebarWidth * dpiScale;
    float hh = Theme::HeaderHeight * dpiScale;
    float sh = Theme::SeparatorHeight * dpiScale;
    float bh = Theme::BottomBtnHeight * dpiScale;
    float bm = Theme::BottomBtnMargin * dpiScale;
    float lh = Theme::LegendHeight * dpiScale;

    if (L.isWideMode) {
        L.sidebar = {0, 0, sw, windowHeight};
        L.mainArea = {sw, 0, windowWidth, windowHeight};
    } else {
        float split = windowHeight * 0.35f;
        L.sidebar = {0, windowHeight - split, windowWidth, windowHeight};
        L.mainArea = {0, 0, windowWidth, windowHeight - split};
    }

    // Header within sidebar
    L.header = {L.sidebar.left, L.sidebar.top, L.sidebar.right, L.sidebar.top + hh};
    L.headerSep = {L.sidebar.left, L.header.bottom, L.sidebar.right, L.header.bottom + sh};

    // Add button at bottom of sidebar
    L.addButton = {
        L.sidebar.left + 8 * dpiScale,
        L.sidebar.bottom - bm - bh,
        L.sidebar.right - 8 * dpiScale,
        L.sidebar.bottom - bm
    };

    // Target scroll area between separator and add button
    L.targetScroll = {
        L.sidebar.left,
        L.headerSep.bottom,
        L.sidebar.right - 1 * dpiScale, // leave room for border
        L.addButton.top - bm
    };

    // Legend at bottom of main area
    L.legendBar = {L.mainArea.left, L.mainArea.bottom - lh, L.mainArea.right, L.mainArea.bottom};

    // Graph area is main minus legend
    L.graphArea = {L.mainArea.left, L.mainArea.top, L.mainArea.right, L.legendBar.top};

    return L;
}
