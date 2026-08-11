// Minimal CRT replacement for demoscene-style builds
// Provides: entry point, heap allocation, memory intrinsics, math, formatting
//
// Yes, I reimplemented the C runtime. No, I am not sorry.
// Microsoft gave me a perfectly good CRT and I said "no thanks, I'll
// hand-roll memcpy like a fucking caveman." This file is what happens
// when executable size becomes a personality trait.
#include <winsock2.h>
#include <windows.h>
#include <objbase.h>

// Required by compiler when using floats
extern "C" int _fltused = 1;

// Declared rather than pulled in from shobjidl.h, which drags a pile of COM
// interface definitions into a translation unit that has no CRT. shell32.lib is
// already on the link line, so the import resolves.
extern "C" HRESULT WINAPI SetCurrentProcessExplicitAppUserModelID(PCWSTR appId);

// Global heap handle — initialized before anything else
extern "C" HANDLE g_heap = nullptr;

// Forward declare the real entry point
int WINAPI wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int);

// Custom entry point — skips all CRT initialization.
// I bypass the entire CRT startup sequence because those extra kilobytes
// were keeping me up at night. This is the "I'll do it myself" Thanos meme
// but for C++ runtime initialization.
#pragma comment(linker, "/ENTRY:_entry")
extern "C" void _entry() {
    g_heap = GetProcessHeap();

    // Initialize Winsock (needed for getaddrinfo in ping workers)
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    // Initialize COM (needed for Direct2D) — load dynamically because linking
    // ole32.lib like a normal person would be too dignified for this codebase
    typedef HRESULT (WINAPI *PFN_CoInitializeEx)(void*, DWORD);
    HMODULE hOle32 = LoadLibraryW(L"ole32.dll");
    if (hOle32) {
        PFN_CoInitializeEx pfn = (PFN_CoInitializeEx)GetProcAddress(hOle32, "CoInitializeEx");
        if (pfn) pfn(nullptr, 0x2 /*COINIT_APARTMENTTHREADED*/);
    }

    // Claim the same AppUserModelID the installer stamps on the Start Menu
    // shortcut. Started from that shortcut the identity is inherited, but
    // started by typing "Pingy" in a terminal, which is the entire point of the
    // PATH entry, there is no shortcut to inherit from and Windows derives an
    // identity from the exe path instead. The window then refuses to group
    // under the pinned icon and pinning it again pins a second one.
    //
    // Must match product.aumid in installer.toml. Check-Metadata.ps1 enforces
    // that in CI.
    SetCurrentProcessExplicitAppUserModelID(L"LockeWerks.Pingy");

    HINSTANCE h = GetModuleHandleW(nullptr);
    int r = wWinMain(h, nullptr, GetCommandLineW(), SW_SHOWDEFAULT);
    ExitProcess((UINT)r);
}

// Heap allocation — replaces CRT malloc/free/new/delete
void* operator new(size_t size) { return HeapAlloc(g_heap, HEAP_ZERO_MEMORY, size ? size : 1); }
void* operator new[](size_t size) { return HeapAlloc(g_heap, HEAP_ZERO_MEMORY, size ? size : 1); }
void operator delete(void* p) noexcept { if (p) HeapFree(g_heap, 0, p); }
void operator delete[](void* p) noexcept { if (p) HeapFree(g_heap, 0, p); }
void operator delete(void* p, size_t) noexcept { if (p) HeapFree(g_heap, 0, p); }
void operator delete[](void* p, size_t) noexcept { if (p) HeapFree(g_heap, 0, p); }

// Aligned new/delete for types the compiler decides need alignment
#include "containers.h"
void* operator new(size_t size, std::align_val_t) { return HeapAlloc(g_heap, HEAP_ZERO_MEMORY, size ? size : 1); }
void* operator new[](size_t size, std::align_val_t) { return HeapAlloc(g_heap, HEAP_ZERO_MEMORY, size ? size : 1); }
void operator delete(void* p, std::align_val_t) noexcept { if (p) HeapFree(g_heap, 0, p); }
void operator delete[](void* p, std::align_val_t) noexcept { if (p) HeapFree(g_heap, 0, p); }

