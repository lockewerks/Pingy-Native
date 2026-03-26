// 3D perspective graph rendering for network latency data.
// Because a flat 2D line chart would have been too easy, too readable,
// and entirely too reasonable. I gave it a perspective camera, Catmull-Rom
// spline interpolation, and Z-depth parallax for a goddamn ping monitor.
#include "graph.h"
#include "theme.h"
#include "math_util.h"
#include "containers.h"

struct CameraParams {
    float dist;
    float graphWidth;
};

static CameraParams ComputeCamera(float renderW, float renderH) {
    CameraParams cam;
    float aspect = renderW / (renderH > 0 ? renderH : 1.0f);
    float halfFOV = Theme::CameraFOV * 0.5f * DEG2RAD;

    cam.dist = (Theme::GraphHeight * 0.5f) / tanf(halfFOV);

    float hHalfFOV = atanf(tanf(halfFOV) * aspect);
    cam.graphWidth = 2.0f * cam.dist * tanf(hHalfFOV) * 0.94f;
    return cam;
}

// Projects world coordinates to screen space. Yes, I have world coordinates.
// For a ping graph. Each target gets its own Z-depth layer for parallax.
// I could've just drawn flat lines but I chose violence.
static D2D1_POINT_2F ProjectToScreen(float wx, float wy, float wz,
                                      const CameraParams& cam, const D2D1_RECT_F& rect) {
    float halfFOV = Theme::CameraFOV * 0.5f * DEG2RAD;
    float rw = RectWidth(rect);
    float rh = RectHeight(rect);
    float aspect = rw / (rh > 0 ? rh : 1.0f);

    // X: perspective divide using Z depth
    float cx = wx - cam.graphWidth * 0.5f;
    float depth = cam.dist - wz;
    if (depth < 0.1f) depth = 0.1f;
    float px = cx / (depth * tanf(halfFOV) * aspect);
    float screenX = rect.left + (px * 0.5f + 0.5f) * rw;

    // Y: linear mapping — 0ms is always at bottom, GraphHeight at top
    float yFrac = wy / Theme::GraphHeight;
    float screenY = rect.bottom - yFrac * rh;

    // Subtle vertical offset per Z-layer because flat graphs are for cowards
    screenY += wz * 1.5f;

    return {screenX, screenY};
}

static void DrawGrid(Renderer& r, const CameraParams& cam, const D2D1_RECT_F& rect, float dpi) {
    float gh = Theme::GraphHeight;
    float gw = cam.graphWidth;

    int hCount = (int)(gh / 0.5f);
    for (int i = 0; i <= hCount; i++) {
        float y = i * 0.5f;
        auto p1 = ProjectToScreen(0, y, 0, cam, rect);
        auto p2 = ProjectToScreen(gw, y, 0, cam, rect);

        D2D1_COLOR_F c;
        float sw;
        if (i == 0) { c = {0.3f, 0.3f, 0.3f, 0.6f}; sw = 1.5f; }
        else if (i % 2 == 0) { c = {0.2f, 0.2f, 0.2f, 0.25f}; sw = 0.8f; }
        else { c = {0.15f, 0.15f, 0.15f, 0.15f}; sw = 0.5f; }

        r.DrawLine(p1, p2, c, sw * dpi);
    }

    for (float x = 0; x <= gw + 0.01f; x += 0.5f) {
        auto p1 = ProjectToScreen(x, 0, 0, cam, rect);
        auto p2 = ProjectToScreen(x, gh, 0, cam, rect);
        r.DrawLine(p1, p2, {0.18f, 0.18f, 0.18f, 0.12f}, 0.5f * dpi);
    }
}

