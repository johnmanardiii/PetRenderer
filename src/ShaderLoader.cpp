//
// Created by johnm on 6/28/2026.
//

#include "ShaderLoader.h"

#include "VulkanUtil.h"

#include <SDL3/SDL_iostream.h>
#include <fmt/printf.h>
#include <fmt/format.h>

bool LoadShader(const char *filePath, VkDevice device, VkShaderModule *shaderModule)
{
    size_t bitesRead;
    // #JOHN - ideally i would do this without allocations...
    std::string shaderPath = fmt::format("{}{}", ASSET_ROOT, filePath);
    void *data = SDL_LoadFile(shaderPath.c_str(), &bitesRead);
    if (!data)
    {
        fmt::print(stderr, "Failed to load shader file {}\n", shaderPath);
        return false;
    }

    VkShaderModuleCreateInfo moduleCreateInfo = {};
    moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    moduleCreateInfo.codeSize = bitesRead;
    moduleCreateInfo.pCode = static_cast<uint32_t *>(data);

    vkchk(vkCreateShaderModule(device, &moduleCreateInfo, nullptr, shaderModule));
    SDL_free(data);
    return true;
}