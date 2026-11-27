//
// Created by johnm on 11/24/2026.
//

#include "CopyQueue.h"
#include "VulkanUtil.h"
#include "ScopedFunctionalQueue.h"
#include "GpuContext.h"

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
DeviceLocalResult CopyDataToDeviceLocal(const CopyQueue &copyQueue, const CopyQueueBufferInput &input)
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
     vkchk(vmaCreateBuffer(copyQueue.allocator, &cpuVisBufferCreateInfo, &cpuVisAllocationInfo, &tmpTransferBuffer,
                           &tmpSrcBufferAllocation, &tmpSrcBufferAllocInfo));

    // copy data into buffer (vma handles flushing if needed)
     vkchk(vmaCopyMemoryToAllocation(copyQueue.allocator, input.data, tmpSrcBufferAllocation, 0,
                                     input.bufferSizeInBytes));
    // end of allocating the cpu visible buffer



    // beginning of allocating the destination buffer (will be in final implementation)

    VkBufferCreateInfo dstBufferCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = input.bufferSizeInBytes,    // for now a 1-to-1 with the inptu buffer
        .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | input.dstBufferUsage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };
    VmaAllocationCreateInfo dstBufferAllocationInfo = {
        .flags = 0,
        .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE
    };
    VmaAllocationInfo dstBufferAllocInfo;
    VkBuffer dstBuffer;
    VmaAllocation dstBufferAllocation;
    vkchk(vmaCreateBuffer(copyQueue.allocator, &dstBufferCreateInfo, &dstBufferAllocationInfo, &dstBuffer,
                          &dstBufferAllocation,
                          &dstBufferAllocInfo));

    // end of allocating destination buffer


    // memory is now visible to be copied over to device local memory:
    // for now, reset the command buffer because it is no longer in use (fully waited on from last copy)
    vkResetCommandBuffer(copyQueue.transferCmdBuffer, 0);
    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };
    vkBeginCommandBuffer(copyQueue.transferCmdBuffer, &beginInfo);

    VkBufferCopy copyInfo = {.srcOffset = 0, .dstOffset = 0, .size = input.bufferSizeInBytes};
    vkCmdCopyBuffer(copyQueue.transferCmdBuffer, tmpTransferBuffer, dstBuffer, 1, &copyInfo);

    vkEndCommandBuffer(copyQueue.transferCmdBuffer);

    VkCommandBufferSubmitInfo transferCmdSubmitInfo = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
                                                       .commandBuffer = copyQueue.transferCmdBuffer
    };
    VkSubmitInfo2 transferSubmitInfo = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
                                        .commandBufferInfoCount = 1,
                                        .pCommandBufferInfos = &transferCmdSubmitInfo
    };
    vkQueueSubmit2(copyQueue.graphicsQueue, 1, &transferSubmitInfo, copyQueue.transferFence);

    // wait for any pending copies:
    vkWaitForFences(copyQueue.device, 1, &copyQueue.transferFence, VK_TRUE, 1000000000);

    vkResetFences(copyQueue.device, 1, &copyQueue.transferFence); // reset fence after waiting on it.

    // deallocate the temporarily made host visible buffers:
    vmaDestroyBuffer(copyQueue.allocator, tmpTransferBuffer, tmpSrcBufferAllocation);

    return {
        dstBuffer, dstBufferAllocation
    };
}

CopyQueue CreateCopyQueue(VmaAllocator allocator, const GpuContext &gpuContext, ScopedFunctionalQueue &deletionQueue)
{
    CopyQueue copyQueue;
    copyQueue.allocator = allocator;
    copyQueue.device = gpuContext.device;
    copyQueue.graphicsQueue = gpuContext.graphicsPresentQueue;

     // - this should probably be a funciton instead of a constructor creating this thing if it can fail
    VkCommandPoolCreateInfo commandPoolCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = gpuContext.selectedPhysicalDeviceInfo.graphicsPresentQueueFamilyIndex,
    };

    vkCreateCommandPool(copyQueue.device, &commandPoolCreateInfo, nullptr, &copyQueue.transferCommandPool);

    VkCommandBufferAllocateInfo cmdBufferAllocInfo = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                                                      .commandPool = copyQueue.transferCommandPool,
                                                      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                                                      .commandBufferCount = 1};
    vkAllocateCommandBuffers(copyQueue.device, &cmdBufferAllocInfo, &copyQueue.transferCmdBuffer);

    VkFenceCreateInfo fenceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        //.flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };
    vkchk(vkCreateFence(copyQueue.device, &fenceCreateInfo, nullptr, &copyQueue.transferFence));

    deletionQueue.Register([copyQueue] {
        vkDestroyCommandPool(copyQueue.device, copyQueue.transferCommandPool, nullptr);
        vkDestroyFence(copyQueue.device, copyQueue.transferFence, nullptr);
    });

    return copyQueue;
}
