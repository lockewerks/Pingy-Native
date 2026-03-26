#pragma once
#include <d2d1.h>
#include <dwrite.h>
#include "containers.h"

// Text format identifiers
enum class TextFmt {
    Title28Bold,     // PINGY title
    Heading22Bold,   // SETTINGS, ADD TARGET
    Body16,          // General text
    Body16Bold,      // Target names
    Small14,         // Status labels
    Small13,         // Section labels
    Small13Bold,     // Section headings
    Tiny12,          // Legend names, info text
    Micro11,         // Stats, time labels
    Latency20Bold,   // Latency numbers
    GraphTitle18Bold,// LATENCY
    Count
};

class Renderer {
public:
    bool Initialize(HWND hwnd);
    void Shutdown();
    void Resize(UINT width, UINT height);

    void BeginDraw();
    bool EndDraw(); // returns false if target needs recreation
    void RecreateTarget(HWND hwnd);

    void Clear(const D2D1_COLOR_F& color);
    void FillRect(const D2D1_RECT_F& rect, const D2D1_COLOR_F& color);
    void DrawRect(const D2D1_RECT_F& rect, const D2D1_COLOR_F& color, float strokeWidth = 1.0f);
    void DrawLine(D2D1_POINT_2F p1, D2D1_POINT_2F p2, const D2D1_COLOR_F& color, float strokeWidth = 1.0f);
    void FillEllipse(D2D1_POINT_2F center, float rx, float ry, const D2D1_COLOR_F& color);
    void DrawText(const WStr& text, const D2D1_RECT_F& rect, TextFmt fmt, const D2D1_COLOR_F& color,
                  DWRITE_TEXT_ALIGNMENT hAlign = DWRITE_TEXT_ALIGNMENT_LEADING,
                  DWRITE_PARAGRAPH_ALIGNMENT vAlign = DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    void PushClip(const D2D1_RECT_F& rect);
    void PopClip();

    // For graph path geometry
    ID2D1PathGeometry* CreatePathFromPoints(const D2D1_POINT_2F* points, int count);
    void DrawPath(ID2D1PathGeometry* path, const D2D1_COLOR_F& color, float strokeWidth);

    // Text measurement
    float MeasureTextWidth(const WStr& text, TextFmt fmt);

    ID2D1HwndRenderTarget* RT() { return rt_; }
    IDWriteFactory* DWrite() { return dwriteFactory_; }
    float DpiScale() const { return dpiScale_; }
    void SetDpiScale(float s) { dpiScale_ = s; }

private:
    void CreateBrush(const D2D1_COLOR_F& color);
    ID2D1SolidColorBrush* GetBrush(const D2D1_COLOR_F& color);

    ID2D1Factory* factory_ = nullptr;
    ID2D1HwndRenderTarget* rt_ = nullptr;
    IDWriteFactory* dwriteFactory_ = nullptr;
    IDWriteTextFormat* textFormats_[(int)TextFmt::Count] = {};
    ID2D1SolidColorBrush* brush_ = nullptr; // single reusable brush
    float dpiScale_ = 1.0f;
};
