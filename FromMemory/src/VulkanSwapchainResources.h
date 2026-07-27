//
// Created by johnm on 4/15/2026.
//

#ifndef VULKANCREATESWAPCHAIN_H
#define VULKANCREATESWAPCHAIN_H

#include "CreateOffscreenRenderImage.h"
#include "FunctionalQueue.h"

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
                             VkExtent2D fallbackExtentSize, J3Alloc* j3Alloc);
    ~VulkanSwapchainResources();

    // don't want this copied around
    VulkanSwapchainResources(const VulkanSwapchainResources &) = delete;
    VulkanSwapchainResources &operator=(const VulkanSwapchainResources &) = delete;

    void Initialize();

    // #JOHN lose out on a fence here but don't really mind
    VkResult PresentImage(VkQueue queue, VkSemaphore waitSemaphore, uint32_t &imageIndex);

    VkResult AcquireNextImage(VkSemaphore imageAcquiredSemaphore, uint32_t &imageIndex, uint32_t timeout = 1000000000);

    VkImage GetImage(uint32_t imageIndex)
    {
        return swapchainInfo->swapchainImages[imageIndex];
    }

    VkSemaphore GetSwapchainImageReadyToPresentSemaphore(uint32_t imageIndex) const
    {
        return swapchainImageReadyToPresentSemaphores[imageIndex];
    }

    [[nodiscard]] VkExtent2D CurrentSwapchainExtent() const
    {
        return swapchainInfo->swapchainExtent;
    }

    [[nodiscard]] std::optional<OffscreenRenderImageInfo> GetOffscreenRenderImageInfo() const
    {
        return offscreenRenderImageInfo;
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

    std::optional<SwapchainInfo> swapchainInfo;
    std::vector<VkSemaphore> swapchainImageReadyToPresentSemaphores;
    std::vector<VkImageView> swapchainImageViews;

    VkExtent2D fallbackExtentSize;
    J3Alloc* j3Alloc;   // #JOHN maybe we should make this a shared ptr? instead of a raw pointer.

    std::optional<OffscreenRenderImageInfo> offscreenRenderImageInfo;
};

#endif // VULKANCREATESWAPCHAIN_H
