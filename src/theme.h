#pragma once
#include <d2d1.h>

namespace Theme {
    // Core palette
    constexpr D2D1_COLOR_F Background    = {0.04f, 0.04f, 0.04f, 1.0f};
    constexpr D2D1_COLOR_F PanelBg       = {0.07f, 0.07f, 0.07f, 1.0f};
    constexpr D2D1_COLOR_F GraphBg       = {0.067f, 0.067f, 0.067f, 1.0f};
    constexpr D2D1_COLOR_F Surface       = {0.12f, 0.12f, 0.12f, 1.0f};
    constexpr D2D1_COLOR_F SurfaceHover  = {0.16f, 0.16f, 0.16f, 1.0f};
    constexpr D2D1_COLOR_F Border        = {0.2f, 0.2f, 0.2f, 1.0f};
    constexpr D2D1_COLOR_F HeaderBg      = {0.06f, 0.06f, 0.06f, 1.0f};

    // Accents
    constexpr D2D1_COLOR_F PrimaryRed    = {1.0f, 0.09f, 0.267f, 1.0f};
    constexpr D2D1_COLOR_F SecondaryRed  = {1.0f, 0.322f, 0.322f, 1.0f};
    constexpr D2D1_COLOR_F DimRed        = {0.6f, 0.1f, 0.1f, 1.0f};

    // Text
    constexpr D2D1_COLOR_F TextPrimary   = {1.0f, 1.0f, 1.0f, 1.0f};
    constexpr D2D1_COLOR_F TextSecondary = {0.7f, 0.7f, 0.7f, 1.0f};
    constexpr D2D1_COLOR_F TextDim       = {0.4f, 0.4f, 0.4f, 1.0f};

    // Latency indicators
    constexpr D2D1_COLOR_F LatencyGood   = {0.3f, 0.9f, 0.4f, 1.0f};
    constexpr D2D1_COLOR_F LatencyMedium = {1.0f, 0.8f, 0.2f, 1.0f};
    constexpr D2D1_COLOR_F LatencyBad    = PrimaryRed;
    constexpr D2D1_COLOR_F LatencyTimeout= {0.15f, 0.15f, 0.15f, 1.0f};

    // Overlay colors
    constexpr D2D1_COLOR_F Overlay70     = {0.0f, 0.0f, 0.0f, 0.7f};
    constexpr D2D1_COLOR_F Overlay65     = {0.0f, 0.0f, 0.0f, 0.65f};
    constexpr D2D1_COLOR_F Overlay50     = {0.0f, 0.0f, 0.0f, 0.5f};
    constexpr D2D1_COLOR_F RemoveBtnBg   = {0.15f, 0.15f, 0.15f, 1.0f};

    // Per-target graph line colors (8 cycle)
    constexpr D2D1_COLOR_F LineColors[] = {
        {1.0f, 0.09f, 0.267f, 1.0f},   // bright red
        {1.0f, 0.45f, 0.45f, 1.0f},    // salmon
        {1.0f, 0.6f, 0.8f, 1.0f},      // pink
        {0.9f, 0.9f, 0.9f, 1.0f},      // near white
        {1.0f, 0.7f, 0.3f, 1.0f},      // orange
        {0.8f, 0.2f, 0.5f, 1.0f},      // magenta
        {1.0f, 0.85f, 0.85f, 1.0f},    // light pink
        {0.7f, 0.1f, 0.1f, 1.0f},      // dark red
    };
    constexpr int LineColorCount = 8;

    // Layout dimensions (in DIPs at 96 DPI)
    constexpr float SidebarWidth      = 320.0f;
    constexpr float AspectThreshold   = 1.2f;
    constexpr float HeaderHeight      = 60.0f;
    constexpr float SeparatorHeight   = 2.0f;
    constexpr float TargetItemHeight  = 72.0f;
    constexpr float TargetItemSpacing = 2.0f;
    constexpr float BottomBtnHeight   = 38.0f;
    constexpr float BottomBtnMargin   = 6.0f;
    constexpr float LegendHeight      = 36.0f;
    constexpr float SettingsWidth     = 320.0f;
    constexpr float DialogWidth       = 400.0f;
    constexpr float DialogHeight      = 290.0f;
    constexpr float DialogBtnWidth    = 150.0f;
    constexpr float DialogBtnHeight   = 40.0f;
    constexpr float DialogInputHeight = 38.0f;
    constexpr float SliderHandleW     = 20.0f;
    constexpr float SliderTrackH      = 6.0f;
    constexpr float ScrollPadding     = 4.0f;

    // Graph constants
    constexpr float GraphHeight       = 5.0f;
    constexpr float CameraTilt        = 3.0f;
    constexpr float CameraFOV         = 50.0f;
    constexpr int   SplineSubdivisions= 10;
    constexpr int   TimeLabelCount    = 7;
    constexpr float ZOffsetPerTarget  = 0.12f;
    constexpr float GraphLineWidth    = 1.5f;
    constexpr float GridExtentX       = 30.0f;
}
