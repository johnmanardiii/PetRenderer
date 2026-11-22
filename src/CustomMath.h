//
// Created by john on 10/12/26.
//

#ifndef PETRENDERER_CUSTOMMATH_H
#define PETRENDERER_CUSTOMMATH_H
#include <numbers>
// #JOHN - classic namespace example...

inline float deg2rad(float deg)
{
    return deg * (std::numbers::pi / 180.0f);
}

// #JOHN - classic template situation just make it that later
inline float Clamp(float x, float min, float max)
{
    if (x < min)
    {
        return min;
    }
    if (x > max)
    {
        return max;
    }
    return x;
}

constexpr float pi = 3.1415926535897932384626433832795028841971693993751058209749445923078164062;

#endif // PETRENDERER_CUSTOMMATH_H
