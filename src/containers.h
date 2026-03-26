#pragma once
// Minimal STL replacements — no CRT dependency, uses HeapAlloc.
// I looked at the STL. I saw std::vector, std::string, std::wstring.
// I said "I can do that" with the confidence of a man who has never
// had to debug a custom allocator at 3am. And yet, here I am.
#include <windows.h>

// g_heap initialized in crt_mini.cpp _entry() before anything else runs
extern "C" HANDLE g_heap;
inline void* _halloc(size_t n) { return HeapAlloc(g_heap, HEAP_ZERO_MEMORY, n ? n : 1); }
inline void _hfree(void* p) { if (p) HeapFree(g_heap, 0, p); }

// Placement new — dark magic required because I banished the CRT and now
// must summon its ghosts manually in every translation unit.
inline void* operator new(size_t, void* p) noexcept { return p; }
inline void* operator new[](size_t, void* p) noexcept { return p; }
namespace std { enum class align_val_t : size_t {}; } // Fake std namespace. The standards committee can bill me.
inline void* operator new(size_t, std::align_val_t, void* p) noexcept { return p; }

// ============ WStr — wide string ============
class WStr {
    wchar_t* d_ = nullptr;
    int len_ = 0;
    int cap_ = 0;

    void _grow(int need) {
        if (need <= cap_) return;
        int nc = cap_ ? cap_ * 2 : 16;
        while (nc < need) nc *= 2;
        wchar_t* nd = (wchar_t*)_halloc((nc + 1) * sizeof(wchar_t));
        if (d_) { for (int i = 0; i < len_; i++) nd[i] = d_[i]; _hfree(d_); }
        nd[len_] = 0;
        d_ = nd; cap_ = nc;
    }
public:
    WStr() : d_(nullptr), len_(0), cap_(0) {}
    WStr(const wchar_t* s) : d_(nullptr), len_(0), cap_(0) { if (s) { int n = 0; while (s[n]) n++; _grow(n); for (int i = 0; i < n; i++) d_[i] = s[i]; len_ = n; d_[len_] = 0; } }
    WStr(const wchar_t* s, int n) : d_(nullptr), len_(0), cap_(0) { _grow(n); for (int i = 0; i < n; i++) d_[i] = s[i]; len_ = n; d_[len_] = 0; }
    WStr(const WStr& o) : d_(nullptr), len_(0), cap_(0) { if (o.len_ > 0) { _grow(o.len_); for (int i = 0; i < o.len_; i++) d_[i] = o.d_[i]; len_ = o.len_; d_[len_] = 0; } }
    WStr(WStr&& o) noexcept : d_(o.d_), len_(o.len_), cap_(o.cap_) { o.d_ = nullptr; o.len_ = 0; o.cap_ = 0; }
    ~WStr() { _hfree(d_); }

    WStr& operator=(const WStr& o) { if (this != &o) { _grow(o.len_); for (int i = 0; i < o.len_; i++) d_[i] = o.d_[i]; len_ = o.len_; d_[len_] = 0; } return *this; }
    WStr& operator=(WStr&& o) noexcept { _hfree(d_); d_ = o.d_; len_ = o.len_; cap_ = o.cap_; o.d_ = nullptr; o.len_ = 0; o.cap_ = 0; return *this; }
    WStr& operator=(const wchar_t* s) { *this = WStr(s); return *this; }

    const wchar_t* c_str() const { return d_ ? d_ : L""; }
    int size() const { return len_; }
    bool empty() const { return len_ == 0; }
    wchar_t& operator[](int i) { return d_[i]; }
    wchar_t operator[](int i) const { return d_[i]; }
    wchar_t front() const { return d_[0]; }
    wchar_t back() const { return d_[len_ - 1]; }

    void clear() { len_ = 0; if (d_) d_[0] = 0; }
    void pop_back() { if (len_ > 0) { len_--; d_[len_] = 0; } }

