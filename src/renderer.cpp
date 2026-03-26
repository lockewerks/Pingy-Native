#include "renderer.h"

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

bool Renderer::Initialize(HWND hwnd) {
    HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &factory_);
    if (FAILED(hr)) return false;

    hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,
        __uuidof(IDWriteFactory), (IUnknown**)&dwriteFactory_);
    if (FAILED(hr)) return false;

    // Get DPI
    UINT dpi = GetDpiForWindow(hwnd);
    dpiScale_ = dpi / 96.0f;

    RECT rc;
    GetClientRect(hwnd, &rc);
    D2D1_SIZE_U size = {(UINT32)(rc.right - rc.left), (UINT32)(rc.bottom - rc.top)};

    D2D1_RENDER_TARGET_PROPERTIES rtProps = D2D1::RenderTargetProperties();
    rtProps.dpiX = 96.0f;  // We handle DPI scaling ourselves
    rtProps.dpiY = 96.0f;

    hr = factory_->CreateHwndRenderTarget(
        rtProps,
        D2D1::HwndRenderTargetProperties(hwnd, size, D2D1_PRESENT_OPTIONS_NONE),
        &rt_);
    if (FAILED(hr)) return false;

    rt_->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
    rt_->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE);

    // Create reusable brush
    rt_->CreateSolidColorBrush(D2D1::ColorF(1, 1, 1, 1), &brush_);

    // Create text formats
    struct FmtDef { float size; DWRITE_FONT_WEIGHT weight; };
    FmtDef fmts[] = {
        {28, DWRITE_FONT_WEIGHT_BOLD},    // Title28Bold
        {22, DWRITE_FONT_WEIGHT_BOLD},    // Heading22Bold
        {16, DWRITE_FONT_WEIGHT_REGULAR}, // Body16
        {16, DWRITE_FONT_WEIGHT_BOLD},    // Body16Bold
        {14, DWRITE_FONT_WEIGHT_REGULAR}, // Small14
        {13, DWRITE_FONT_WEIGHT_REGULAR}, // Small13
        {13, DWRITE_FONT_WEIGHT_BOLD},    // Small13Bold
        {12, DWRITE_FONT_WEIGHT_REGULAR}, // Tiny12
        {11, DWRITE_FONT_WEIGHT_REGULAR}, // Micro11
        {20, DWRITE_FONT_WEIGHT_BOLD},    // Latency20Bold
        {18, DWRITE_FONT_WEIGHT_BOLD},    // GraphTitle18Bold
    };

    for (int i = 0; i < (int)TextFmt::Count; i++) {
        dwriteFactory_->CreateTextFormat(
            L"Segoe UI", nullptr,
            fmts[i].weight, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
            fmts[i].size * dpiScale_, L"en-us", &textFormats_[i]);
        if (textFormats_[i]) {
            textFormats_[i]->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
        }
    }

    return true;
}

void Renderer::Shutdown() {
    for (int i = 0; i < (int)TextFmt::Count; i++) { if (textFormats_[i]) { textFormats_[i]->Release(); textFormats_[i] = nullptr; } }
    if (brush_) { brush_->Release(); brush_ = nullptr; }
    if (rt_) { rt_->Release(); rt_ = nullptr; }
    if (dwriteFactory_) { dwriteFactory_->Release(); dwriteFactory_ = nullptr; }
    if (factory_) { factory_->Release(); factory_ = nullptr; }
}

void Renderer::Resize(UINT width, UINT height) {
    if (rt_) rt_->Resize(D2D1::SizeU(width, height));
}

void Renderer::RecreateTarget(HWND hwnd) {
    if (brush_) { brush_->Release(); brush_ = nullptr; }
    if (rt_) { rt_->Release(); rt_ = nullptr; }

    RECT rc;
    GetClientRect(hwnd, &rc);
    D2D1_SIZE_U size = {(UINT32)(rc.right - rc.left), (UINT32)(rc.bottom - rc.top)};

    D2D1_RENDER_TARGET_PROPERTIES rtProps = D2D1::RenderTargetProperties();
    rtProps.dpiX = 96.0f;
    rtProps.dpiY = 96.0f;

    factory_->CreateHwndRenderTarget(
        rtProps,
        D2D1::HwndRenderTargetProperties(hwnd, size, D2D1_PRESENT_OPTIONS_NONE),
        &rt_);
    if (rt_) {
        rt_->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
        rt_->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE);
        rt_->CreateSolidColorBrush(D2D1::ColorF(1, 1, 1, 1), &brush_);
    }
}

