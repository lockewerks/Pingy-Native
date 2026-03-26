#pragma once
#include <d2d1.h>

// Math functions provided by crt_mini.cpp at link time
extern "C" float tanf(float);
extern "C" float atanf(float);
extern "C" float sinf(float);
extern "C" float cosf(float);
extern "C" float sqrtf(float);
extern "C" float fabsf(float);
extern "C" float fmodf(float, float);

constexpr float PI = 3.14159265358979f;
constexpr float DEG2RAD = PI / 180.0f;

struct Vec3 {
    float x, y, z;
};

inline Vec3 CatmullRom(Vec3 p0, Vec3 p1, Vec3 p2, Vec3 p3, float t) {
    float t2 = t * t, t3 = t2 * t;
    return {
        0.5f * ((2*p1.x) + (-p0.x+p2.x)*t + (2*p0.x-5*p1.x+4*p2.x-p3.x)*t2 + (-p0.x+3*p1.x-3*p2.x+p3.x)*t3),
        0.5f * ((2*p1.y) + (-p0.y+p2.y)*t + (2*p0.y-5*p1.y+4*p2.y-p3.y)*t2 + (-p0.y+3*p1.y-3*p2.y+p3.y)*t3),
        0.5f * ((2*p1.z) + (-p0.z+p2.z)*t + (2*p0.z-5*p1.z+4*p2.z-p3.z)*t2 + (-p0.z+3*p1.z-3*p2.z+p3.z)*t3),
    };
}

inline bool PointInRect(float px, float py, const D2D1_RECT_F& r) {
    return px >= r.left && px <= r.right && py >= r.top && py <= r.bottom;
}

inline float Clamp(float v, float lo, float hi) {
    float t = (v < lo) ? lo : v;
    return (t > hi) ? hi : t;
}

inline float Lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

inline D2D1_COLOR_F ColorWithAlpha(D2D1_COLOR_F c, float a) {
    return {c.r, c.g, c.b, a};
}

inline D2D1_COLOR_F ColorScale(D2D1_COLOR_F c, float s) {
    return {c.r * s, c.g * s, c.b * s, c.a};
}

inline float RectWidth(const D2D1_RECT_F& r) { return r.right - r.left; }
inline float RectHeight(const D2D1_RECT_F& r) { return r.bottom - r.top; }
