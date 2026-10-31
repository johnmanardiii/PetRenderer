//
// Created by john on 10/31/26.
//

#include "Model.h"

// #JOHN - remove this once you abstract it
#include "cgltf.h"
#include <cassert>

CPUMeshData ParseCpuMeshData(const cgltf_data *data)
{
    // #JOHN - for now, this isn't generalizable... but that's ok
    CPUMeshData loadedModel;

    // for now, going to assume just one mesh, and then parse the first one
    assert(data->meshes_count == 1);

    cgltf_mesh *meshToLoad = data->meshes;
    loadedModel.name = meshToLoad->name;

    // For now, also assume only one primitive
    assert(meshToLoad->primitives_count == 1);

    cgltf_primitive *primitiveToLoad = meshToLoad->primitives;

    // assume triangles only for now
    assert(primitiveToLoad->type == cgltf_primitive_type_triangles);

    // assume no indices for now
    assert(primitiveToLoad->indices == nullptr);

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

CPUMeshData loadMeshPositions(const char* filePath)
{
    CPUMeshData cpu_mesh_data;

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