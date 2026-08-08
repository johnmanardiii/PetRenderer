//
// Created by johnm on 8/7/2026.
//

#ifndef VULKANINIT_H
#define VULKANINIT_H

#include <vector>
#include <string>
#include "volk.h"
#include <optional>

struct PhysicalDeviceInfo
{
    VkPhysicalDevice physicalDevice;
    uint32_t graphicsPresentQueueFamilyIndex; // family index of queue that does everything we want (lazy)
};

// #JOHN - figure out if I even need the required layers
VkInstance CreateInstance(const std::vector<std::string> RequiredLayers,
                          const std::vector<const char*>& RequiredExtensions);

std::optional<PhysicalDeviceInfo> selectPhysicalDevice(VkInstance instance, VkSurfaceKHR surface);

#endif //VULKANINIT_H
