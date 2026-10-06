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
    // #JOHN - not sure if this is efficient but yolo
    Matrix4D(const Matrix3D& other)
    {
        //FromMemory/docs/matrix-gpu-upload-and-layout.md
        // return Matrix4D();
    }
};

#endif // PETRENDERER_MATRIX4D_H
