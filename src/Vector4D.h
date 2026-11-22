//
// Created by johnm on 11/21/2026.
//

#ifndef VECTOR4D_H
#define VECTOR4D_H

#include "Vector3D.h"

struct Vector4D {
    float x, y, z, w;

    Vector4D() = default;

    Vector4D(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}

    Vector4D(Vector3D v, float w = 1.0f) : x(v.x), y(v.y), z(v.z), w(w) {}

    float &operator[](int i) {
        return ((&x)[i]);
    }

    const float &operator[](int i) const {
        return ((&x)[i]);
    }

    Vector4D &operator*=(float s) {
        x *= s;
        y *= s;
        z *= s;
        w *= s;
        return *this;
    }

    operator Vector3D() const
    {
        return {x, y, z};
    }
};

inline Vector4D operator*(const Vector4D &v, float s) {
    return
        {
                v.x * s, v.y * s, v.z * s, v.w * s
        };
}

inline Vector4D operator*(float s, const Vector4D &v)
{
    return v * s;
}

#endif //VECTOR4D_H
