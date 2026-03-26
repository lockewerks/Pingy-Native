#include "ui_sidebar.h"
#include "theme.h"
#include "math_util.h"

static D2D1_RECT_F GearBtnRect(const LayoutRects& L, float dpi) {
    float s = 40 * dpi;
    return {L.header.right - 10*dpi - s, L.header.top + (Theme::HeaderHeight*dpi - s)/2,
            L.header.right - 10*dpi, L.header.top + (Theme::HeaderHeight*dpi + s)/2};
}

static D2D1_RECT_F StartStopRect(const LayoutRects& L, float dpi) {
    float w = 72*dpi, h = 32*dpi;
    float right = L.header.right - 54*dpi;
    float cy = L.header.top + Theme::HeaderHeight*dpi*0.5f;
    return {right - w, cy - h/2, right, cy + h/2};
}

static D2D1_RECT_F ItemRect(const LayoutRects& L, int index, float scrollOffset, float dpi) {
    float pad = Theme::ScrollPadding * dpi;
    float itemH = Theme::TargetItemHeight * dpi;
    float spacing = Theme::TargetItemSpacing * dpi;
    float y = L.targetScroll.top + pad + index * (itemH + spacing) - scrollOffset;
    return {L.targetScroll.left + pad, y, L.targetScroll.right - pad, y + itemH};
}

static D2D1_RECT_F RemoveBtnRect(const D2D1_RECT_F& item, float dpi) {
    float s = 30 * dpi;
    float cx = item.right - 6*dpi - s/2;
    float cy = (item.top + item.bottom) / 2;
    return {cx - s/2, cy - s/2, cx + s/2, cy + s/2};
}

