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

// staging buffer stuff

// #JOHN - ideally this is like a big queue of work that wouldn't be too hard to break up if it gets too big...
// and maybe since it is a vector we could be like... "yo if adding this one to your plate would make you too full"
// like > 256 MB or whatever the limit is, then we can flush you and keep going. Not sure how to query that limit tho.
// but maybe we'll crash into that bridge when we crash into it.

// #JOHN - you need to make this more generic before you go onto textures but yolo... I think maybe
// somehow this needs to be a more central "request system" passed in... where you are like "yo can I get a staging block"
// and then it handles the flushing. Also if you go that route you really need to query something from the device
// on the staging memory. "DeviceLocalUploader..." name pending on that one.

// Sheesh the fact it needs the queue index is telling me it needs to be a central system.
// take in the device, queue index, and the also have it query it, and then pass it in.... should be shared by anything
// that needs to go CPU visible to gpu local like this... and it should just say "yep go off king" and then
// there will be a final flush and then these objects (like gpu mesh) will hold onto only the things they care about.
// https://vkguide.dev/docs/new_chapter_2/vulkan_imgui_setup/
// https://docs.vulkan.org/tutorial/latest/04_Vertex_buffers/02_Staging_buffer.html
// maybe a lambda passed? https://github.com/KhronosGroup/Vulkan-Docs/wiki/Synchronization-Examples#transfer-dependencies:~:text=()%3B%0A...%0AvkCmdEndRenderPass()%3B-,Transfer%20Dependencies,-Upload%20data%20from
// make the object hold onto the fence and command buffer and pool
// maybe also make it image vs buffer different
//https://github.com/zeux/niagara/blob/567ebe331247e122425eaf1a6319d8c64daf816d/src/resources.cpp#L152
//https://github.com/zeux/niagara/blob/567ebe331247e122425eaf1a6319d8c64daf816d/src/textures.cpp#L159
void CopyStagingBuffersToDestinationBuffers(VkDevice device, const std::vector<CPUVisibleStagingBuffer>& stagingBuffers)
{
    // create the command pool to use for the work
    VkCommandPoolCreateInfo cmdPoolCreateInfo =
        {
        .sType =  = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags =
    };
    vkchk(vkCreateCommandPool(device, ));
}

// end of staging buffer stuff

GPUMesh LoadGPUMeshFromPath(VmaAllocator vmaAllocator, VkDevice device, const char *filePath)
{
    LoadedMeshData cpuData = LoadCPUMeshDataFromPath(filePath);

    // intentionally interleave the vertex attributes for learning purposes #JOHN - come back to this and maybe break it up
    std::vector<Vertex> interleavedVertexData = CreateInterleavedVertexList(cpuData);

    // #JOHN - todo: move this to dedicated gpu local memory as part of a larger system
    // was reading over this: https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/struct_vma_allocation_create_info.html
    VmaAllocationCreateInfo vertexAllocCreateInfo = {
        .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO
    };

    size_t positionDataSize = sizeof(Vertex) * interleavedVertexData.size();
    VkBufferCreateInfo vertex_buffer_create_info =
    {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = positionDataSize,
        .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, // #JOHN probably want some other ones I can't think of rn
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    // #JOHN - this should probably be all constructed at once and failure should be handled better but yolo for now
    GPUMesh gpuMesh;
    VmaAllocationInfo vertexAllocInfo;
    vkchk(vmaCreateBuffer(vmaAllocator, &vertex_buffer_create_info, &vertexAllocCreateInfo, &gpuMesh.vertexBuffer,
                          &gpuMesh.vertexBufferAlloc, &vertexAllocInfo));

    memcpy(vertexAllocInfo.pMappedData, interleavedVertexData.data(), positionDataSize);

    VmaAllocationCreateInfo indexAllocCreateInfo = {
        .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO
    };

    // #JOHN - should probably make a types file that specifies we are using uint32 for the index type
    size_t indexDataSize = sizeof(uint32_t) * cpuData.indices.size();
    VkBufferCreateInfo index_buffer_create_info =
    {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = indexDataSize,
        .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    VmaAllocationInfo indexBufferAllocInfo;
    vkchk(vmaCreateBuffer(vmaAllocator, &index_buffer_create_info, &indexAllocCreateInfo, &gpuMesh.indexBuffer,
                          &gpuMesh.indexBufferAlloc, &indexBufferAllocInfo));

    memcpy(indexBufferAllocInfo.pMappedData, cpuData.indices.data(), indexDataSize);

    gpuMesh.indexCount = cpuData.indices.size();


    std::string indexBufferName = fmt::sprintf("Staging Index Buffer for %s", filePath);
    DebugSetName(device, VK_OBJECT_TYPE_BUFFER, gpuMesh.indexBuffer, indexBufferName.c_str());

    std::string vertexBufferName = fmt::sprintf("Staging Vertex Buffer for %s", filePath);
    DebugSetName(device, VK_OBJECT_TYPE_BUFFER, gpuMesh.vertexBuffer, vertexBufferName.c_str());

    // #JOHN - ideally this would be general enough to be usable with textures
    // #JOHN do full GPU to GPU copy here and wait

    // destroy staging buffers

    // gpu mesh should be valid here
    return gpuMesh;
}


void DestroyGpuMesh(VmaAllocator vmaAllocator, GPUMesh &mesh)
{
    vmaDestroyBuffer(vmaAllocator, mesh.vertexBuffer, mesh.vertexBufferAlloc);
    vmaDestroyBuffer(vmaAllocator, mesh.indexBuffer, mesh.indexBufferAlloc);
}