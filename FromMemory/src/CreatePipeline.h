//
// Created by johnm on 6/28/2026.
//

#ifndef CREATEPIPELINE_H
#define CREATEPIPELINE_H

#include <volk.h>

struct PipelineObjects
{
    VkPipelineLayout layout = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;

    VkDevice device = VK_NULL_HANDLE;
};

PipelineObjects CreatePipelineObjects(VkDevice device, VkDescriptorSetLayout descriptorSetLayout);
void DestroyPipelineObjects(VkDevice device, PipelineObjects& pipelineObjects);

#endif //CREATEPIPELINE_H
