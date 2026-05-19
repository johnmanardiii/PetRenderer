//
// Created by johnm on 5/11/2026.
//

#include "VulkanCommandBufferCreator.h"

#include "VulkanUtil.h"
#include <cassert>

VulkanCommandStructures::VulkanCommandStructures(VkDevice device, uint32_t queueFamilyIndex, uint32_t framesInFlight)
    : numFramesInFlight(framesInFlight), device(device), queueFamilyIndex(queueFamilyIndex)
{
    assert(numFramesInFlight > 0);
    // Going to reset these queues each frame, so went with that flag
    // always go down a useless rabbit hole when making these for protected memory:
    // https://docs.vulkan.org/guide/latest/protected.html
    // but like, maybe I'll mess with that but it doesn't seem to matter.
    VkCommandPoolCreateInfo commandPoolCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = queueFamilyIndex,
    };
    // let's create a command pool from the provided queue family index:
    vkchk(vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &commandPool));

    // #JOHN question for the class... in our case where we have multiple frames in flight, but like
    // single threaded recording, is that a one command pool, multiple (n frames) command buffer case?
    // for now lets do one per pool and ask Ethan when we get to work

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

    for (int i = 0; i < numFramesInFlight; i++)
    {
        PerFrameInFlightStructure perFrame;

        vkchk(vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, &perFrame.commandBuffer));
        vkchk(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &perFrame.imageAcquiredSemaphore));
        vkchk(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &perFrame.readyToPresentSemaphore));

        frameInFlightStructures.emplace_back(perFrame);
    }
}
VulkanCommandStructures::~VulkanCommandStructures()
{
    // destroy semaphores and all that

    // Note: if the command pool is destroyed, it cleans up its command buffers
    if (commandPool != VK_NULL_HANDLE)
    {
        vkDestroyCommandPool(device, commandPool, nullptr);
    }
}
VulkanCommandStructures::VulkanCommandStructures(VulkanCommandStructures &&other) noexcept
    : commandPool(std::exchange(other.commandPool, VK_NULL_HANDLE)),
      frameInFlightStructures(std::exchange(other.frameInFlightStructures, VK_NULL_HANDLE)), device(other.device),
      queueFamilyIndex(other.queueFamilyIndex)
{
}
VulkanCommandStructures &VulkanCommandStructures::operator=(VulkanCommandStructures &&other) noexcept
{
    if (this == &other)
        return *this;

    device = other.device;
    queueFamilyIndex = other.queueFamilyIndex;
    commandPool = std::exchange(other.commandPool, VK_NULL_HANDLE);
    frameInFlightStructures = std::exchange(other.frameInFlightStructures, VK_NULL_HANDLE);

    return *this;
}