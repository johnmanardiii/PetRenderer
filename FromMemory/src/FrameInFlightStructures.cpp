//
// Created by johnm on 5/11/2026.
//

#include "FrameInFlightStructures.h"

#include "VulkanDebugUtil.h"
#include "VulkanUtil.h"
#include <cassert>

FrameInFlightStructures::FrameInFlightStructures(VkDevice device, VkCommandPool cmdPool, std::vector<PerFrameInFlightStructure> frameInFlightStructure)
    : device(device), commandPool(cmdPool), frameInFlightStructures(std::move(frameInFlightStructure))
{

}

FrameInFlightStructures FrameInFlightStructures::Create(VkDevice device, uint32_t queueFamilyIndex,
                                                                       uint32_t framesInFlight)
{
    assert(framesInFlight > 0);

    // each command structure has it's own command pool. Could bite me in the future so be aware.
    VkCommandPoolCreateInfo commandPoolCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = queueFamilyIndex,
    };

    // let's create a command pool from the provided queue family index:
    VkCommandPool commandPool = VK_NULL_HANDLE;
    vkchk(vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &commandPool));

    // name the command pool
    DebugSetName(device, VK_OBJECT_TYPE_COMMAND_POOL, commandPool, "FrameInFlight CommandPool");

    VkCommandBufferAllocateInfo commandBufferAllocateInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = nullptr,
        .commandPool = commandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1 // for now going to make one command buffer
    };

    VkSemaphoreCreateInfo semaphoreCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };

    VkFenceCreateInfo fenceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT, // start signaled so we don't need to reset on the first frame
    };

    std::vector<PerFrameInFlightStructure> frameInFlightStructures;
    for (int i = 0; i < framesInFlight; i++)
    {
        PerFrameInFlightStructure perFrame;

        vkchk(vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, &perFrame.commandBuffer));
        vkchk(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &perFrame.imageAcquiredSemaphore));

        // name all of these per-frame in flight structures for debug purposes.
        DebugSetName(device, VK_OBJECT_TYPE_COMMAND_BUFFER, perFrame.commandBuffer, fmt::format("FrameInFlight {} CommandBuffer", i).c_str());
        DebugSetName(device, VK_OBJECT_TYPE_SEMAPHORE, perFrame.imageAcquiredSemaphore, fmt::format("FrameInFlight {} Image Acquired Semaphore", i).c_str());

        frameInFlightStructures.emplace_back(perFrame);
    }

    return FrameInFlightStructures(device, commandPool, std::move(frameInFlightStructures));
}

FrameInFlightStructures::~FrameInFlightStructures()
{
    // destroy semaphores and all that
    for (PerFrameInFlightStructure& structure : frameInFlightStructures)
    {
        vkDestroySemaphore(device.get(), structure.imageAcquiredSemaphore, nullptr);
    }

    // Note: if the command pool is destroyed, it cleans up its command buffers
    if (commandPool.get() != VK_NULL_HANDLE)
    {
        vkDestroyCommandPool(device.get(), commandPool.get(), nullptr);
    }
}