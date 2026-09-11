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

    Vector3D& operator*=(float s)
    {
        x *= s;
        y *= s;
        z *= s;
        return *this;
    }

    Vector3D& operator/=(float s)
    {
        x /= s;
        y /= s;
        z /= s;
        return *this;
    }

    Vector3D& operator+=(const Vector3D& other)
    {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }

    Vector3D& operator-=(const Vector3D& other)
    {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        return *this;
    }
};

inline Vector3D operator*(const Vector3D& v, float s)
{
    return
    {
        v.x *s, v.y *s, v.z * s
    };
}

inline Vector3D operator*(float s, const Vector3D &v)
{
    return v * s;
}

inline Vector3D operator/(const Vector3D &v, float s)
{
    s = 1.0F / s;
    return v * s;
}

inline Vector3D operator-(const Vector3D& v)
{
    return {-v.x, -v.y, -v.z};
}

inline Vector3D operator+(const Vector3D& rhs, const Vector3D& lhs)
{
    return
    {
        rhs.x + lhs.x, rhs.y + lhs.y, rhs.z + lhs.z
    };
}

inline Vector3D operator-(const Vector3D &rhs, const Vector3D &lhs)
{
    return {rhs.x - lhs.x, rhs.y - lhs.y, rhs.z - lhs.z};
}

inline float Magnitude(const Vector3D& v)
{
    return (sqrt(v.x * v.x + v.y * v.y + v.z * v.z));
}

inline Vector3D Normalize(const Vector3D& v)
{
    return v / Magnitude(v);
}

// just making a little change for now will come back to this

 // modifying some files while i read the math book heh

// going over vector addition rn
