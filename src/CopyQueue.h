//
// Created by johnm on 11/24/2026.
//

#ifndef COPYQUEUE_H
#define COPYQUEUE_H

#include <volk.h>
#include "vk_mem_alloc.h"

struct ImmediateSubmitInfo;

// should I put the immediate submit info into this? would make some sense.
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


// #JOHN - the placement of this already feels off since it is in the same header file...
struct CopyQueueImageInput
{
    const void *data;
    size_t bufferSizeInBytes;
    const char *pDebugName;

    // #JOHN - honestly... from an api perspective I'm not sure if i should just have it create the vkimage beforehand or not...
    // this whole api feel clunky right here but maybe adding defaults will make it chill
    // feels like I'm weirdly limiting myself and also making it complicated the way I am doing this.
    VkImageType imageType;
    VkFormat format;
    VkExtent3D extent;
    uint32_t mipLevels;
    uint32_t arrayLayers;
    VkImageTiling tiling;
    VkImageUsageFlags usage;
};

// #JOHN - the name result is already kind of bothering me
// also coming back to this its kind of a "no duh" it's device local
struct DeviceLocalImageResult
{
    VkImage image;

    VmaAllocation imageAlloc;
};

DeviceLocalImageResult CopyImageToDeviceLocal(VmaAllocator allocator, const ImmediateSubmitInfo &immediateSubmit, const CopyQueueImageInput &input);

#endif //COPYQUEUE_H
