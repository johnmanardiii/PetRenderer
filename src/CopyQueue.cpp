//
// Created by johnm on 11/24/2026.
//

#include "CopyQueue.h"
#include "VulkanUtil.h"
#include "ImmediateSubmit.h"
#include "VulkanDebugUtil.h"

// #JOHN - honestly this file should be renamed since it is not a queue at all right now

/*
Very simple (for now) implementation to get buffer data to a device local buffer using a copy queue.
4 things that bother me about it (that I won't fix unless I start doing this mid-rendering):
1. The device local memory is created and destroyed EVERY TIME. It should be reused.
2. This BLOCKS once it is done (kind of defeating the purpose of using the copy queue), but sync should be handled by
 whatever needs the resource.
 3. If the device uses unified memory, then the transfer isn't needed and it can just be made on the device.
 4. Happens directly on the graphics queue instead of utilizing the transfer queue.

 Reading the vma docs... I can't help but feel like I am doing too much in my own implementation and there is a function that handles this but maybe not
 Like especilly it seems like it can handle the unified memory case.
 */

DeviceLocalResult CopyDataToDeviceLocal(VmaAllocator allocator, const ImmediateSubmitInfo &immediateSubmit,
                                        const CopyQueueBufferInput &input)
{
    // beginning of allocate the cpu visible buffer (should be in some "allocated once" so this code is temp
    // Could have used vmaCreateBuffer for this if this is my use case but
    VkBufferCreateInfo cpuVisBufferCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = input.bufferSizeInBytes,
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    VmaAllocationCreateInfo cpuVisAllocationInfo = {.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                                                             VMA_ALLOCATION_CREATE_MAPPED_BIT,
                                                    .usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST};

    // temp source buffer allocation info
    VmaAllocationInfo tmpSrcBufferAllocInfo;
    VkBuffer tmpTransferBuffer;
    VmaAllocation tmpSrcBufferAllocation;
    vkchk(vmaCreateBuffer(allocator, &cpuVisBufferCreateInfo, &cpuVisAllocationInfo, &tmpTransferBuffer,
                          &tmpSrcBufferAllocation, &tmpSrcBufferAllocInfo));

    // copy data into buffer (vma handles flushing if needed)
    vkchk(vmaCopyMemoryToAllocation(allocator, input.data, tmpSrcBufferAllocation, 0, input.bufferSizeInBytes));
    // end of allocating the cpu visible buffer

    // beginning of allocating the destination buffer (will be in final implementation)

    VkBufferCreateInfo dstBufferCreateInfo = {.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                                              .size = input.bufferSizeInBytes, // for now a 1-to-1 with the inptu buffer
                                              .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | input.dstBufferUsage,
                                              .sharingMode = VK_SHARING_MODE_EXCLUSIVE};
    VmaAllocationCreateInfo dstBufferAllocationInfo = {.flags = 0, .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE};
    VmaAllocationInfo dstBufferAllocInfo;
    VkBuffer dstBuffer;
    VmaAllocation dstBufferAllocation;
    vkchk(vmaCreateBuffer(allocator, &dstBufferCreateInfo, &dstBufferAllocationInfo, &dstBuffer, &dstBufferAllocation,
                          &dstBufferAllocInfo));

    auto copyToLocal = [input, sourceBuffer = tmpTransferBuffer, destinationBuffer = dstBuffer] (VkCommandBuffer cmdBuffer) {
        

        // end of allocating destination buffer

        // memory is now visible to be copied over to device local memory:
        // for now, reset the command buffer because it is no longer in use (fully waited on from last copy)

        VkBufferCopy copyInfo = {.srcOffset = 0, .dstOffset = 0, .size = input.bufferSizeInBytes};
        vkCmdCopyBuffer(cmdBuffer, sourceBuffer, destinationBuffer, 1, &copyInfo);
    };

    ImmediateSubmit(immediateSubmit, copyToLocal);

    if (input.pDebugName)
    {
        DebugSetName(immediateSubmit.device, VK_OBJECT_TYPE_BUFFER, dstBuffer, input.pDebugName);
    }
    

    // deallocate the temporarily made host visible buffers:
    vmaDestroyBuffer(allocator, tmpTransferBuffer, tmpSrcBufferAllocation);

    return {
        dstBuffer, dstBufferAllocation
    };
}

DeviceLocalImageResult CopyImageToDeviceLocal(VmaAllocator allocator, const ImmediateSubmitInfo &immediateSubmit,
                                              const CopyQueueImageInput &input)
{
    DeviceLocalImageResult result;

    return result;
}
