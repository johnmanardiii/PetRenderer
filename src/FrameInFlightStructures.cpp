//
// Created by johnm on 5/11/2026.
//

#include "FrameInFlightStructures.h"

#include "VulkanDebugUtil.h"
#include "VulkanUtil.h"
#include <cassert>

FrameInFlightStructures CreateFrameInFlightStructures(VkDevice device, uint32_t queueFamilyIndex,
                                                      uint32_t framesInFlight)
{
    FrameInFlightStructures frameInFlightStructures;

    assert(framesInFlight > 0);

    // each command structure has it's own command pool. Could bite me in the future so be aware.
    VkCommandPoolCreateInfo commandPoolCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = queueFamilyIndex,
    };

    // let's create a command pool from the provided queue family index:
    vkchk(vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &frameInFlightStructures.commandPool));

    // name the command pool
    DebugSetName(device, VK_OBJECT_TYPE_COMMAND_POOL, frameInFlightStructures.commandPool,
                 "FrameInFlight CommandPool");

    VkCommandBufferAllocateInfo commandBufferAllocateInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = nullptr,
        .commandPool = frameInFlightStructures.commandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1 // for now going to make one command buffer
    };

    VkSemaphoreCreateInfo semaphoreCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };

    for (int i = 0; i < framesInFlight; i++)
    {
        PerFrameInFlightStructure perFrame;

        vkchk(vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, &perFrame.commandBuffer));
        vkchk(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &perFrame.imageAcquiredSemaphore));

        // name all of these per-frame in flight structures for debug purposes.
        DebugSetName(device, VK_OBJECT_TYPE_COMMAND_BUFFER, perFrame.commandBuffer,
                     fmt::format("FrameInFlight {} CommandBuffer", i).c_str());
        DebugSetName(device, VK_OBJECT_TYPE_SEMAPHORE, perFrame.imageAcquiredSemaphore,
                     fmt::format("FrameInFlight {} Image Acquired Semaphore", i).c_str());

        frameInFlightStructures.perFrameInFlightStructures.push_back(perFrame);
    }

    return frameInFlightStructures;
}

void DestroyFrameInFlightStructures(VkDevice device, FrameInFlightStructures &structures)
{
    for (PerFrameInFlightStructure &structure : structures.perFrameInFlightStructures)
    {
        vkDestroySemaphore(device, structure.imageAcquiredSemaphore, nullptr);
    }

    // Note: if the command pool is destroyed, it cleans up its command buffers
    if (structures.commandPool != VK_NULL_HANDLE)
    {
        vkDestroyCommandPool(device, structures.commandPool, nullptr);
    }
}

PerFrameInFlightStructure & GetFrameInFlightStructure(FrameInFlightStructures &structures, int currentFrameCount)
{
    return structures.perFrameInFlightStructures[currentFrameCount % structures.perFrameInFlightStructures.size()];
}