static void DrawLines(Renderer& r, PingManager& pm, const CameraParams& cam,
                       const D2D1_RECT_F& rect, float dpi) {
    auto& targets = pm.Targets();

    for (int ti = 0; ti < (int)targets.size(); ti++) {
        auto& t = targets[ti];
        if (t.historyCount < 2) continue;

        float zOff = -ti * Theme::ZOffsetPerTarget;
        float maxLat = t.maxLatency > 0 ? t.maxLatency : 100.0f;
        float ceil = maxLat * 1.2f;
        float yScale = Theme::GraphHeight / (ceil > 50.0f ? ceil : 50.0f);

        int count = t.historyCount;
        float xScale = cam.graphWidth / (float)(count - 1);

        Vec<Vec3> raw;
        raw.resize(count);
        for (int i = 0; i < count; i++) {
            auto result = t.GetHistoryAt(i);
            float x = i * xScale;
            float yVal = result.success ? (float)result.latencyMs * yScale : 0.0f;
            if (yVal > Theme::GraphHeight) yVal = Theme::GraphHeight;
            raw[i] = {x, yVal, zOff};
        }

        int subDiv = Theme::SplineSubdivisions;
        int splineCount = (count - 1) * subDiv + 1;
        Vec<D2D1_POINT_2F> screenPts;
        screenPts.resize(splineCount);

        int idx = 0;
        for (int i = 0; i < count - 1; i++) {
            Vec3 p0 = raw[(i - 1 > 0) ? i - 1 : 0];
            Vec3 p1 = raw[i];
            Vec3 p2 = raw[i + 1];
            int i3 = i + 2;
            Vec3 p3 = raw[(i3 < count) ? i3 : count - 1];

            for (int s = 0; s < subDiv; s++) {
                float st = s / (float)subDiv;
                Vec3 pt = CatmullRom(p0, p1, p2, p3, st);
                screenPts[idx++] = ProjectToScreen(pt.x, pt.y, pt.z, cam, rect);
            }
        }
        screenPts[idx] = ProjectToScreen(raw.back().x, raw.back().y, raw.back().z, cam, rect);

        D2D1_COLOR_F lineColor = Theme::LineColors[ti % Theme::LineColorCount];
        ID2D1PathGeometry* path = r.CreatePathFromPoints(screenPts.data(), splineCount);
        if (path) {
            r.DrawPath(path, lineColor, Theme::GraphLineWidth * dpi);
            path->Release();
        }
    }
}

static WStr FormatTimeAgo(float ms) {
    if (ms < 500) return WStr(L"now");
    float sec = ms / 1000.0f;
    if (sec < 60) {
        wchar_t buf[16]; FmtFloat(buf, 16, sec, 0);
        WStr r(buf); r += L"s"; return r;
    }
    float mn = sec / 60.0f;
    if (mn < 60) {
        int m = (int)mn;
        int s = (int)fmodf(sec, 60.0f);
        wchar_t buf[32];
        if (s > 0) wsprintfW(buf, L"%dm%ds", m, s);
        else wsprintfW(buf, L"%dm", m);
        return WStr(buf);
    }
    wchar_t buf[16]; FmtFloat(buf, 16, mn/60.0f, 0);
    WStr r(buf); r += L"h"; return r;
}