void Renderer::BeginDraw() { if (rt_) rt_->BeginDraw(); }

bool Renderer::EndDraw() {
    if (!rt_) return false;
    HRESULT hr = rt_->EndDraw();
    return hr != D2DERR_RECREATE_TARGET;
}

void Renderer::Clear(const D2D1_COLOR_F& color) {
    if (rt_) rt_->Clear(color);
}

ID2D1SolidColorBrush* Renderer::GetBrush(const D2D1_COLOR_F& color) {
    if (brush_) brush_->SetColor(color);
    return brush_;
}

void Renderer::FillRect(const D2D1_RECT_F& rect, const D2D1_COLOR_F& color) {
    if (rt_) rt_->FillRectangle(rect, GetBrush(color));
}

void Renderer::DrawRect(const D2D1_RECT_F& rect, const D2D1_COLOR_F& color, float strokeWidth) {
    if (rt_) rt_->DrawRectangle(rect, GetBrush(color), strokeWidth);
}

void Renderer::DrawLine(D2D1_POINT_2F p1, D2D1_POINT_2F p2, const D2D1_COLOR_F& color, float strokeWidth) {
    if (rt_) rt_->DrawLine(p1, p2, GetBrush(color), strokeWidth);
}

void Renderer::FillEllipse(D2D1_POINT_2F center, float rx, float ry, const D2D1_COLOR_F& color) {
    if (rt_) rt_->FillEllipse(D2D1::Ellipse(center, rx, ry), GetBrush(color));
}

void Renderer::DrawText(const WStr& text, const D2D1_RECT_F& rect, TextFmt fmt,
                         const D2D1_COLOR_F& color, DWRITE_TEXT_ALIGNMENT hAlign,
                         DWRITE_PARAGRAPH_ALIGNMENT vAlign) {
    if (!rt_ || !textFormats_[(int)fmt]) return;
    auto* tf = textFormats_[(int)fmt];
    tf->SetTextAlignment(hAlign);
    tf->SetParagraphAlignment(vAlign);
    rt_->DrawText(text.c_str(), (UINT32)text.size(), tf, rect, GetBrush(color));
}

void Renderer::PushClip(const D2D1_RECT_F& rect) {
    if (rt_) rt_->PushAxisAlignedClip(rect, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
}

void Renderer::PopClip() {
    if (rt_) rt_->PopAxisAlignedClip();
}

ID2D1PathGeometry* Renderer::CreatePathFromPoints(const D2D1_POINT_2F* points, int count) {
    if (!factory_ || count < 2) return nullptr;
    ID2D1PathGeometry* path = nullptr;
    factory_->CreatePathGeometry(&path);
    if (!path) return nullptr;

    ID2D1GeometrySink* sink = nullptr;
    path->Open(&sink);
    if (sink) {
        sink->BeginFigure(points[0], D2D1_FIGURE_BEGIN_HOLLOW);
        for (int i = 1; i < count; i++) {
            sink->AddLine(points[i]);
        }
        sink->EndFigure(D2D1_FIGURE_END_OPEN);
        sink->Close();
        sink->Release();
    }
    return path;
}

void Renderer::DrawPath(ID2D1PathGeometry* path, const D2D1_COLOR_F& color, float strokeWidth) {
    if (rt_ && path) rt_->DrawGeometry(path, GetBrush(color), strokeWidth);
}

float Renderer::MeasureTextWidth(const WStr& text, TextFmt fmt) {
    if (!dwriteFactory_ || !textFormats_[(int)fmt]) return 0;
    IDWriteTextLayout* layout = nullptr;
    dwriteFactory_->CreateTextLayout(text.c_str(), (UINT32)text.size(),
        textFormats_[(int)fmt], 10000, 1000, &layout);
    if (!layout) return 0;
    DWRITE_TEXT_METRICS metrics;
    layout->GetMetrics(&metrics);
    layout->Release();
    return metrics.width;
}
