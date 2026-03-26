#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <windows.h>
#include "app.h"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int) {
    App::g_app = new App();

    if (!App::g_app->Initialize(hInstance)) {
        MessageBoxW(nullptr, L"Failed to initialize Pingy", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    App::g_app->Run();
    return 0;
}