void Graph::Draw(Renderer& r, PingManager& pm, const LayoutRects& L, float dpi) {
    auto& graphR = L.graphArea;
    r.FillRect(graphR, Theme::GraphBg);

    // Projection rect: inset so 0ms baseline sits above time labels,
    // and max latency sits below the title area
    float topPad = 60 * dpi;   // room for title + status
    float bottomPad = 28 * dpi; // room for time labels
    D2D1_RECT_F projRect = {graphR.left, graphR.top + topPad, graphR.right, graphR.bottom - bottomPad};

    auto cam = ComputeCamera(RectWidth(projRect), RectHeight(projRect));

    r.PushClip(graphR);
    DrawGrid(r, cam, projRect, dpi);
    DrawLines(r, pm, cam, projRect, dpi);
    r.PopClip();

    // Overlay: left label strip
    float stripW = 72 * dpi;
    r.FillRect({graphR.left, graphR.top, graphR.left + stripW, graphR.bottom}, Theme::Overlay50);

    // Title "LATENCY"
    D2D1_RECT_F titleR = {graphR.left + stripW + 8 * dpi, graphR.top + 10 * dpi,
                           graphR.right, graphR.top + 38 * dpi};
    r.DrawText(L"LATENCY", titleR, TextFmt::GraphTitle18Bold, Theme::PrimaryRed);

    // Status
    D2D1_RECT_F statusR = {graphR.left + stripW + 8 * dpi, graphR.top + 36 * dpi,
                            graphR.right, graphR.top + 56 * dpi};
    r.DrawText(pm.IsRunning() ? L"RUNNING" : L"STOPPED", statusR, TextFmt::Small14,
               pm.IsRunning() ? Theme::LatencyGood : Theme::TextDim);

    // Y-axis labels — aligned to projRect (where data actually renders)
    float maxLat = 0;
    int maxHistory = 0;
    for (auto& t : pm.Targets()) {
        if (t.maxLatency > maxLat) maxLat = t.maxLatency;
        if (t.historyCount > maxHistory) maxHistory = t.historyCount;
    }
    if (maxLat <= 0) maxLat = 100;
    float displayMax = maxLat * 1.2f;
    if (displayMax < 50) displayMax = 50;

    // Max label — at top of projection rect
    wchar_t yBuf[32];
    FmtFloat(yBuf, 24, displayMax, 0);
    wchar_t yLabel[32];
    wsprintfW(yLabel, L"%s ms", yBuf);
    D2D1_RECT_F maxR = {graphR.left + 6 * dpi, projRect.top - 2 * dpi,
                         graphR.left + stripW - 2 * dpi, projRect.top + 18 * dpi};
    r.DrawText(yLabel, maxR, TextFmt::Small13, Theme::TextSecondary,
               DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_NEAR);

    // Mid label — at middle of projection rect
    FmtFloat(yBuf, 24, displayMax * 0.5f, 0);
    wsprintfW(yLabel, L"%s ms", yBuf);
    float midY = (projRect.top + projRect.bottom) / 2;
    D2D1_RECT_F midR = {graphR.left + 6 * dpi, midY - 10 * dpi,
                         graphR.left + stripW - 2 * dpi, midY + 10 * dpi};
    r.DrawText(yLabel, midR, TextFmt::Small13, Theme::TextSecondary,
               DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    // 0ms label — at bottom of projection rect (where 0ms actually is)
    D2D1_RECT_F minR = {graphR.left + 6 * dpi, projRect.bottom - 18 * dpi,
                         graphR.left + stripW - 2 * dpi, projRect.bottom + 2 * dpi};
    r.DrawText(L"0 ms", minR, TextFmt::Small13, Theme::TextSecondary,
               DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_FAR);

    // X-axis time labels — below the projection rect
    if (maxHistory > 0) {
        float totalMs = (float)maxHistory * pm.Settings().timeoutMs;
        float gw = RectWidth(graphR);
        for (int i = 0; i < Theme::TimeLabelCount; i++) {
            float frac = i / (float)(Theme::TimeLabelCount - 1);
            float xFrac = Lerp(0.06f, 0.97f, frac);
            float x = graphR.left + xFrac * gw;
            float timeAgoMs = totalMs * (1.0f - frac);
            auto label = FormatTimeAgo(timeAgoMs);
            D2D1_RECT_F tR = {x - 30 * dpi, projRect.bottom + 2 * dpi,
                               x + 30 * dpi, projRect.bottom + 22 * dpi};
            r.DrawText(label.c_str(), tR, TextFmt::Micro11, Theme::TextDim,
                       DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        }
    }
}

void Graph::DrawLegend(Renderer& r, PingManager& pm, const LayoutRects& L, float dpi) {
    r.FillRect(L.legendBar, Theme::Overlay65);

    auto& targets = pm.Targets();
    if (targets.empty()) return;

    float w = RectWidth(L.legendBar);
    float pad = 8 * dpi;
    float availW = w - pad * 2;
    float entryW = availW / (float)targets.size();

    for (int i = 0; i < (int)targets.size(); i++) {
        float x = L.legendBar.left + pad + i * entryW;
        float cy = (L.legendBar.top + L.legendBar.bottom) / 2;

        float swatchW = 14 * dpi, swatchH = 4 * dpi;
        D2D1_RECT_F swatch = {x, cy - swatchH / 2, x + swatchW, cy + swatchH / 2};
        r.FillRect(swatch, Theme::LineColors[i % Theme::LineColorCount]);

        D2D1_RECT_F nameR = {x + swatchW + 6 * dpi, L.legendBar.top,
                              x + entryW - 4 * dpi, L.legendBar.bottom};
        r.DrawText(targets[i].displayName, nameR, TextFmt::Tiny12, Theme::TextSecondary,
                   DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    }
}
