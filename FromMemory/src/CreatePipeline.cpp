//
// Created by johnm on 6/28/2026.
//

#include "CreatePipeline.h"
#include "ShaderLoader.h"
#include "VulkanUtil.h"

#include <assert.h>

PipelineObjects::PipelineObjects(VkDevice device, VkDescriptorSetLayout descriptorSetLayout) : device(device)
{
    // #JOHN - crazzzyyyy how we hardcoding the shader but yolo
    VkShaderModule shaderModule = VK_NULL_HANDLE;
    // Uhhh the working directory shall be ...
    if (!LoadShader("../shaders/gradient/gradient.comp.spv", device, &shaderModule))
    {
        assert(false);
    }

    VkPipelineShaderStageCreateInfo shaderStageCreateInfo = {};
    shaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageCreateInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    shaderStageCreateInfo.module = shaderModule;
    shaderStageCreateInfo.pName = "main";
    // no specialization info for this one

    // Create the pipeline layout (and I think store it? needs to be deleted laters)
    // layouts are like the thing connecting the descriptor set to the pipeline -> shader
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};

    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount = 1;
    pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout; // #JOHN this isn't needed after the create call

    vkchk(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &layout));

    VkComputePipelineCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    createInfo.stage = shaderStageCreateInfo;
    createInfo.layout = layout;

    // #JOHN doooon't care about pipeline caches yet...
    vkchk(vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &createInfo, nullptr, &pipeline));

    // we have created the pipeline, OK to destroy shader module
    vkDestroyShaderModule(device, shaderModule, nullptr);
}

PipelineObjects::~PipelineObjects()
{
    vkDestroyPipeline(device, pipeline, nullptr);
    vkDestroyPipelineLayout(device, layout, nullptr);
}