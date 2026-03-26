#include <windows.h>
extern "C" int _fltused = 1;
extern "C" HANDLE g_heap = nullptr;
void* operator new(size_t s) { return HeapAlloc(g_heap, HEAP_ZERO_MEMORY, s?s:1); }
void* operator new[](size_t s) { return HeapAlloc(g_heap, HEAP_ZERO_MEMORY, s?s:1); }
void operator delete(void* p) noexcept { if(p) HeapFree(g_heap,0,p); }
void operator delete[](void* p) noexcept { if(p) HeapFree(g_heap,0,p); }
void operator delete(void* p, size_t) noexcept { if(p) HeapFree(g_heap,0,p); }
void operator delete[](void* p, size_t) noexcept { if(p) HeapFree(g_heap,0,p); }


extern "C" {
    #pragma function(memcpy)
    void* memcpy(void* d, const void* s, size_t n) { char* dd=(char*)d; const char* ss=(const char*)s; while(n--) *dd++=*ss++; return d; }
    #pragma function(memset)
    void* memset(void* d, int v, size_t n) { char* dd=(char*)d; while(n--) *dd++=(char)v; return d; }
    #pragma function(memmove)
    void* memmove(void* d, const void* s, size_t n) { char* dd=(char*)d; const char* ss=(const char*)s; if(dd<ss){while(n--)*dd++=*ss++;}else{dd+=n;ss+=n;while(n--)*--dd=*--ss;} return d; }
    #pragma function(memcmp)
    int memcmp(const void* a, const void* b, size_t n) { const unsigned char *pa=(const unsigned char*)a,*pb=(const unsigned char*)b; while(n--){if(*pa!=*pb)return *pa-*pb;pa++;pb++;}return 0; }
    size_t wcslen(const wchar_t* s) { const wchar_t* p=s; while(*p)p++; return p-s; }
    int atexit(void(*)(void)) { return 0; }
    int _Init_thread_epoch = 0;
    void _Init_thread_header(int* p) { if(*p==-1)return; *p=1; }
    void _Init_thread_footer(int* p) { *p=-1; }
    unsigned long _tls_index = 0;
    int _purecall(void) { return 0; }
    void __std_terminate() { ExitProcess(1); }
    float fabsf(float x) { return x<0?-x:x; }
}
#include "containers.h"
#include "data.h"

#pragma comment(linker, "/ENTRY:testmain")
extern "C" void testmain() {
    g_heap = GetProcessHeap();
    
    MessageBoxW(nullptr, L"Starting test...", L"Test", MB_OK);
    
    // Test WStr
    WStr s1(L"hello world");
    MessageBoxW(nullptr, s1.c_str(), L"WStr test", MB_OK);
    
    // Test PingTarget
    PingTarget target;
    target.host = L"8.8.8.8";
    target.displayName = L"Test";
    target.InitializeHistory(10);
    MessageBoxW(nullptr, L"PingTarget created", L"Test", MB_OK);
    
    // Test Vec<PingTarget> push
    Vec<PingTarget> targets;
    targets.push_back(static_cast<PingTarget&&>(target));
    MessageBoxW(nullptr, L"push_back 1 done", L"Test", MB_OK);
    
    PingTarget t2;
    t2.host = L"1.1.1.1";
    t2.displayName = L"CF";
    t2.InitializeHistory(10);
    targets.push_back(static_cast<PingTarget&&>(t2));
    MessageBoxW(nullptr, L"push_back 2 done", L"Test", MB_OK);
    
    // Force realloc
    for (int i = 0; i < 10; i++) {
        PingTarget ti;
        wchar_t buf[32];
        wsprintfW(buf, L"T%d", i);
        ti.host = WStr(buf);
        ti.displayName = WStr(buf);
        ti.InitializeHistory(10);
        targets.push_back(static_cast<PingTarget&&>(ti));
    }
    MessageBoxW(nullptr, L"All pushes done!", L"Test", MB_OK);
    
    ExitProcess(0);
}
