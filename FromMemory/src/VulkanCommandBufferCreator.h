//
// Created by johnm on 5/11/2026.
//

#ifndef VULKANCOMMANDBUFFERCREATOR_H
#define VULKANCOMMANDBUFFERCREATOR_H

#include "vulkan/vulkan.h"
#include <vector>

// created per frame in flight
struct PerFrameInFlightStructure
{
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    VkSemaphore imageAcquiredSemaphore = VK_NULL_HANDLE;

};

/*
 * #JOHN not sure why i grouped all of this into like, one file and class but I felt like "command pool" and
 * command buffer were related enough to store them on the same thing.
 *
 */
// #JOHN name of this is super vague...
class VulkanCommandStructures
{
public:
    VulkanCommandStructures(VkDevice device, uint32_t queueFamilyIndex, uint32_t framesInFlight);
    ~VulkanCommandStructures();
    VulkanCommandStructures(const VulkanCommandStructures&) = delete;
    VulkanCommandStructures& operator=(const VulkanCommandStructures&) = delete;
    VulkanCommandStructures(VulkanCommandStructures&&) noexcept;
    VulkanCommandStructures& operator=(VulkanCommandStructures&&) noexcept;
    PerFrameInFlightStructure& getPerFrameInFlightStructure(uint32_t frame)
    {
        return frameInFlightStructures[frame % frameInFlightStructures.size()];
    }
private:
    uint32_t numFramesInFlight;
    // #JOHN honestly ... give this a destructor. in case the whole thing blows up in the middle of construction.
    VkCommandPool commandPool = VK_NULL_HANDLE;
    std::vector<PerFrameInFlightStructure> frameInFlightStructures;

    // #JOHN this needs its own clean up for realllllll especially since I will be making n of them.
    VkDevice device;
    uint32_t queueFamilyIndex;  // keeping this in case I am curious later, should be chill. #JOHN
};


#endif //VULKANCOMMANDBUFFERCREATOR_H
