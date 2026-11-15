//
// Created by john on 10/31/26.
//

#ifndef PETRENDERER_MODEL_H
#define PETRENDERER_MODEL_H

#include "Matrix4D.h"
#include "Vector3D.h"
#include "vk_mem_alloc.h"
#include <cstdint>
#include <string>
#include <vector>



// maybe needs a more specific name yolo
// maybe in the future we would want some shared definition between shaders and cpp
// #JOHN - come back to this after profiling & shadows
// https://learn.arm.com/learning-paths/mobile-graphics-and-gaming/optimizing-vertex-efficiency/vme-learning-path/ Real-Time Rendering, 4th ed., §16.4.5, “Vertex and Index Buffers/Arrays 
//https://github.com/zeux/meshoptimizer vertex fetch mechanics
struct Vertex
{
    Vector3D pos;
    float u;    // was complaining about alignment so I am going to shove this here
    Vector3D norm;
    float v;    // and why not this while I'm here.
};

// need to move mesh loading into a simple "hey just give path and I'll give you some gpu blocks to use"
// and then eventually have a retirement queue for this that appropriately waits for the gpu to be done before deleting
// with the delete funciton currently located in the pipeline file.
struct GPUMesh
{
    VkDeviceAddress vertexBufferAddress;
    VkBuffer vertexBuffer;
    VmaAllocation vertexBufferAlloc;

    // #JOHN - do I need the alloc info?
    VmaAllocationInfo vertexBufferAllocInfo;

    VkBuffer indexBuffer;
    VmaAllocation indexBufferAlloc;
    // #JOHN - do I need the alloc info?
    VmaAllocationInfo indexBufferAllocInfo;

    uint32_t indexCount;
};

GPUMesh LoadGPUMeshFromPath(VmaAllocator vmaAllocator, VkDevice device, const char *filePath);

// #JOHN - ideally this would be tied to the object and deferred deleted
void DestroyGpuMesh(VmaAllocator vmaAllocator, GPUMesh& mesh);

#endif // PETRENDERER_MODEL_H
