//
// Created by john on 10/31/26.
//

#include "Model.h"

// #JOHN - remove this once you abstract it
#include "VulkanDebugUtil.h"
#include "VulkanUtil.h"
#include "cgltf.h"
#include <cassert>

CPUMesh ParseCpuMeshData(const cgltf_data *data)
{
    // #JOHN - for now, this isn't generalizable... but that's ok
    CPUMesh loadedModel;

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
    cgltf_size attributeCount = primitiveToLoad->attributes_count;
    for (int i = 0; i < attributeCount; i++)
    {
        if (primitiveToLoad->attributes[i].type == cgltf_attribute_type_position)
        {
            positionAttribute = &primitiveToLoad->attributes[i];
            break;
        }
    }
    assert(positionAttribute != nullptr);

    cgltf_accessor *posAccessor = positionAttribute->data;

    // make some assertions to make sure it is OK to copy into my stuff the way I plan to
    assert(posAccessor->type == cgltf_type_vec3);
    assert(posAccessor->offset == 0);
    assert(posAccessor->stride == sizeof(Vector3D));

    cgltf_float *startAddress = nullptr;
    cgltf_size numPosFloats = cgltf_accessor_unpack_floats(posAccessor, nullptr, 0);
    assert(numPosFloats % 3 == 0);
    assert(numPosFloats / 3 == posAccessor->count);

    loadedModel.positions.resize(posAccessor->count);

    cgltf_accessor_unpack_floats(posAccessor, reinterpret_cast<float *>(loadedModel.positions.data()), numPosFloats);

    return loadedModel;
}

CPUMesh LoadCPUMeshDataFromPath(const char *filePath)
{
    CPUMesh cpu_mesh_data;

    cgltf_options options = {};
    cgltf_data *data = NULL;
    cgltf_result result = cgltf_parse_file(&options, filePath, &data);

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


GPUMesh LoadGPUMeshFromPath(VmaAllocator vmaAllocator, VkDevice device, const char *filePath)
{
    CPUMesh cpuData = LoadCPUMeshDataFromPath(filePath);

    // #JOHN - todo: move this to dedicated gpu local memory as part of a larger system
    // was reading over this: https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/struct_vma_allocation_create_info.html
    VmaAllocationCreateInfo vertexAllocCreateInfo = {
        .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO
    };

    size_t positionDataSize = sizeof(Vector3D) * cpuData.positions.size();
    VkBufferCreateInfo vertex_buffer_create_info =
    {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = positionDataSize,
        .usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, // #JOHN probably want some other ones I can't think of rn
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    // #JOHN - this should probably be all constructed at once and failure should be handled better but yolo for now
    GPUMesh gpuMesh;

    vkchk(vmaCreateBuffer(vmaAllocator, &vertex_buffer_create_info, &vertexAllocCreateInfo, &gpuMesh.vertexBuffer,
                          &gpuMesh.vertexBufferAlloc, &gpuMesh.vertexBufferAllocInfo));

    memcpy(gpuMesh.vertexBufferAllocInfo.pMappedData, cpuData.positions.data(), positionDataSize);

    VkBufferDeviceAddressInfo vertexBuffDeviceAddrInfo {
        .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .buffer = gpuMesh.vertexBuffer
    };
    gpuMesh.vertexBufferAddress = vkGetBufferDeviceAddress(device, &vertexBuffDeviceAddrInfo);

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

    vkchk(vmaCreateBuffer(vmaAllocator, &index_buffer_create_info, &indexAllocCreateInfo, &gpuMesh.indexBuffer,
                          &gpuMesh.indexBufferAlloc, &gpuMesh.indexBufferAllocInfo));

    memcpy(gpuMesh.indexBufferAllocInfo.pMappedData, cpuData.indices.data(), indexDataSize);

    gpuMesh.indexCount = cpuData.indices.size();


    std::string indexBufferName = fmt::sprintf("Index Buffer for %s", filePath);
    DebugSetName(device, VK_OBJECT_TYPE_BUFFER, gpuMesh.indexBuffer, indexBufferName.c_str());

    std::string vertexBufferName = fmt::sprintf("Vertex Buffer for %s", filePath);
    DebugSetName(device, VK_OBJECT_TYPE_BUFFER, gpuMesh.vertexBuffer, vertexBufferName.c_str());

    return gpuMesh;
}


void DestroyGpuMesh(VmaAllocator vmaAllocator, GPUMesh &mesh)
{
    vmaDestroyBuffer(vmaAllocator, mesh.vertexBuffer, mesh.vertexBufferAlloc);
    vmaDestroyBuffer(vmaAllocator, mesh.indexBuffer, mesh.indexBufferAlloc);
}