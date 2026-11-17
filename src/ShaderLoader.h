//
// Created by johnm on 6/28/2026.
//

#ifndef SHADERLOADER_H
#define SHADERLOADER_H

#include <volk.h>

bool LoadShader(const char *filePath, VkDevice device, VkShaderModule *shaderModule);

#endif //SHADERLOADER_H
