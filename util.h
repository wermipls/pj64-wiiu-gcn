#include <math.h>

struct Vec2
{
    float x;
    float y;
};

// simple dz (no scale)
int deadzone(int val, int dz)
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

struct Vec2 circle_to_square(int x, int y)
{
    float euclidean = sqrtf(x*x + y*y);
    float manhattan = abs(x)+abs(y);

    float scale = manhattan / euclidean;

    struct Vec2 result;
    result.x = x * scale;
    result.y = y * scale;

    return result;
}
