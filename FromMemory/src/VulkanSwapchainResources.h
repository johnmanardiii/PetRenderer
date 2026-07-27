//
// Created by johnm on 4/15/2026.
//

#ifndef VULKANCREATESWAPCHAIN_H
#define VULKANCREATESWAPCHAIN_H

#include "FunctionalQueue.h"
#include "OffscreenRenderImage.h"

#include "Signal.h"
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
    VkResult PresentImage(VkQueue queue, VkSemaphore waitSemaphore, uint32_t &imageIndex);

    VkResult AcquireNextImage(VkSemaphore imageAcquiredSemaphore, uint32_t &imageIndex, uint32_t timeout = 1000000000);

    VkImage GetImage(uint32_t imageIndex)
    {
        return swapchainInfo.swapchainImages[imageIndex];
    }

    VkSemaphore GetSwapchainImageReadyToPresentSemaphore(uint32_t imageIndex) const
    {
        return swapchainImageReadyToPresentSemaphores[imageIndex];
    }

    [[nodiscard]] VkExtent2D CurrentSwapchainExtent() const
    {
        return swapchainInfo.swapchainExtent;
    }

    Signal<> onSwapchainCreated;

    bool Recreate();

private:
    FunctionalQueue deletionQueue;

    void DestroySwapchainResources();
    bool CreateSwapchainResources();

    VkDevice device;
    VkPhysicalDevice physicalDevice;
    VkSurfaceKHR surface;

    SwapchainInfo swapchainInfo;
    std::vector<VkSemaphore> swapchainImageReadyToPresentSemaphores;
    std::vector<VkImageView> swapchainImageViews;

    VkExtent2D fallbackExtentSize;
};

#endif // VULKANCREATESWAPCHAIN_H
