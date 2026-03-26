#pragma once
#include "renderer.h"
#include "ping_manager.h"
#include "layout.h"
#include "ui_text_input.h"

struct AddDialogState {
    bool visible = false;
    TextInputState hostInput;
    TextInputState nameInput;
    WStr errorText;
    bool hoverAdd = false;
    bool hoverCancel = false;

    void Show() {
        visible = true;
        hostInput.Clear();
        nameInput.Clear();
        hostInput.placeholder = L"e.g. 8.8.8.8 or example.com";
        nameInput.placeholder = L"e.g. My Server";
        errorText.clear();
        hostInput.focused = true;
        nameInput.focused = false;
    }
    void Hide() { visible = false; hostInput.focused = false; nameInput.focused = false; }
};

namespace UIAddDialog {
    void Draw(Renderer& r, AddDialogState& state, float windowW, float windowH,
              float dpiScale, ULONGLONG tickCount);
    // Returns: 0=nothing, 1=add, 2=cancel, 3=clicked outside (dismiss)
    int HitTest(AddDialogState& state, Renderer& r, float mx, float my,
                float windowW, float windowH, float dpiScale);
    // Returns true if target was added
    bool TryAdd(AddDialogState& state, PingManager& pm);
    // Get the focused input state, or nullptr
    TextInputState* FocusedInput(AddDialogState& state);
    void TabFocus(AddDialogState& state);
}
