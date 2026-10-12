//
// Created by john on 10/4/26.
//

#ifndef PETRENDERER_MATRIX4D_H
#define PETRENDERER_MATRIX4D_H

// making this now because I need to pass a matrix to a shader and it seems to have complications if not
// 16 byte aligned...
// column-major just like Matrix3D
#include "Matrix3D.h"   // not sure if including this is a good idea but yolo

struct Matrix4D
{
private:
    float n[4][4];
public:
    Matrix4D() = default;

    Matrix4D(float n00, float n01, float n02, float n03,
        float n10, float n11, float n12, float n13,
        float n20, float n21, float n22, float n23,
        float n30, float n31, float n32, float n33)
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
    Matrix4D(const Matrix3D& other)
    {
        //FromMemory/docs/matrix-gpu-upload-and-layout.md
        // return Matrix4D();
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
};

#endif // PETRENDERER_MATRIX4D_H
