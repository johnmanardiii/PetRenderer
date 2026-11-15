//
// Created by johnm on 5/11/2026.
//

#ifndef VULKANCOMMANDBUFFERCREATOR_H
#define VULKANCOMMANDBUFFERCREATOR_H

#include "ScopedFunctionalQueue.h"
#include "VulkanUtil.h"

#include <optional>
#include <vector>
#include <volk.h>

// created per frame in flight

// #JOHN - I think for std::move to work appropriately here I need a wrapper that nulls them on move
struct PerFrameInFlightStructure
{
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    VkSemaphore imageAcquiredSemaphore = VK_NULL_HANDLE;

    uint64_t frameSubmitted = 0;
};

struct FrameInFlightStructures
{
    VkCommandPool commandPool = VK_NULL_HANDLE;
    std::vector<PerFrameInFlightStructure> perFrameInFlightStructures;
};

// #JOHN - not sure how I feel about bundling this scoped queue with the creation but yolo
FrameInFlightStructures CreateFrameInFlightStructures(VkDevice device, uint32_t queueFamilyIndex, uint32_t framesInFlight);

void DestroyFrameInFlightStructures(VkDevice device, FrameInFlightStructures& structures);

PerFrameInFlightStructure& GetFrameInFlightStructure(FrameInFlightStructures& structures, int currentFrameCount);


#endif //VULKANCOMMANDBUFFERCREATOR_H
