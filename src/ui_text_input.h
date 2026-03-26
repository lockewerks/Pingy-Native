#pragma once
#include <d2d1.h>
#include "containers.h"
#include "renderer.h"

struct TextInputState {
    WStr text;
    int cursorPos = 0;
    bool focused = false;
    WStr placeholder;

    void InsertChar(wchar_t ch);
    void Backspace();
    void Delete();
    void MoveCursor(int delta);
    void Home();
    void End();
    void SelectAll();
    void Clear();
    void PasteFromClipboard(HWND hwnd);
};

namespace UITextInput {
    void Draw(Renderer& r, const TextInputState& state, const D2D1_RECT_F& rect,
              const D2D1_COLOR_F& bgColor, float dpiScale, ULONGLONG tickCount);
    int HitTestCursor(Renderer& r, const TextInputState& state, const D2D1_RECT_F& rect,
                      float clickX, float dpiScale);
}
