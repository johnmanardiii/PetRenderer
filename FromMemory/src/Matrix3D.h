//
// Created by johnm on 10/1/2026.
//

#ifndef MATRIX3D_H
#define MATRIX3D_H
#include "Vector3D.h"

struct Matrix3D
{
private:
    float n[3][3];
public:

    Matrix3D() = default;
    Matrix3D(float n00, float n01, float n02,
        float n10, float n11, float n12,
        float n20, float n21, float n22)
    {
        // row / column are switched because we are storing in column major order
        n[0][0] = n00;
        n[0][1] = n10;
        n[0][2] = n20;

        n[1][0] = n01;
        n[1][1] = n11;
        n[1][2] = n21;

        n[2][0] = n02;
        n[2][1] = n12;
        n[2][2] = n22;
    }

    Matrix3D(const Vector3D& a, const Vector3D& b, const Vector3D& c)
    {
        // row / column are switched because we are storing in column major order
        n[0][0] = a.x;
        n[0][1] = a.y;
        n[0][2] = a.z;

        n[1][0] = b.x;
        n[1][1] = b.y;
        n[1][2] = b.z;

        n[2][0] = c.x;
        n[2][1] = c.y;
        n[2][2] = c.z;
    }

    float& operator()(int i, int j)
    {
        return n[j][i];
    }

    const float& operator()(int i, int j) const
    {
        return n[j][i];
    }

    Vector3D& operator[](int col)
    {
        return *reinterpret_cast<Vector3D *>(n[col]);
    }

    const Vector3D& operator[](int col) const
    {
        return *reinterpret_cast<const Vector3D *>(n[col]);
    }
};

Matrix3D operator*(const Matrix3D& A, const Matrix3D& B)
{
    return Matrix3D(
        A(0, 0) * B(0, 0) + A(0, 1) * B(1, 0) + A(0, 2) * B(2, 0),
        A(0, 0) * B(0, 1) + A(0, 1) * B(1, 1) + A(0, 2) * B(2, 1),
        A(0, 0) * B(0, 2) + A(0, 1) * B(1, 2) + A(0, 2) * B(2, 2),

        A(1, 0) * B(0, 0) + A(1, 1) * B(1, 0) + A(1, 2) * B(2, 0),
        A(1, 0) * B(0, 1) + A(1, 1) * B(1, 1) + A(1, 2) * B(2, 1),
        A(1, 0) * B(0, 2) + A(1, 1) * B(1, 2) + A(1, 2) * B(2, 2),

        A(2, 0) * B(0, 0) + A(2, 1) * B(1, 0) + A(2, 2) * B(2, 0),
        A(2, 0) * B(0, 1) + A(2, 1) * B(1, 1) + A(2, 2) * B(2, 1),
        A(2, 0) * B(0, 2) + A(2, 1) * B(1, 2) + A(2, 2) * B(2, 2)
    );
}

Vector3D operator*(const Matrix3D& M, const Vector3D& v)
{
    return Vector3D(
        v.x * M(0, 0) + v.y * M(0, 1) + v.z * M(0, 2),
        v.x * M(1, 0) + v.y * M(1, 1) + v.z * M(1, 2),
        v.x * M(2, 0) + v.y * M(2, 1) + v.z * M(2, 2)
    );
}

// for later slang stuff: docs.shader-slang.org/en/latest/external/slang/docs/user-guide/a1-01-matrix-layout.html
// docs.vulkan.org/guide/latest/shader_memory_layout.html
//docs.vulkan.org/spec/latest/chapters/interfaces.html#interfaces-resources-layout
//docs.shader-slang.org/en/latest/external/slang/docs/command-line-slangc-reference.html
// fvk-use-scalar-layout

#endif //MATRIX3D_H
