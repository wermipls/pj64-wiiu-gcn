#pragma once

#include <math.h>

struct Vec2
{
    float x;
    float y;
};

// simple dz (no scale)
static inline int deadzone(int val, int dz)
{
    if (val > 0) {
        val -= dz;
        if (val < 0) return 0;
    }
    if (val < 0) {
        val += dz;
        if (val > 0) return 0;
    }

    return val;
}

static inline struct Vec2 circle_to_square(int x, int y)
{
    float euclidean = sqrtf(x*x + y*y);
    float manhattan = abs(x)+abs(y);

    float scale = manhattan / euclidean;

    struct Vec2 result;
    result.x = x * scale;
    result.y = y * scale;

    return result;
}

static inline int clamp(int val, int min, int max)
{
    if (val < min) return min;
    if (val > max) return max;
    return val;
}

static inline int smin(int a, int b)
{
    if (a > b) {
        return b;
    } else {
        return a;
    }
}

static inline int smax(int a, int b)
{
    if (a < b) {
        return b;
    } else {
        return a;
    }
}
