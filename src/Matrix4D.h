//
// Created by john on 10/4/26.
//

#ifndef PETRENDERER_MATRIX4D_H
#define PETRENDERER_MATRIX4D_H

// making this now because I need to pass a matrix to a shader and it seems to have complications if not
// 16 byte aligned...
// column-major just like Matrix3D
#include "Matrix3D.h" // not sure if including this is a good idea but yolo
#include "Vector4D.h"

struct Matrix4D
{
  private:
    float n[4][4];

  public:
    Matrix4D() = default;

    Matrix4D(float n00, float n01, float n02, float n03, float n10, float n11, float n12, float n13, float n20,
             float n21, float n22, float n23, float n30, float n31, float n32, float n33)
    {
        // row / column are switched because we are storing in column major order
        n[0][0] = n00;
        n[0][1] = n10;
        n[0][2] = n20;
        n[0][3] = n30;

        n[1][0] = n01;
        n[1][1] = n11;
        n[1][2] = n21;
        n[1][3] = n31;

        n[2][0] = n02;
        n[2][1] = n12;
        n[2][2] = n22;
        n[2][3] = n32;

        n[3][0] = n03;
        n[3][1] = n13;
        n[3][2] = n23;
        n[3][3] = n33;
    }

    // #JOHN - not sure if this is efficient but yolo
    Matrix4D(const Matrix3D &other)
    {
        // FromMemory/docs/matrix-gpu-upload-and-layout.md
        //  return Matrix4D();
        n[0][0] = other(0, 0);
        n[0][1] = other(1, 0);
        n[0][2] = other(2, 0);
        n[0][3] = 0.0;

        n[1][0] = other(0, 1);
        n[1][1] = other(1, 1);
        n[1][2] = other(2, 1);
        n[1][3] = 0.0;

        n[2][0] = other(0, 2);
        n[2][1] = other(1, 2);
        n[2][2] = other(2, 2);
        n[2][3] = 0.0;

        n[3][0] = 0.0;
        n[3][1] = 0.0;
        n[3][2] = 0.0;
        n[3][3] = 1.0;
    }

    float &operator()(int i, int j)
    {
        return n[j][i];
    }

    const float &operator()(int i, int j) const
    {
        return n[j][i];
    }

    static Matrix4D MakeRotationX(float rad)
    {
        float c = cos(rad);
        float s = sin(rad);

        return Matrix4D(1, 0, 0, 0,
                        0, c, -s, 0,
                        0, s, c, 0,
                        0, 0, 0, 1);

    }

    static Matrix4D MakeRotationY(float rad)
    {
        float c = cos(rad);
        float s = sin(rad);
        return Matrix4D(c, 0.0f, s, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, -s, 0.0f, c, 0.0f, 0.0, 0.0, 0.0, 1.0);
    }

