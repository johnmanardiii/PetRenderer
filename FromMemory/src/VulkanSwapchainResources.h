//
// Created by johnm on 4/15/2026.
//

#ifndef VULKANCREATESWAPCHAIN_H
#define VULKANCREATESWAPCHAIN_H

#include "FunctionalQueue.h"
#include <SDL3/SDL_video.h>
#include <optional>
#include <vector>
#include <volk.h>

/*
 * Misc things I need later that I determined from creating the swapchain
 *
 * Everything in this struct is automatically cleaned up from "destroy swapchain"
 */
struct SwapchainInfo
{
    VkSwapchainKHR swapchain;
    VkFormat swapchainImageFormat;
    std::vector<VkImage> swapchainImages;
    VkExtent2D swapchainExtent;
};

struct AcquiredSwapchainImageInfo
{
    VkImage image;
    VkImageView imageView;
    uint32_t imageIndex;
    VkExtent2D extent;
    VkSemaphore imageReadyToPresentSemaphore;
};

class VulkanSwapchainResources
{
public:
    VulkanSwapchainResources(VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface,
                             VkExtent2D fallbackExtentSize);
    ~VulkanSwapchainResources();

    // don't want this copied around
    VulkanSwapchainResources(const VulkanSwapchainResources &) = delete;
    VulkanSwapchainResources &operator=(const VulkanSwapchainResources &) = delete;

    // #JOHN lose out on a fence here but don't really mind
    void PresentImage(VkQueue queue, VkSemaphore waitSemaphore, uint32_t &imageIndex);

    std::optional<AcquiredSwapchainImageInfo> AcquireNextImage(VkSemaphore imageAcquiredSemaphore,
                                                               uint32_t timeout = 1000000000);

    void SetDefaultExtent(const VkExtent2D extent)
    {
        fallbackExtentSize = extent;
        bNeedsRebuild = true;
    }

    VkExtent2D GetExtent() const
    {
        // john - not sure if this should be fallback or the actual one...
        if (swapchainInfo)
        {
            return swapchainInfo->swapchainExtent;
        }
        return fallbackExtentSize;
    }

private:
    bool Recreate();
    void DestroySwapchainResources();
    bool CreateSwapchainResources();

    bool bNeedsRebuild = false;
    FunctionalQueue deletionQueue;

    VkDevice device;
    VkPhysicalDevice physicalDevice;
    VkSurfaceKHR surface;

    std::optional<SwapchainInfo> swapchainInfo;
    std::vector<VkSemaphore> swapchainImageReadyToPresentSemaphores;
    std::vector<VkImageView> swapchainImageViews;

    VkExtent2D fallbackExtentSize;
};

#endif // VULKANCREATESWAPCHAIN_H