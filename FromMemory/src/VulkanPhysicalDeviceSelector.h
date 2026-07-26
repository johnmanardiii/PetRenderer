//
// Created by johnm on 4/12/2026.
//

#ifndef VULKANPHYSICALDEVICESELECTOR_H
#define VULKANPHYSICALDEVICESELECTOR_H

#include <volk.h>
#include <optional>

/*
 *  When this class is created, it fetches everything it might need from vulkan in it's constructor.
 *  When it is destructed, all of the resources needed for selecting the physical device are cleaned up...
 *  could honestly make it just get the resources it needs in the actual selection function but we shall see
 */
// suuuuper simplified where we just want one queue capable of graphics + present
struct PhysicalDeviceInfo
{
    VkPhysicalDevice physicalDevice;
    uint32_t
        graphicsPresentQueueFamilyIndex; // family index of queue that does everything we want (lazy)
};

class VulkanPhysicalDeviceSelector
{
  public:
    std::optional<PhysicalDeviceInfo> selectPhysicalDevice(VkInstance instance, VkSurfaceKHR surface);

  private:
    bool HasSwapChainExtension(VkPhysicalDevice device);
};

#endif // VULKANPHYSICALDEVICESELECTOR_H
