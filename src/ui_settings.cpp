#include "ui_settings.h"
#include "theme.h"
#include "math_util.h"

D2D1_RECT_F UISettings::PanelRect(const LayoutRects& L, float dpi) {
    float w = Theme::SettingsWidth * dpi;
    return {L.windowWidth - w, 0, L.windowWidth, L.windowHeight};
}

struct SliderLayout {
    D2D1_RECT_F track;
    D2D1_RECT_F fill;
    D2D1_RECT_F handle;
    D2D1_RECT_F valueRect;
    D2D1_RECT_F labelRect;
};

static SliderLayout ComputeSlider(D2D1_RECT_F panel, float yOff, float value, float minVal, float maxVal,
                                   const wchar_t* label, float dpi) {
    SliderLayout s;
    float pad = 16 * dpi;
    float y = panel.top + yOff;

    s.labelRect = {panel.left + pad, y, panel.right - pad, y + 22*dpi};
    y += 28*dpi;

    float trackLeft = panel.left + pad;
    float trackRight = panel.right - 80*dpi;
    float trackH = Theme::SliderTrackH * dpi;
    float handleW = Theme::SliderHandleW * dpi;

    s.track = {trackLeft, y - trackH/2, trackRight, y + trackH/2};

    float frac = (value - minVal) / (maxVal - minVal);
    float handleX = trackLeft + frac * (trackRight - trackLeft);
    s.fill = {trackLeft, y - trackH/2, handleX, y + trackH/2};
    s.handle = {handleX - handleW/2, y - 12*dpi, handleX + handleW/2, y + 12*dpi};
    s.valueRect = {trackRight + 8*dpi, y - 12*dpi, panel.right - pad, y + 12*dpi};

    return s;
}

