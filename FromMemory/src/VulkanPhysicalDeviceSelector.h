//
// Created by johnm on 4/12/2026.
//

#ifndef VULKANPHYSICALDEVICESELECTOR_H
#define VULKANPHYSICALDEVICESELECTOR_H

#include "vulkan/vulkan.h"
#include <optional>

/*
 *  When this class is created, it fetches everything it might need from vulkan in it's constructor.
 *  When it is destructed, all of the resources needed for selecting the physical device are cleaned up...
 *  could honestly make it just get the resources it needs in the actual selection function but we shall see
 */
class VulkanPhysicalDeviceSelector {
public:
    std::optional<VkPhysicalDevice> selectPhysicalDevice(VkInstance instance, VkSurfaceKHR surface);
private:

};



#endif //VULKANPHYSICALDEVICESELECTOR_H