void UISidebar::Draw(Renderer& r, PingManager& pm, const LayoutRects& L,
                      SidebarState& state, float dpi) {
    // Sidebar background
    r.FillRect(L.sidebar, Theme::PanelBg);

    // Right border
    r.FillRect({L.sidebar.right - 1*dpi, L.sidebar.top, L.sidebar.right, L.sidebar.bottom}, Theme::Border);

    // Header
    r.FillRect(L.header, Theme::HeaderBg);

    // Title "PINGY"
    D2D1_RECT_F titleRect = {L.header.left + 16*dpi, L.header.top, L.header.left + 180*dpi, L.header.bottom};
    r.DrawText(L"PINGY", titleRect, TextFmt::Title28Bold, Theme::PrimaryRed);

    // Gear button
    auto gearR = GearBtnRect(L, dpi);
    r.DrawText(L"\u2699", gearR, TextFmt::Heading22Bold,
               state.hoverGear ? Theme::TextPrimary : Theme::TextSecondary,
               DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    // Start/Stop button
    auto ssR = StartStopRect(L, dpi);
    bool running = pm.IsRunning();
    D2D1_COLOR_F ssBg = running ? Theme::Surface : Theme::PrimaryRed;
    if (state.hoverStartStop) ssBg = ColorScale(ssBg, 1.2f);
    r.FillRect(ssR, ssBg);
    r.DrawText(running ? L"STOP" : L"START", ssR, TextFmt::Small13,
               Theme::TextPrimary, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    // Separator
    r.FillRect(L.headerSep, Theme::PrimaryRed);

    // Target list (clipped)
    r.PushClip(L.targetScroll);
    auto& targets = pm.Targets();
    for (int i = 0; i < (int)targets.size(); i++) {
        auto itemR = ItemRect(L, i, state.scrollOffset, dpi);
        if (itemR.bottom < L.targetScroll.top || itemR.top > L.targetScroll.bottom) continue;

        auto& t = targets[i];
        bool hovered = (i == state.hoverItemIndex);
        r.FillRect(itemR, hovered ? Theme::SurfaceHover : Theme::Surface);

        // Status dot
        float dotX = itemR.left + 10*dpi + 5*dpi;
        float dotY = (itemR.top + itemR.bottom) / 2;
        D2D1_COLOR_F dotColor = Theme::TextDim;
        if (t.historyCount > 0) {
            dotColor = (t.currentLatency >= 0) ? Theme::LatencyGood : Theme::PrimaryRed;
        }
        r.FillEllipse({dotX, dotY}, 5*dpi, 5*dpi, dotColor);

        float textLeft = itemR.left + 28*dpi;
        float midY = (itemR.top + itemR.bottom) / 2;
        float rightEdge = itemR.right - 42*dpi;

        // Name (top-left)
        D2D1_RECT_F nameR = {textLeft, itemR.top + 6*dpi, rightEdge * 0.55f + itemR.left * 0.45f, midY};
        r.DrawText(t.displayName, nameR, TextFmt::Body16Bold, Theme::TextPrimary,
                   DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

        // Host (bottom-left)
        wchar_t hostBuf[256];
        if (t.minLatency > 0 || t.maxLatency > 0) {
            wchar_t minB[16], maxB[16];
            FmtFloat(minB, 16, t.minLatency, 0);
            FmtFloat(maxB, 16, t.maxLatency, 0);
            wsprintfW(hostBuf, L"%s  [%s-%sms]", t.host.c_str(), minB, maxB);
        } else {
            wsprintfW(hostBuf, L"%s", t.host.c_str());
        }
        D2D1_RECT_F hostR = {textLeft, midY, rightEdge * 0.55f + itemR.left * 0.45f, itemR.bottom - 6*dpi};
        r.DrawText(hostBuf, hostR, TextFmt::Tiny12, Theme::TextDim,
                   DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

        // Latency (top-right)
        wchar_t latBuf[64];
        D2D1_COLOR_F latColor = Theme::TextDim;
        if (t.historyCount == 0) {
            wsprintfW(latBuf, L"--");
        } else if (t.currentLatency >= 0) {
            wsprintfW(latBuf, L"%dms", (int)t.currentLatency);
            if (t.currentLatency < 30) latColor = Theme::LatencyGood;
            else if (t.currentLatency < 100) latColor = Theme::TextPrimary;
            else if (t.currentLatency < 200) latColor = Theme::LatencyMedium;
            else latColor = Theme::LatencyBad;
        } else {
            wsprintfW(latBuf, L"TIMEOUT");
            latColor = Theme::PrimaryRed;
        }
        D2D1_RECT_F latR = {rightEdge * 0.55f + itemR.left * 0.45f + 4*dpi, itemR.top + 6*dpi, rightEdge, midY};
        r.DrawText(latBuf, latR, TextFmt::Latency20Bold, latColor,
                   DWRITE_TEXT_ALIGNMENT_TRAILING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

        // Stats (bottom-right)
        wchar_t statBuf[128];
        if (t.historyCount > 0) {
            wchar_t avgB[16], lossB[16];
            FmtFloat(avgB, 16, t.averageLatency, 0);
            FmtFloat(lossB, 16, t.packetLossPercent, 1);
            wsprintfW(statBuf, L"avg %sms | loss %s%%", avgB, lossB);
        } else {
            wsprintfW(statBuf, L"%s", pm.IsRunning() ? L"waiting..." : L"stopped");
        }
        D2D1_RECT_F statR = {rightEdge * 0.55f + itemR.left * 0.45f + 4*dpi, midY, rightEdge, itemR.bottom - 6*dpi};
        r.DrawText(statBuf, statR, TextFmt::Micro11, Theme::TextSecondary,
                   DWRITE_TEXT_ALIGNMENT_TRAILING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

        // Remove button
        auto rmR = RemoveBtnRect(itemR, dpi);
        bool rmHover = (i == state.hoverRemoveIndex);
        r.FillRect(rmR, rmHover ? Theme::SurfaceHover : Theme::RemoveBtnBg);
        r.DrawText(L"X", rmR, TextFmt::Small14, Theme::SecondaryRed,
                   DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    }
    r.PopClip();

    // Add button
    bool addHover = state.hoverAddBtn;
    r.FillRect(L.addButton, addHover ? ColorScale(Theme::DimRed, 1.3f) : Theme::DimRed);
    r.DrawText(L"+ ADD TARGET", L.addButton, TextFmt::Small14, Theme::TextPrimary,
               DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
}

int UISidebar::HitTest(const LayoutRects& L, PingManager& pm, SidebarState& state,
                        float mx, float my, float dpi, WStr& removeHost) {
    if (PointInRect(mx, my, GearBtnRect(L, dpi))) return 1;
    if (PointInRect(mx, my, StartStopRect(L, dpi))) return 2;
    if (PointInRect(mx, my, L.addButton)) return 3;

    // Check remove buttons
    auto& targets = pm.Targets();
    for (int i = 0; i < (int)targets.size(); i++) {
        auto itemR = ItemRect(L, i, state.scrollOffset, dpi);
        auto rmR = RemoveBtnRect(itemR, dpi);
        if (PointInRect(mx, my, rmR)) {
            removeHost = targets[i].host;
            return 4; // remove
        }
    }
    return 0;
}

void UISidebar::UpdateHover(const LayoutRects& L, PingManager& pm, SidebarState& state,
                             float mx, float my, float dpi) {
    state.hoverGear = PointInRect(mx, my, GearBtnRect(L, dpi));
    state.hoverStartStop = PointInRect(mx, my, StartStopRect(L, dpi));
    state.hoverAddBtn = PointInRect(mx, my, L.addButton);
    state.hoverItemIndex = -1;
    state.hoverRemoveIndex = -1;

    auto& targets = pm.Targets();
    for (int i = 0; i < (int)targets.size(); i++) {
        auto itemR = ItemRect(L, i, state.scrollOffset, dpi);
        if (PointInRect(mx, my, itemR)) {
            state.hoverItemIndex = i;
            auto rmR = RemoveBtnRect(itemR, dpi);
            if (PointInRect(mx, my, rmR)) state.hoverRemoveIndex = i;
            break;
        }
    }
}

void UISidebar::Scroll(SidebarState& state, int delta, const LayoutRects& L,
                        int targetCount, float dpi) {
    float itemH = Theme::TargetItemHeight * dpi;
    float spacing = Theme::TargetItemSpacing * dpi;
    float pad = Theme::ScrollPadding * dpi;
    float contentH = targetCount * (itemH + spacing) - spacing + pad * 2;
    float viewH = RectHeight(L.targetScroll);
    float maxScroll = contentH > viewH ? contentH - viewH : 0;

    state.scrollOffset -= delta * 0.3f;
    state.scrollOffset = Clamp(state.scrollOffset, 0, maxScroll);
}