    WStr& operator+=(wchar_t ch) { _grow(len_ + 1); d_[len_++] = ch; d_[len_] = 0; return *this; }
    WStr& operator+=(const wchar_t* s) { int n = 0; while (s[n]) n++; _grow(len_ + n); for (int i = 0; i < n; i++) d_[len_ + i] = s[i]; len_ += n; d_[len_] = 0; return *this; }
    WStr& operator+=(const WStr& o) { _grow(len_ + o.len_); for (int i = 0; i < o.len_; i++) d_[len_ + i] = o.d_[i]; len_ += o.len_; d_[len_] = 0; return *this; }

    WStr operator+(const WStr& o) const { WStr r(*this); r += o; return r; }
    WStr operator+(const wchar_t* s) const { WStr r(*this); r += s; return r; }

    bool operator==(const WStr& o) const { if (len_ != o.len_) return false; for (int i = 0; i < len_; i++) if (d_[i] != o.d_[i]) return false; return true; }
    bool operator!=(const WStr& o) const { return !(*this == o); }

    void insert(int pos, wchar_t ch) { _grow(len_ + 1); for (int i = len_; i > pos; i--) d_[i] = d_[i-1]; d_[pos] = ch; len_++; d_[len_] = 0; }
    void insert(int pos, const WStr& s) { _grow(len_ + s.len_); for (int i = len_ - 1; i >= pos; i--) d_[i + s.len_] = d_[i]; for (int i = 0; i < s.len_; i++) d_[pos + i] = s.d_[i]; len_ += s.len_; d_[len_] = 0; }
    void erase(int pos, int count = 1) { if (pos >= len_) return; if (pos + count > len_) count = len_ - pos; for (int i = pos; i + count < len_; i++) d_[i] = d_[i + count]; len_ -= count; d_[len_] = 0; }

    int find(const wchar_t* s, int from = 0) const {
        int slen = 0; while (s[slen]) slen++;
        for (int i = from; i <= len_ - slen; i++) { bool ok = true; for (int j = 0; j < slen; j++) if (d_[i+j] != s[j]) { ok = false; break; } if (ok) return i; }
        return -1;
    }
    int find(wchar_t ch, int from = 0) const { for (int i = from; i < len_; i++) if (d_[i] == ch) return i; return -1; }

    WStr substr(int pos, int count = -1) const {
        if (pos >= len_) return {};
        if (count < 0 || pos + count > len_) count = len_ - pos;
        return WStr(d_ + pos, count);
    }
};

inline WStr operator+(const wchar_t* a, const WStr& b) { WStr r(a); r += b; return r; }

// ============ Str — narrow string ============
class Str {
    char* d_ = nullptr;
    int len_ = 0;
    int cap_ = 0;

    void _grow(int need) {
        if (need <= cap_) return;
        int nc = cap_ ? cap_ * 2 : 32;
        while (nc < need) nc *= 2;
        char* nd = (char*)_halloc(nc + 1);
        if (d_) { for (int i = 0; i < len_; i++) nd[i] = d_[i]; _hfree(d_); }
        nd[len_] = 0;
        d_ = nd; cap_ = nc;
    }
public:
    Str() : d_(nullptr), len_(0), cap_(0) {}
    Str(const char* s) : d_(nullptr), len_(0), cap_(0) { if (s) { int n = 0; while (s[n]) n++; _grow(n); for (int i = 0; i < n; i++) d_[i] = s[i]; len_ = n; d_[len_] = 0; } }
    Str(const char* s, int n) : d_(nullptr), len_(0), cap_(0) { _grow(n); for (int i = 0; i < n; i++) d_[i] = s[i]; len_ = n; d_[len_] = 0; }
    Str(int n, char ch) : d_(nullptr), len_(0), cap_(0) { _grow(n); for (int i = 0; i < n; i++) d_[i] = ch; len_ = n; d_[len_] = 0; }
    Str(const Str& o) : d_(nullptr), len_(0), cap_(0) { _grow(o.len_); for (int i = 0; i < o.len_; i++) d_[i] = o.d_[i]; len_ = o.len_; d_[len_] = 0; }
    Str(Str&& o) noexcept : d_(o.d_), len_(o.len_), cap_(o.cap_) { o.d_ = nullptr; o.len_ = 0; o.cap_ = 0; }
    ~Str() { _hfree(d_); }

