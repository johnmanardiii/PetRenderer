//
// Created by john on 10/31/26.
//

#include "Model.h"

// #JOHN - remove this once you abstract it
#include "VulkanDebugUtil.h"
#include "VulkanUtil.h"
#include "cgltf.h"
#include <cassert>
#include <SDL3/SDL_iostream.h>
#include <fmt/format.h>
#include <fmt/printf.h>
#include "Vector2D.h"
#include "CopyQueue.h"

/*
 * #JOHN - the point of this for now is to abstract out the main loop from "scene specific things" so that I can
 * better read vulkan engine because I can't imagine doing model loading alongside everything else right now
 *
 */

struct Material
{

};

struct LoadedMeshData
{
    std::string name;
    std::vector<Vector3D> positions;
    std::vector<Vector3D> normals;
    std::vector<Vector2D> uvs;
    std::vector<uint32_t> indices;
};

void LoadTextureData(const cgltf_data* data, LoadedMeshData& meshData)
{
    int i = 0;
    for (int i = 0; i < data->materials_count; i++)
    {
        cgltf_material mat = data->materials[i];
        // For now, assert that it has pbr metallic roughness and handle the case where it isn't the case later
        // when you want to load a model w/out textures or something like that #JOHN
        assert(mat.has_pbr_metallic_roughness);
        // not going to really check the rest of the other possible things for now, but know there is specular glossiness and clearcoat and transmission...


    }
}

LoadedMeshData ParseCpuMeshData(const cgltf_data *data)
{
    // #JOHN - for now, this isn't generalizable... but that's ok
    LoadedMeshData loadedModel;

    // for now, going to assume just one mesh, and then parse the first one
    assert(data->meshes_count == 1);

    cgltf_mesh *meshToLoad = data->meshes;
    loadedModel.name = meshToLoad->name;

    // For now, also assume only one primitive
    assert(meshToLoad->primitives_count == 1);

    cgltf_primitive *primitiveToLoad = meshToLoad->primitives;

    // assume triangles only for now
    assert(primitiveToLoad->type == cgltf_primitive_type_triangles);

    // index loading
    {
        // TODO: GENERATE DEFAULT INDICES IF NOT PROVIDED. DON'T WANT TO HANDLE SEPARATE FORMAT
        assert(primitiveToLoad->indices != nullptr);
    }

    cgltf_size numIndices = cgltf_accessor_unpack_indices(primitiveToLoad->indices, nullptr, sizeof(uint32_t), 0);
    loadedModel.indices.resize(numIndices);
    cgltf_accessor_unpack_indices(primitiveToLoad->indices, loadedModel.indices.data(), sizeof(uint32_t), numIndices);

    // position loading
    cgltf_attribute *positionAttribute = nullptr;
    cgltf_attribute *normalAttribute = nullptr;
    cgltf_attribute *uvAttribute = nullptr;
    cgltf_size attributeCount = primitiveToLoad->attributes_count;
    for (int i = 0; i < attributeCount; i++)
    {
        if (primitiveToLoad->attributes[i].type == cgltf_attribute_type_position)
        {
            positionAttribute = &primitiveToLoad->attributes[i];
        }
        else if (primitiveToLoad->attributes[i].type == cgltf_attribute_type_normal)
        {
            normalAttribute = &primitiveToLoad->attributes[i];
        }
        else if (primitiveToLoad->attributes[i].type == cgltf_attribute_type_texcoord)
        {
            uvAttribute = &primitiveToLoad->attributes[i];
        }
    }

    assert(positionAttribute != nullptr);

    cgltf_accessor *posAccessor = positionAttribute->data;

    // make some assertions to make sure it is OK to copy into my stuff the way I plan to
    assert(posAccessor->type == cgltf_type_vec3);
    assert(posAccessor->offset == 0);
    assert(posAccessor->stride == sizeof(Vector3D));

    cgltf_size numPosFloats = cgltf_accessor_unpack_floats(posAccessor, nullptr, 0);
    assert(numPosFloats % 3 == 0);
    assert(numPosFloats / 3 == posAccessor->count);

    loadedModel.positions.resize(posAccessor->count);

    cgltf_accessor_unpack_floats(posAccessor, reinterpret_cast<float *>(loadedModel.positions.data()), numPosFloats);

    // load normals (honestly a lot of the same code as the position if there is another one then you should abstract it) set the normal data to the same count as position
    loadedModel.normals.resize(posAccessor->count);
    if (normalAttribute != nullptr)
    {
        // load the normals from the attribute
        cgltf_accessor *normAccessor = normalAttribute->data;
        assert(normAccessor->type == cgltf_type_vec3);
        assert(normAccessor->offset == 0);
        assert(normAccessor->stride == sizeof(Vector3D));
        cgltf_size numNormFloats = cgltf_accessor_unpack_floats(normAccessor, nullptr, 0);
        assert(numNormFloats == numPosFloats);

        cgltf_accessor_unpack_floats(normAccessor, reinterpret_cast<float *>(loadedModel.normals.data()), numNormFloats);
    }
    else
    {
        // initialize the normals to 0
        Vector3D zero{0.0f, 0.0f, 0.0f};
        std::fill(loadedModel.normals.begin(), loadedModel.normals.end(), zero);
    }

    // load uvs
    loadedModel.uvs.resize(posAccessor->count);
    if (uvAttribute != nullptr)
    {
        cgltf_accessor *uvAccessor = uvAttribute->data;
        assert(uvAccessor->type == cgltf_type_vec2);
        assert(uvAccessor->offset == 0);
        assert(uvAccessor->stride == sizeof(Vector2D));

        cgltf_size numUvFloats = cgltf_accessor_unpack_floats(uvAccessor, nullptr, 0);
        assert((numUvFloats / 2) == (numPosFloats / 3));

        cgltf_accessor_unpack_floats(uvAccessor, reinterpret_cast<float *>(loadedModel.uvs.data()), numUvFloats);
    }
    else
    {
        Vector2D zero{0.0f, 0.0f};
        std::fill(loadedModel.uvs.begin(), loadedModel.uvs.end(), zero);
    }

    LoadTextureData(data, loadedModel);

    return loadedModel;
}

