//
// Created by johnm on 5/11/2026.
//

#ifndef VULKANCOMMANDBUFFERCREATOR_H
#define VULKANCOMMANDBUFFERCREATOR_H

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

/*
 * #JOHN not sure why i grouped all of this into like, one file and class but I felt like "command pool" and
 * command buffer were related enough to store them on the same thing.
 */
class FrameInFlightStructures
{
public:
    static FrameInFlightStructures Create(VkDevice device, uint32_t queueFamilyIndex, uint32_t framesInFlight);

    // turn this into something that calls a shared private "#JOHN destruct vars I pass to you relating to me"
    ~FrameInFlightStructures();

    FrameInFlightStructures(FrameInFlightStructures&&) = default;
    FrameInFlightStructures& operator=(FrameInFlightStructures&&) = default;

    FrameInFlightStructures(const FrameInFlightStructures&) = delete;
    FrameInFlightStructures& operator=(const FrameInFlightStructures&) = delete;

    PerFrameInFlightStructure& getPerFrameInFlightStructure(const uint32_t frame)
    {
        return frameInFlightStructures[frame % frameInFlightStructures.size()];
    }

private:
    FrameInFlightStructures(VkDevice device, VkCommandPool cmdPool, std::vector<PerFrameInFlightStructure> frameInFlightStructure);

    VkHandleRef<VkDevice> device;
    VkHandleRef<VkCommandPool> commandPool;

    std::vector<PerFrameInFlightStructure> frameInFlightStructures;
};


#endif //VULKANCOMMANDBUFFERCREATOR_H