    Str& operator=(const Str& o) { if (this != &o) { _grow(o.len_); for (int i = 0; i < o.len_; i++) d_[i] = o.d_[i]; len_ = o.len_; d_[len_] = 0; } return *this; }
    Str& operator=(Str&& o) noexcept { _hfree(d_); d_ = o.d_; len_ = o.len_; cap_ = o.cap_; o.d_ = nullptr; o.len_ = 0; o.cap_ = 0; return *this; }

    const char* c_str() const { return d_ ? d_ : ""; }
    char* data() { return d_; }
    int size() const { return len_; }
    bool empty() const { return len_ == 0; }
    char& operator[](int i) { return d_[i]; }
    char operator[](int i) const { return d_[i]; }
    char back() const { return d_[len_ - 1]; }
    void pop_back() { if (len_ > 0) { len_--; d_[len_] = 0; } }
    void clear() { len_ = 0; if (d_) d_[0] = 0; }
    void resize(int n) { _grow(n); if (n > len_) for (int i = len_; i < n; i++) d_[i] = 0; len_ = n; d_[len_] = 0; }

    Str& operator+=(char ch) { _grow(len_ + 1); d_[len_++] = ch; d_[len_] = 0; return *this; }
    Str& operator+=(const char* s) { int n = 0; while (s[n]) n++; _grow(len_ + n); for (int i = 0; i < n; i++) d_[len_ + i] = s[i]; len_ += n; d_[len_] = 0; return *this; }
    Str& operator+=(const Str& o) { _grow(len_ + o.len_); for (int i = 0; i < o.len_; i++) d_[len_ + i] = o.d_[i]; len_ += o.len_; d_[len_] = 0; return *this; }

    Str operator+(const char* s) const { Str r(*this); r += s; return r; }
    Str operator+(const Str& o) const { Str r(*this); r += o; return r; }

    int find(const char* s, int from = 0) const {
        int slen = 0; while (s[slen]) slen++;
        for (int i = from; i <= len_ - slen; i++) { bool ok = true; for (int j = 0; j < slen; j++) if (d_[i+j] != s[j]) { ok = false; break; } if (ok) return i; }
        return -1;
    }
    int find(char ch, int from = 0) const { for (int i = from; i < len_; i++) if (d_[i] == ch) return i; return -1; }
    Str substr(int pos, int count = -1) const {
        if (pos >= len_) return {};
        if (count < 0 || pos + count > len_) count = len_ - pos;
        return Str(d_ + pos, count);
    }
};

// ============ Vec<T> — dynamic array ============
// It's std::vector at home. Placement new, manual destructors, move semantics
// via raw casts — all the things your CS professor warned you about.
template<typename T>
class Vec {
    T* d_ = nullptr;
    int len_ = 0;
    int cap_ = 0;

    void _grow(int need) {
        if (need <= cap_) return;
        int nc = cap_ ? cap_ * 2 : 8;
        while (nc < need) nc *= 2;
        T* nd = (T*)_halloc(nc * sizeof(T));
        for (int i = 0; i < len_; i++) new (&nd[i]) T(static_cast<T&&>(d_[i]));
        for (int i = 0; i < len_; i++) d_[i].~T();
        _hfree(d_);
        d_ = nd; cap_ = nc;
    }
public:
    Vec() : d_(nullptr), len_(0), cap_(0) {}
    Vec(const Vec& o) : d_(nullptr), len_(0), cap_(0) { _grow(o.len_); for (int i = 0; i < o.len_; i++) new (&d_[i]) T(o.d_[i]); len_ = o.len_; }
    Vec(Vec&& o) noexcept : d_(o.d_), len_(o.len_), cap_(o.cap_) { o.d_ = nullptr; o.len_ = 0; o.cap_ = 0; }
    ~Vec() { clear(); _hfree(d_); }

