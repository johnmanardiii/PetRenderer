//
// Created by johnm on 6/28/2026.
//

#ifndef CREATEPIPELINE_H
#define CREATEPIPELINE_H

#include <volk.h>
// #JOHN - super temp just get it working version
struct PipelineObjects
{
    VkPipelineLayout layout = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;

    VkDevice device = VK_NULL_HANDLE;

    PipelineObjects(VkDevice device, VkDescriptorSetLayout descriptorSetLayout);
    ~PipelineObjects();

    PipelineObjects(const PipelineObjects&) = delete;
    PipelineObjects& operator=(const PipelineObjects&) = delete;
    PipelineObjects(PipelineObjects&&) = delete;
    PipelineObjects& operator=(PipelineObjects&&) = delete;
};

#endif //CREATEPIPELINE_H
