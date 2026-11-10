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

/*
 * #JOHN - the point of this for now is to abstract out the main loop from "scene specific things" so that I can
 * better read vulkan engine because I can't imagine doing model loading alongside everything else right now
 *
 */
struct CPUMesh
{
    std::string name;
    std::vector<Vector3D> positions;
    std::vector<uint32_t> indices;
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