    Vec& operator=(const Vec& o) { if (this != &o) { clear(); _grow(o.len_); for (int i = 0; i < o.len_; i++) new (&d_[i]) T(o.d_[i]); len_ = o.len_; } return *this; }
    Vec& operator=(Vec&& o) noexcept { clear(); _hfree(d_); d_ = o.d_; len_ = o.len_; cap_ = o.cap_; o.d_ = nullptr; o.len_ = 0; o.cap_ = 0; return *this; }

    void reserve(int n) { _grow(n); }
    void push_back(const T& v) { _grow(len_ + 1); new (&d_[len_++]) T(v); }
    void push_back(T&& v) { _grow(len_ + 1); new (&d_[len_++]) T(static_cast<T&&>(v)); }

    void erase(int i) {
        if (i < 0 || i >= len_) return;
        d_[i].~T();
        for (int j = i; j < len_ - 1; j++) {
            new (&d_[j]) T(static_cast<T&&>(d_[j+1]));
            d_[j+1].~T();
        }
        len_--;
    }
    void clear() { for (int i = 0; i < len_; i++) d_[i].~T(); len_ = 0; }
    void resize(int n) { if (n > len_) { _grow(n); for (int i = len_; i < n; i++) new (&d_[i]) T(); len_ = n; } else { for (int i = n; i < len_; i++) d_[i].~T(); len_ = n; } }

    int size() const { return len_; }
    bool empty() const { return len_ == 0; }
    T& operator[](int i) { return d_[i]; }
    const T& operator[](int i) const { return d_[i]; }
    T& back() { return d_[len_ - 1]; }
    const T& back() const { return d_[len_ - 1]; }
    T* data() { return d_; }
    const T* data() const { return d_; }

    T* begin() { return d_; }
    T* end() { return d_ + len_; }
    const T* begin() const { return d_; }
    const T* end() const { return d_ + len_; }
};

// ============ Formatting helpers ============
// Replace swprintf/std::to_string without CRT.
// sprintf is like 8KB of code. I need to print a number. This is fine.
inline Str IntToStr(int v) {
    char buf[16]; int i = 15; buf[i] = 0;
    bool neg = v < 0; if (neg) v = -v;
    if (v == 0) { buf[--i] = '0'; }
    else { while (v > 0) { buf[--i] = '0' + (v % 10); v /= 10; } }
    if (neg) buf[--i] = '-';
    return Str(&buf[i]);
}

inline void FmtFloat(wchar_t* buf, int bufSize, float v, int decimals) {
    // Simple float formatter: handles %.0f and %.1f cases
    bool neg = v < 0; if (neg) v = -v;
    int whole = (int)v;
    int frac = 0;
    if (decimals == 1) frac = (int)((v - whole) * 10.0f + 0.5f);
    else if (decimals == 0) { if (v - whole >= 0.5f) whole++; }

    // Handle carry
    if (decimals == 1 && frac >= 10) { whole++; frac -= 10; }

    int pos = 0;
    if (neg && (whole > 0 || frac > 0)) buf[pos++] = L'-';

    // Write whole part
    wchar_t tmp[16]; int ti = 0;
    if (whole == 0) tmp[ti++] = L'0';
    else { while (whole > 0) { tmp[ti++] = L'0' + (whole % 10); whole /= 10; } }
    for (int i = ti - 1; i >= 0 && pos < bufSize - 4; i--) buf[pos++] = tmp[i];

    if (decimals > 0) {
        buf[pos++] = L'.';
        buf[pos++] = L'0' + frac;
    }
    buf[pos] = 0;
}
