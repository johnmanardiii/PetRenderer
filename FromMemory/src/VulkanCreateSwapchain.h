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

/*
 * Misc things I need later that I determined from creating the swapchain
 */
struct SwapchainInfo
{
    VkSwapchainKHR swapchain;
    VkFormat swapchainImageFormat;
    std::vector<VkImage> swapchainImages;
};

namespace VulkanCreateSwapchain
{
std::optional<SwapchainInfo> CreateSwapchain(VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface,
                                              VkExtent2D fallbackExtentSize);

std::optional<std::vector<VkImageView>> CreateSwapchainImageViews(VkDevice device, const SwapchainInfo& swapchain);
};

#endif // VULKANCREATESWAPCHAIN_H
