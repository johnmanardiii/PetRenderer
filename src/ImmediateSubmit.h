//
// Created by johnm on 11/26/2026.
//

#ifndef IMMEDIATESUBMIT_H
#define IMMEDIATESUBMIT_H

#include "volk.h"
#include <functional>

struct GpuContext;
class ScopedFunctionalQueue;

struct ImmediateSubmitInfo
{
    VkDevice device;
    VkQueue graphicsQueue;
    VkCommandPool commandPool;
    VkCommandBuffer cmdBuffer;
    VkFence fence;
};

ImmediateSubmitInfo CreateImmediateSubmitInfo(const GpuContext &gpuContext, ScopedFunctionalQueue &deletionQueue);

void ImmediateSubmit(const ImmediateSubmitInfo& info, std::function<void(VkCommandBuffer)> func);

#endif //IMMEDIATESUBMIT_H
