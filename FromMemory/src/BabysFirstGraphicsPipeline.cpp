//
// Created by johnm on 8/12/2026.
//

#include "BabysFirstGraphicsPipeline.h"

BabyFirstGraphicsPipelineInfo CreateBabysFirstGraphicsPipelineInfo(GpuContext &gpuContext)
{
    // create the shaders we are going to use (vert and frag for now)

    // VkPipelineShaderStageCreateInfo


    // create the pipeline

    VkGraphicsPipelineCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .flags = 0,
    };

    VkPipeline babyPipeline = VK_NULL_HANDLE;

    return BabyFirstGraphicsPipelineInfo();
}

void DestroyBabysFirstGraphicsPipelineInfo()
{
}