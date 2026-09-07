#pragma once

// basing this on that one eric lengyel book

struct Vector3D
{
    float x, y, z;

    Vector3D() = default;

    Vector3D(float x, float y, float z) : x(x), y(y), z(z)
    {

    }

    float& operator[](int i)
    {
        return ((&x)[i]);
    }

    const float &operator[](int i) const
    {
        return ((&x)[i]);
    }
};