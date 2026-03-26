#include "ui_add_dialog.h"
#include "theme.h"
#include "math_util.h"

static D2D1_RECT_F DialogPanel(float ww, float wh, float dpi) {
    float dw = Theme::DialogWidth * dpi;
    float dh = Theme::DialogHeight * dpi;
    float cx = ww / 2, cy = wh / 2;
    return {cx - dw/2, cy - dh/2, cx + dw/2, cy + dh/2};
}

static D2D1_RECT_F HostInputRect(D2D1_RECT_F p, float dpi) {
    return {p.left + 24*dpi, p.top + 78*dpi, p.right - 24*dpi, p.top + 78*dpi + Theme::DialogInputHeight*dpi};
}

static D2D1_RECT_F NameInputRect(D2D1_RECT_F p, float dpi) {
    return {p.left + 24*dpi, p.top + 148*dpi, p.right - 24*dpi, p.top + 148*dpi + Theme::DialogInputHeight*dpi};
}

static D2D1_RECT_F CancelBtnRect(D2D1_RECT_F p, float dpi) {
    float bw = Theme::DialogBtnWidth * dpi;
    float bh = Theme::DialogBtnHeight * dpi;
    float cx = (p.left + p.right) / 2;
    float y = p.top + 220*dpi;
    return {cx - 8*dpi - bw, y, cx - 8*dpi, y + bh};
}

static D2D1_RECT_F AddBtnRect(D2D1_RECT_F p, float dpi) {
    float bw = Theme::DialogBtnWidth * dpi;
    float bh = Theme::DialogBtnHeight * dpi;
    float cx = (p.left + p.right) / 2;
    float y = p.top + 220*dpi;
    return {cx + 8*dpi, y, cx + 8*dpi + bw, y + bh};
}

void UIAddDialog::Draw(Renderer& r, AddDialogState& state, float ww, float wh,
                        float dpi, ULONGLONG tick) {
    if (!state.visible) return;

    // Overlay
    r.FillRect({0, 0, ww, wh}, Theme::Overlay70);

    auto panel = DialogPanel(ww, wh, dpi);
    r.FillRect(panel, Theme::PanelBg);
    r.DrawRect(panel, Theme::Border, 1.0f);

    // Title
    D2D1_RECT_F titleR = {panel.left, panel.top + 14*dpi, panel.right, panel.top + 48*dpi};
    r.DrawText(L"ADD TARGET", titleR, TextFmt::Heading22Bold, Theme::PrimaryRed,
               DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    // Host label
    D2D1_RECT_F hlR = {panel.left + 24*dpi, panel.top + 56*dpi, panel.right - 24*dpi, panel.top + 76*dpi};
    r.DrawText(L"Host / IP Address", hlR, TextFmt::Small13, Theme::TextSecondary);

    // Host input
    auto hiR = HostInputRect(panel, dpi);
    UITextInput::Draw(r, state.hostInput, hiR, Theme::Surface, dpi, tick);

    // Name label
    D2D1_RECT_F nlR = {panel.left + 24*dpi, panel.top + 126*dpi, panel.right - 24*dpi, panel.top + 146*dpi};
    r.DrawText(L"Display Name (optional)", nlR, TextFmt::Small13, Theme::TextSecondary);

    // Name input
    auto niR = NameInputRect(panel, dpi);
    UITextInput::Draw(r, state.nameInput, niR, Theme::Surface, dpi, tick);

    // Error text
    if (!state.errorText.empty()) {
        D2D1_RECT_F errR = {panel.left + 24*dpi, panel.top + 194*dpi, panel.right - 24*dpi, panel.top + 216*dpi};
        r.DrawText(state.errorText, errR, TextFmt::Small13, Theme::PrimaryRed,
                   DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    }

    // Buttons
    auto cancelR = CancelBtnRect(panel, dpi);
    r.FillRect(cancelR, state.hoverCancel ? Theme::SurfaceHover : Theme::Surface);
    r.DrawText(L"CANCEL", cancelR, TextFmt::Small14, Theme::TextSecondary,
               DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    auto addR = AddBtnRect(panel, dpi);
    r.FillRect(addR, state.hoverAdd ? ColorScale(Theme::PrimaryRed, 1.2f) : Theme::PrimaryRed);
    r.DrawText(L"ADD TARGET", addR, TextFmt::Small14, Theme::TextPrimary,
               DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
}

int UIAddDialog::HitTest(AddDialogState& state, Renderer& r, float mx, float my,
                          float ww, float wh, float dpi) {
    if (!state.visible) return 0;

    auto panel = DialogPanel(ww, wh, dpi);

    // Check inputs for focus
    auto hiR = HostInputRect(panel, dpi);
    auto niR = NameInputRect(panel, dpi);
    if (PointInRect(mx, my, hiR)) {
        state.hostInput.focused = true;
        state.nameInput.focused = false;
        state.hostInput.cursorPos = UITextInput::HitTestCursor(r, state.hostInput, hiR, mx, dpi);
    } else if (PointInRect(mx, my, niR)) {
        state.nameInput.focused = true;
        state.hostInput.focused = false;
        state.nameInput.cursorPos = UITextInput::HitTestCursor(r, state.nameInput, niR, mx, dpi);
    }

    // Buttons
    if (PointInRect(mx, my, AddBtnRect(panel, dpi))) return 1;
    if (PointInRect(mx, my, CancelBtnRect(panel, dpi))) return 2;

    // Click outside panel dismisses
    if (!PointInRect(mx, my, panel)) return 3;

    return 0;
}

bool UIAddDialog::TryAdd(AddDialogState& state, PingManager& pm) {
    WStr host = state.hostInput.text;
    WStr name = state.nameInput.text;

    while (!host.empty() && host.front() == L' ') host.erase(0);
    while (!host.empty() && host.back() == L' ') host.pop_back();
    while (!name.empty() && name.front() == L' ') name.erase(0);
    while (!name.empty() && name.back() == L' ') name.pop_back();

    if (host.empty()) {
        state.errorText = L"Host/IP is required";
        return false;
    }
    if (name.empty()) name = host;

    for (int i = 0; i < pm.Targets().size(); i++) {
        if (pm.Targets()[i].host == host) {
            state.errorText = L"Target already exists";
            return false;
        }
    }

    pm.AddTarget(host, name);
    state.Hide();
    return true;
}

TextInputState* UIAddDialog::FocusedInput(AddDialogState& state) {
    if (state.hostInput.focused) return &state.hostInput;
    if (state.nameInput.focused) return &state.nameInput;
    return nullptr;
}

void UIAddDialog::TabFocus(AddDialogState& state) {
    if (state.hostInput.focused) {
        state.hostInput.focused = false;
        state.nameInput.focused = true;
    } else {
        state.nameInput.focused = false;
        state.hostInput.focused = true;
    }
}