void UISettings::Draw(Renderer& r, const LayoutRects& L, SettingsState& state,
                       PingManager& pm, float dpi) {
    if (!state.visible) return;

    auto panel = PanelRect(L, dpi);
    r.FillRect(panel, Theme::PanelBg);

    // Left border
    r.FillRect({panel.left, panel.top, panel.left + 1*dpi, panel.bottom}, Theme::Border);

    // Title
    D2D1_RECT_F titleR = {panel.left + 16*dpi, panel.top + 16*dpi, panel.right - 40*dpi, panel.top + 50*dpi};
    r.DrawText(L"SETTINGS", titleR, TextFmt::Heading22Bold, Theme::PrimaryRed);

    // Close button
    D2D1_RECT_F closeR = {panel.right - 40*dpi, panel.top + 8*dpi, panel.right - 8*dpi, panel.top + 40*dpi};
    r.FillRect(closeR, Theme::RemoveBtnBg);
    r.DrawText(L"X", closeR, TextFmt::Body16, Theme::TextSecondary,
               DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    // Timeout slider
    auto ts = ComputeSlider(panel, 64*dpi, state.timeoutValue, 50, 5000, L"Ping Timeout / Interval", dpi);
    r.DrawText(L"Ping Timeout / Interval", ts.labelRect, TextFmt::Small13Bold, Theme::TextSecondary,
               DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    r.FillRect(ts.track, Theme::Border);
    r.FillRect(ts.fill, Theme::PrimaryRed);
    r.FillRect(ts.handle, Theme::PrimaryRed);
    wchar_t tvTmp[16]; FmtFloat(tvTmp, 16, state.timeoutValue, 0);
    wchar_t tvBuf[32]; wsprintfW(tvBuf, L"%sms", tvTmp);
    r.DrawText(tvBuf, ts.valueRect, TextFmt::Small14, Theme::TextPrimary,
               DWRITE_TEXT_ALIGNMENT_TRAILING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    // TTL slider
    auto ttls = ComputeSlider(panel, 150*dpi, state.ttlValue, 1, 255, L"TTL (Time To Live)", dpi);
    r.DrawText(L"TTL (Time To Live)", ttls.labelRect, TextFmt::Small13Bold, Theme::TextSecondary,
               DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    r.FillRect(ttls.track, Theme::Border);
    r.FillRect(ttls.fill, Theme::PrimaryRed);
    r.FillRect(ttls.handle, Theme::PrimaryRed);
    wchar_t ttlBuf[32]; FmtFloat(ttlBuf, 32, state.ttlValue, 0);
    r.DrawText(ttlBuf, ttls.valueRect, TextFmt::Small14, Theme::TextPrimary,
               DWRITE_TEXT_ALIGNMENT_TRAILING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    // Apply button
    float applyY = panel.top + 240*dpi;
    D2D1_RECT_F applyR = {panel.left + 16*dpi, applyY, panel.right - 16*dpi, applyY + 40*dpi};
    r.FillRect(applyR, Theme::PrimaryRed);
    r.DrawText(L"APPLY", applyR, TextFmt::Body16, Theme::TextPrimary,
               DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    // Info text
    float infoY = applyY + 50*dpi;
    D2D1_RECT_F infoR = {panel.left + 16*dpi, infoY, panel.right - 16*dpi, infoY + 70*dpi};
    // Enable word wrap for info text
    r.DrawText(L"Timeout controls both the ping timeout and interval between pings. Lower = faster but more traffic.",
               infoR, TextFmt::Tiny12, Theme::TextDim,
               DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
}

int UISettings::HitTest(const LayoutRects& L, SettingsState& state, float mx, float my, float dpi) {
    if (!state.visible) return 0;
    auto panel = PanelRect(L, dpi);

    // Close button
    D2D1_RECT_F closeR = {panel.right - 40*dpi, panel.top + 8*dpi, panel.right - 8*dpi, panel.top + 40*dpi};
    if (PointInRect(mx, my, closeR)) return 1;

    // Apply button
    float applyY = panel.top + 240*dpi;
    D2D1_RECT_F applyR = {panel.left + 16*dpi, applyY, panel.right - 16*dpi, applyY + 40*dpi};
    if (PointInRect(mx, my, applyR)) return 2;

    return 0;
}

void UISettings::OnMouseDown(const LayoutRects& L, SettingsState& state, float mx, float my, float dpi) {
    if (!state.visible) return;
    auto panel = PanelRect(L, dpi);

    auto ts = ComputeSlider(panel, 64*dpi, state.timeoutValue, 50, 5000, nullptr, dpi);
    if (PointInRect(mx, my, ts.handle) || PointInRect(mx, my, ts.track)) {
        state.draggingTimeout = true;
        float frac = Clamp((mx - ts.track.left) / RectWidth(ts.track), 0.0f, 1.0f);
        state.timeoutValue = 50 + frac * (5000 - 50);
    }

    auto ttls = ComputeSlider(panel, 150*dpi, state.ttlValue, 1, 255, nullptr, dpi);
    if (PointInRect(mx, my, ttls.handle) || PointInRect(mx, my, ttls.track)) {
        state.draggingTtl = true;
        float frac = Clamp((mx - ttls.track.left) / RectWidth(ttls.track), 0.0f, 1.0f);
        state.ttlValue = 1 + frac * (255 - 1);
    }
}

void UISettings::OnMouseMove(const LayoutRects& L, SettingsState& state, float mx, float dpi) {
    if (!state.visible) return;
    auto panel = PanelRect(L, dpi);

    if (state.draggingTimeout) {
        auto ts = ComputeSlider(panel, 64*dpi, state.timeoutValue, 50, 5000, nullptr, dpi);
        float frac = Clamp((mx - ts.track.left) / RectWidth(ts.track), 0.0f, 1.0f);
        state.timeoutValue = 50 + frac * (5000 - 50);
    }
    if (state.draggingTtl) {
        auto ttls = ComputeSlider(panel, 150*dpi, state.ttlValue, 1, 255, nullptr, dpi);
        float frac = Clamp((mx - ttls.track.left) / RectWidth(ttls.track), 0.0f, 1.0f);
        state.ttlValue = 1 + frac * (255 - 1);
    }
}

void UISettings::OnMouseUp(SettingsState& state) {
    state.draggingTimeout = false;
    state.draggingTtl = false;
}
