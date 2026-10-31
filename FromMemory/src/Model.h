//
// Created by john on 10/31/26.
//

#ifndef PETRENDERER_MODEL_H
#define PETRENDERER_MODEL_H

#include <string>
#include <vector>
#include "Vector3D.h"

/*
 * #JOHN - the point of this for now is to abstract out the main loop from "scene specific things" so that I can
 * better read vulkan engine because I can't imagine doing model loading alongside everything else right now
 *
 */
struct CPUMeshData
{
    std::string name;
    std::vector<Vector3D> positions;
};

CPUMeshData loadMeshPositions(const char* filePath);

#endif // PETRENDERER_MODEL_H
