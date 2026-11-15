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

#endif // PETRENDERER_CUSTOMMATH_H