    static Matrix4D MakeRotationZ(float rad)
    {
        float c = cos(rad);
        float s = sin(rad);
        return Matrix4D(c, -s, 0, 0, s, c, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
    }

    static Matrix4D Translate(const Vector3D &v)
    {
        return Matrix4D(1, 0, 0, v.x, 0, 1, 0, v.y, 0, 0, 1, v.z, 0, 0, 0, 1);
    }

    static Matrix4D Scale(const Vector3D &v)
    {
        return Matrix4D(v.x, 0, 0, 0, 0, v.y, 0, 0, 0, 0, v.z, 0, 0, 0, 0, 1);
    }

    // uniform scale
    static Matrix4D Scale(float s)
    {
        return Matrix4D(s, 0, 0, 0, 0, s, 0, 0, 0, 0, s, 0, 0, 0, 0, 1);
    }

    Vector4D& operator[](int col)
    {
        return *reinterpret_cast<Vector4D *>(n[col]);
    }

    const Vector4D& operator[](int col) const
    {
        return *reinterpret_cast<const Vector4D *>(n[col]);
    }
};

inline Matrix4D operator*(const Matrix4D &A, const Matrix4D &B)
{
    return Matrix4D(A(0, 0) * B(0, 0) + A(0, 1) * B(1, 0) + A(0, 2) * B(2, 0) + A(0, 3) * B(3, 0),
                    A(0, 0) * B(0, 1) + A(0, 1) * B(1, 1) + A(0, 2) * B(2, 1) + A(0, 3) * B(3, 1),
                    A(0, 0) * B(0, 2) + A(0, 1) * B(1, 2) + A(0, 2) * B(2, 2) + A(0, 3) * B(3, 2),
                    A(0, 0) * B(0, 3) + A(0, 1) * B(1, 3) + A(0, 2) * B(2, 3) + A(0, 3) * B(3, 3),

                    A(1, 0) * B(0, 0) + A(1, 1) * B(1, 0) + A(1, 2) * B(2, 0) + A(1, 3) * B(3, 0),
                    A(1, 0) * B(0, 1) + A(1, 1) * B(1, 1) + A(1, 2) * B(2, 1) + A(1, 3) * B(3, 1),
                    A(1, 0) * B(0, 2) + A(1, 1) * B(1, 2) + A(1, 2) * B(2, 2) + A(1, 3) * B(3, 2),
                    A(1, 0) * B(0, 3) + A(1, 1) * B(1, 3) + A(1, 2) * B(2, 3) + A(1, 3) * B(3, 3),

                    A(2, 0) * B(0, 0) + A(2, 1) * B(1, 0) + A(2, 2) * B(2, 0) + A(2, 3) * B(3, 0),
                    A(2, 0) * B(0, 1) + A(2, 1) * B(1, 1) + A(2, 2) * B(2, 1) + A(2, 3) * B(3, 1),
                    A(2, 0) * B(0, 2) + A(2, 1) * B(1, 2) + A(2, 2) * B(2, 2) + A(2, 3) * B(3, 2),
                    A(2, 0) * B(0, 3) + A(2, 1) * B(1, 3) + A(2, 2) * B(2, 3) + A(2, 3) * B(3, 3),

                    A(3, 0) * B(0, 0) + A(3, 1) * B(1, 0) + A(3, 2) * B(2, 0) + A(3, 3) * B(3, 0),
                    A(3, 0) * B(0, 1) + A(3, 1) * B(1, 1) + A(3, 2) * B(2, 1) + A(3, 3) * B(3, 1),
                    A(3, 0) * B(0, 2) + A(3, 1) * B(1, 2) + A(3, 2) * B(2, 2) + A(3, 3) * B(3, 2),
                    A(3, 0) * B(0, 3) + A(3, 1) * B(1, 3) + A(3, 2) * B(2, 3) + A(3, 3) * B(3, 3));
}

inline Matrix4D MakeFrustumProjection(float fovyRad, float aspectRatio, float n, float f)
{
    float g = 1 / tanf(fovyRad * .5f);
    float k = f / (f - n);
    return Matrix4D(g / aspectRatio, 0, 0, 0, 0, g, 0, 0, 0, 0, k, -n * k, 0, 0, 1, 0);
}

inline Matrix4D MakeInfiniteProjection(float fovyRad, float aspectRatio, float n, float e = 0x1p-20f) // 2^-20
{
    float g = 1 / tanf(fovyRad * .5f);
    e = 1.0f - e;
    return Matrix4D(g / aspectRatio, 0, 0, 0, 0, g, 0, 0, 0, 0, e, -n * e, 0, 0, 1, 0);
}

// use these! (greater z values pass depth test)
// honestly I am shocked about learning the way floating point is distributed especially with 1/z function

inline Matrix4D MakeRevFrustumProjection(float fovy, float s, float n, float f)
{
    float g = 1.0F / tanf(fovy * 0.5f);
    float k = n / (n - f);

    return (Matrix4D(g / s, 0.0F, 0.0F, 0.0f, 0.0f, g, 0.0f, 0.0f, 0.0f, 0.0f, k, -f * k, 0.0f, 0.0f, 1.0f, 0.0f));
}

inline Matrix4D MakeRevInfiniteProjection(float fovy, float s, float n, float e = 0x1p-20f)
{
    float g = 1.0F / tanf(fovy * 0.5f);

    return (
        Matrix4D(g / s, 0.0F, 0.0F, 0.0f, 0.0f, g, 0.0f, 0.0f, 0.0f, 0.0f, e, n * (1.0f - e), 0.0f, 0.0f, 1.0f, 0.0f));
}

inline Matrix4D Mat4Identity()
{
    return {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
}

#endif // PETRENDERER_MATRIX4D_H
