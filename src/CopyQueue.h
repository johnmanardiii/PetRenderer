//
// Created by johnm on 11/24/2026.
//

#ifndef COPYQUEUE_H
#define COPYQUEUE_H

#include <volk.h>
#include "vk_mem_alloc.h"

struct GpuContext;
class ScopedFunctionalQueue;

struct CopyQueueBufferInput
{
    const void *data;
    size_t bufferSizeInBytes;
    VkBufferUsageFlags dstBufferUsage;
};

struct DeviceLocalResult
{
    VkBuffer deviceLocalBuffer;
    VmaAllocation deviceLocalBufferAlloc;
};

struct CopyQueue
{
    VmaAllocator allocator;
    VkDevice device;
    VkQueue graphicsQueue;

    // command pool / command buffer stuff
    VkCommandPool transferCommandPool;
    VkCommandBuffer transferCmdBuffer;
    VkFence transferFence;
};

CopyQueue CreateCopyQueue(VmaAllocator allocator, const GpuContext &gpuContext, ScopedFunctionalQueue &deletionQueue);

DeviceLocalResult CopyDataToDeviceLocal(const CopyQueue &copyQueue, const CopyQueueBufferInput &input);

#endif //COPYQUEUE_H
