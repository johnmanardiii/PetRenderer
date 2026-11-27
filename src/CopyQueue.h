//
// Created by johnm on 11/24/2026.
//

#ifndef COPYQUEUE_H
#define COPYQUEUE_H

#include <volk.h>
#include "vk_mem_alloc.h"

struct ImmediateSubmitInfo;

struct CopyQueueBufferInput
{
    const void *data;
    size_t bufferSizeInBytes;
    VkBufferUsageFlags dstBufferUsage;
    const char *pDebugName;
};

struct DeviceLocalResult
{
    VkBuffer deviceLocalBuffer;
    VmaAllocation deviceLocalBufferAlloc;
};

DeviceLocalResult CopyDataToDeviceLocal(VmaAllocator allocator, const ImmediateSubmitInfo &immediateSubmit,
                                        const CopyQueueBufferInput &input);

#endif //COPYQUEUE_H