LoadedMeshData LoadCPUMeshDataFromPath(const char *filePath)
{
    LoadedMeshData cpu_mesh_data;

    cgltf_options options = {};
    cgltf_data *data = NULL;

    // #JOHN - this should really be a shared function now that it is repeated across both load shader and here
    size_t bitesRead;
    std::string modelPath = fmt::format("{}{}", ASSET_ROOT, filePath);
    void *biteData = SDL_LoadFile(modelPath.c_str(), &bitesRead);

    if (!biteData)
    {
        SDL_Log("%s\n", SDL_GetError());
        return cpu_mesh_data; // #JOHN - might need a failure return...
    }
    cgltf_result result = cgltf_parse(&options, biteData, bitesRead, &data);

    // john - this guide is kinda good tho: https://www.khronos.org/files/gltf20-reference-guide.pdf
    // plan: get straight positions + indices in
    // see a fox
    // get texture in brute force
    // then think about broader scene idea
    // then if I am feeling myself maybe try to get multiple in a scene I can see (albedo only...)
    // animated meshes sounds coool.
    if (result == cgltf_result_success)
    {
        // #JOHN - might be a more efficient way to do this with glbs but if this works it works
        cgltf_result loadResult = cgltf_load_buffers(&options, data, filePath);
        if (loadResult == cgltf_result_success)
        {
            cpu_mesh_data = ParseCpuMeshData(data);
        }
        else
        {
            assert(false);
        }

        cgltf_free(data);
    }
    else
    {
        assert(false);
    }

    return cpu_mesh_data;
}

std::vector<Vertex> CreateInterleavedVertexList(const LoadedMeshData& loadedMeshData)
{
    std::vector<Vertex> result;
    result.resize(loadedMeshData.positions.size());
    for (size_t i = 0; i < loadedMeshData.positions.size(); i++)
    {
        result[i] = {loadedMeshData.positions[i], loadedMeshData.uvs[i].x, loadedMeshData.normals[i],
                     loadedMeshData.uvs[i].y};
    }

    return result;
}

GPUMesh LoadGPUMeshFromPath(CopyQueue &copyQueue, VkDevice device, const char *filePath)
{
    LoadedMeshData cpuData = LoadCPUMeshDataFromPath(filePath);

    // intentionally interleave the vertex attributes for learning purposes #JOHN - come back to this and maybe break it up
    std::vector<Vertex> interleavedVertexData = CreateInterleavedVertexList(cpuData);

    size_t positionDataSize = sizeof(Vertex) * interleavedVertexData.size();
    DeviceLocalResult vertexBuffer = CopyDataToDeviceLocal(copyQueue,
        {interleavedVertexData.data(), positionDataSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT});

    size_t indexDataSize = sizeof(uint32_t) * cpuData.indices.size();
    DeviceLocalResult indexBuffer = CopyDataToDeviceLocal(copyQueue, {cpuData.indices.data(), indexDataSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT});

    std::string indexBufferName = fmt::sprintf("Staging Index Buffer for %s", filePath);
    DebugSetName(device, VK_OBJECT_TYPE_BUFFER, indexBuffer.deviceLocalBuffer, indexBufferName.c_str());

    std::string vertexBufferName = fmt::sprintf("Staging Vertex Buffer for %s", filePath);
    DebugSetName(device, VK_OBJECT_TYPE_BUFFER, vertexBuffer.deviceLocalBuffer, vertexBufferName.c_str());

    // gpu mesh should be valid here
    return
    {
        .vertexBuffer = vertexBuffer.deviceLocalBuffer, .vertexBufferAlloc = vertexBuffer.deviceLocalBufferAlloc,
        .indexBuffer = indexBuffer.deviceLocalBuffer, .indexBufferAlloc = indexBuffer.deviceLocalBufferAlloc,
        .indexCount = static_cast<uint32_t>(cpuData.indices.size())
    };
}


void DestroyGpuMesh(VmaAllocator vmaAllocator, GPUMesh &mesh)
{
    vmaDestroyBuffer(vmaAllocator, mesh.vertexBuffer, mesh.vertexBufferAlloc);
    vmaDestroyBuffer(vmaAllocator, mesh.indexBuffer, mesh.indexBufferAlloc);
}