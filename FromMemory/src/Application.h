//
// Created by john on 10/31/26.
//

#ifndef PETRENDERER_APPLICATION_H
#define PETRENDERER_APPLICATION_H
#include "Vector3D.h"

#include <string>
#include <vector>

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

class Application
{
public:
    void LoadModels();
private:

};

#endif // PETRENDERER_APPLICATION_H
