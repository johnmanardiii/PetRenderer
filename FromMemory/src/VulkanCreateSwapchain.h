//
// Created by johnm on 4/15/2026.
//

#ifndef VULKANCREATESWAPCHAIN_H
#define VULKANCREATESWAPCHAIN_H
#include "VulkanLogicalDeviceCreator.h"
#include "VulkanPhysicalDeviceSelector.h"

#include <optional>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace VulkanCreateSwapchain
{
std::optional<VkSwapchainKHR> CreateSwapchain(VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface,
                                              VkExtent2D fallbackExtentSize);
};

#endif // VULKANCREATESWAPCHAIN_H
