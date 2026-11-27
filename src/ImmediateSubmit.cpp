//
// Created by johnm on 11/26/2026.
//

#include "ImmediateSubmit.h"

#include "GpuContext.h"
#include "ScopedFunctionalQueue.h"

ImmediateSubmitInfo CreateImmediateSubmitInfo(const GpuContext &gpuContext, ScopedFunctionalQueue &deletionQueue)
{
    ImmediateSubmitInfo immediateSubmitInfo;
    immediateSubmitInfo.device = gpuContext.device;
    immediateSubmitInfo.graphicsQueue = gpuContext.graphicsPresentQueue;


    VkCommandPoolCreateInfo commandPoolCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = gpuContext.selectedPhysicalDeviceInfo.graphicsPresentQueueFamilyIndex,
    };

    vkCreateCommandPool(immediateSubmitInfo.device, &commandPoolCreateInfo, nullptr, &immediateSubmitInfo.commandPool);

    VkCommandBufferAllocateInfo cmdBufferAllocInfo = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                                                      .commandPool = immediateSubmitInfo.commandPool,
                                                      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                                                      .commandBufferCount = 1};
    vkAllocateCommandBuffers(immediateSubmitInfo.device, &cmdBufferAllocInfo, &immediateSubmitInfo.cmdBuffer);

    VkFenceCreateInfo fenceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
    };
    vkchk(vkCreateFence(immediateSubmitInfo.device, &fenceCreateInfo, nullptr, &immediateSubmitInfo.fence));

    deletionQueue.Register([immediateSubmitInfo] {
        vkDestroyCommandPool(immediateSubmitInfo.device, immediateSubmitInfo.commandPool, nullptr);
        vkDestroyFence(immediateSubmitInfo.device, immediateSubmitInfo.fence, nullptr);
    });

    return immediateSubmitInfo;
}

void ImmediateSubmit(const ImmediateSubmitInfo& info, std::function<void(VkCommandBuffer)> func)
{
    vkResetCommandBuffer(info.cmdBuffer, 0);
    VkCommandBufferBeginInfo beginInfo = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                                          .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};
    vkBeginCommandBuffer(info.cmdBuffer, &beginInfo);

    // call actual function passed in.
    func(info.cmdBuffer);

    vkEndCommandBuffer(info.cmdBuffer);

    VkCommandBufferSubmitInfo transferCmdSubmitInfo = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
                                                       .commandBuffer = info.cmdBuffer};
    VkSubmitInfo2 transferSubmitInfo = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
                                        .commandBufferInfoCount = 1,
                                        .pCommandBufferInfos = &transferCmdSubmitInfo};
    vkQueueSubmit2(info.graphicsQueue, 1, &transferSubmitInfo, info.fence);

    // wait for any pending copies:
    vkchk(vkWaitForFences(info.device, 1, &info.fence, VK_TRUE, 10000000000));

    vkResetFences(info.device, 1, &info.fence); // reset fence after waiting on it.
}