// Memory intrinsics — the compiler expects these to exist because it assumes
// I haven't committed the war crime of removing the entire C runtime
extern "C" {
    #pragma function(memcpy)
    void* memcpy(void* dst, const void* src, size_t n) {
        char* d = (char*)dst; const char* s = (const char*)src;
        while (n--) *d++ = *s++;
        return dst;
    }

    #pragma function(memset)
    void* memset(void* dst, int val, size_t n) {
        char* d = (char*)dst;
        while (n--) *d++ = (char)val;
        return dst;
    }

    #pragma function(memmove)
    void* memmove(void* dst, const void* src, size_t n) {
        char* d = (char*)dst; const char* s = (const char*)src;
        if (d < s) { while (n--) *d++ = *s++; }
        else { d += n; s += n; while (n--) *--d = *--s; }
        return dst;
    }

    #pragma function(memcmp)
    int memcmp(const void* a, const void* b, size_t n) {
        const unsigned char *pa = (const unsigned char*)a, *pb = (const unsigned char*)b;
        while (n--) { if (*pa != *pb) return *pa - *pb; pa++; pb++; }
        return 0;
    }

    size_t strlen(const char* s) { const char* p = s; while (*p) p++; return p - s; }
    size_t wcslen(const wchar_t* s) { const wchar_t* p = s; while (*p) p++; return p - s; }

#ifdef _M_IX86
    // x86 CRT helpers for 64-bit math and float conversion.
    // Naked functions with inline assembly. I am in the trenches now.
    __declspec(naked) void __cdecl _aulldiv() {
        __asm {
            push ebx
            mov eax, [esp+16] ; high dword of divisor
            test eax, eax
            jnz short hard
            mov ecx, [esp+12] ; low dword of divisor
            mov eax, [esp+8]  ; high dword of dividend
            xor edx, edx
            div ecx
            mov ebx, eax
            mov eax, [esp+4]  ; low dword of dividend
            div ecx
            mov edx, ebx
            pop ebx
            ret 16
        hard:
            ; full 64/64 - simplified: just return 0 for now
            xor eax, eax
            xor edx, edx
            pop ebx
            ret 16
        }
    }

    __declspec(naked) void __cdecl _aullrem() {
        __asm {
            push ebx
            mov eax, [esp+16]
            test eax, eax
            jnz short hard2
            mov ecx, [esp+12]
            mov eax, [esp+8]
            xor edx, edx
            div ecx
            mov eax, [esp+4]
            div ecx
            mov eax, edx
            xor edx, edx
            pop ebx
            ret 16
        hard2:
            xor eax, eax
            xor edx, edx
            pop ebx
            ret 16
        }
    }

    // long to float conversion
    void __cdecl _ltof3() {
        // Compiler uses this for int64 -> float. Provide via x87 FPU.
        __asm {
            fild qword ptr [esp+4]
            ret
        }
    }
#endif

    int _purecall(void) { ExitProcess(3); return 0; }
    void __std_terminate() { ExitProcess(1); }

    // Thread-local storage index
    unsigned long _tls_index = 0;

    // Thread-safe static initialization
    // MSVC protocol for static local guard variables:
    //   guard == 0:  uninitialized
    //   guard == -1: fully initialized (skip construction)
    //   guard == 1:  initialization in progress
    // _Init_thread_header: if guard==0, set to 1 (init time). if guard==-1, skip (set to -1 so caller skips).
    // _Init_thread_footer: set guard to -1 (construction complete)
    int _Init_thread_epoch = 0;
    void _Init_thread_header(int* pOnce) {
        // If already initialized, tell caller to skip by leaving *pOnce == -1
        if (*pOnce == -1) return;
        // Not yet initialized: set to 1 (in-progress), caller will construct
        // After construction, compiler calls _Init_thread_footer
        *pOnce = 1;
    }
    void _Init_thread_footer(int* pOnce) {
        // Mark as fully initialized
        *pOnce = -1;
    }

    // atexit — cleanup is for people who allocate responsibly.
    // I just call ExitProcess and let the OS sort it out. Godspeed.
    int atexit(void (*)(void)) { return 0; }
}

// Math — polynomial/Padé approximations for trig functions.
// Are these as accurate as the CRT versions? No. Do they fit in fewer bytes? Yes.
// That's my whole value system here.
extern "C" {
    float tanf(float x) {
        float x2 = x * x;
        return x * (15.0f - x2) / (15.0f - 6.0f * x2);
    }

    float atanf(float x) {
        if (x > 1.0f) return 1.5707963f - atanf(1.0f / x);
        if (x < -1.0f) return -1.5707963f - atanf(1.0f / x);
        float x2 = x * x;
        return x - x * x2 / 3.0f + x * x2 * x2 / 5.0f - x * x2 * x2 * x2 / 7.0f;
    }

    // Newton's method: iterate until "close enough" (4 iterations, take it or leave it)
    float sqrtf(float x) {
        if (x <= 0) return 0;
        float g = x * 0.5f;
        g = 0.5f * (g + x / g);
        g = 0.5f * (g + x / g);
        g = 0.5f * (g + x / g);
        g = 0.5f * (g + x / g);
        return g;
    }

    float fabsf(float x) { return x < 0 ? -x : x; }

    float fmodf(float a, float b) {
        if (b == 0) return 0;
        return a - (float)(int)(a / b) * b;
    }

    float cosf(float x) {
        float x2 = x * x;
        return 1.0f - x2 / 2.0f + x2 * x2 / 24.0f - x2 * x2 * x2 / 720.0f;
    }

    float sinf(float x) {
        float x2 = x * x;
        return x - x * x2 / 6.0f + x * x2 * x2 / 120.0f - x * x2 * x2 * x2 / 5040.0f;
    }

    double tan(double x) { return (double)tanf((float)x); }
    double atan(double x) { return (double)atanf((float)x); }
    double sqrt(double x) { return (double)sqrtf((float)x); }
    double fabs(double x) { return (double)fabsf((float)x); }
    double fmod(double a, double b) { return (double)fmodf((float)a, (float)b); }
    double cos(double x) { return (double)cosf((float)x); }
    double sin(double x) { return (double)sinf((float)x); }
}
