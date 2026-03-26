#include "ui_text_input.h"
#include "theme.h"

void TextInputState::InsertChar(wchar_t ch) {
    if (ch < 32 && ch != L'\t') return; // filter control chars
    text.insert(cursorPos, ch);
    cursorPos++;
}

void TextInputState::Backspace() {
    if (cursorPos > 0) {
        text.erase(cursorPos - 1, 1);
        cursorPos--;
    }
}

void TextInputState::Delete() {
    if (cursorPos < text.size()) {
        text.erase(cursorPos, 1);
    }
}

void TextInputState::MoveCursor(int delta) {
    cursorPos += delta;
    if (cursorPos < 0) cursorPos = 0;
    if (cursorPos > text.size()) cursorPos = text.size();
}

void TextInputState::Home() { cursorPos = 0; }
void TextInputState::End() { cursorPos = text.size(); }
void TextInputState::SelectAll() { cursorPos = text.size(); }

void TextInputState::Clear() {
    text.clear();
    cursorPos = 0;
}

void TextInputState::PasteFromClipboard(HWND hwnd) {
    if (!OpenClipboard(hwnd)) return;
    HANDLE hData = GetClipboardData(CF_UNICODETEXT);
    if (hData) {
        wchar_t* pszText = (wchar_t*)GlobalLock(hData);
        if (pszText) {
            WStr paste(pszText);
            // Remove newlines
            for (int i = 0; i < paste.size(); i++)
                if (paste[i] == L'\n' || paste[i] == L'\r') paste[i] = L' ';
            text.insert(cursorPos, paste);
            cursorPos += paste.size();
            GlobalUnlock(hData);
        }
    }
    CloseClipboard();
}

void UITextInput::Draw(Renderer& r, const TextInputState& state, const D2D1_RECT_F& rect,
                        const D2D1_COLOR_F& bgColor, float dpiScale, ULONGLONG tickCount) {
    r.FillRect(rect, bgColor);

    float pad = 10 * dpiScale;
    D2D1_RECT_F textRect = {rect.left + pad, rect.top, rect.right - pad, rect.bottom};

    if (state.text.empty() && !state.focused) {
        // Draw placeholder
        r.DrawText(state.placeholder, textRect, TextFmt::Body16, Theme::TextDim,
                   DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    } else {
        // Draw text
        r.DrawText(state.text, textRect, TextFmt::Body16, Theme::TextPrimary,
                   DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

        // Draw cursor (blink every 500ms)
        if (state.focused && ((tickCount / 500) % 2 == 0)) {
            float cursorX = textRect.left;
            if (state.cursorPos > 0) {
                WStr sub = state.text.substr(0, state.cursorPos);
                cursorX += r.MeasureTextWidth(sub, TextFmt::Body16);
            }
            float cy1 = rect.top + 6 * dpiScale;
            float cy2 = rect.bottom - 6 * dpiScale;
            r.DrawLine({cursorX, cy1}, {cursorX, cy2}, Theme::PrimaryRed, 1.5f * dpiScale);
        }
    }

    // Focus border
    if (state.focused) {
        r.DrawRect(rect, Theme::PrimaryRed, 1.0f);
    }
}

int UITextInput::HitTestCursor(Renderer& r, const TextInputState& state, const D2D1_RECT_F& rect,
                                float clickX, float dpiScale) {
    float pad = 10 * dpiScale;
    float baseX = rect.left + pad;
    float relX = clickX - baseX;
    if (relX <= 0) return 0;

    // Binary search for cursor position
    for (int i = 0; i <= state.text.size(); i++) {
        WStr sub = state.text.substr(0, i);
        float w = r.MeasureTextWidth(sub, TextFmt::Body16);
        if (w >= relX) {
            // Check if closer to i or i-1
            if (i > 0) {
                WStr prev = state.text.substr(0, i - 1);
                float pw = r.MeasureTextWidth(prev, TextFmt::Body16);
                return (relX - pw < w - relX) ? i - 1 : i;
            }
            return i;
        }
    }
    return state.text.size();
}
