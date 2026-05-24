//
// Created by johnm on 4/15/2026.
//

#ifndef VULKANCREATESWAPCHAIN_H
#define VULKANCREATESWAPCHAIN_H

#include <SDL3/SDL_video.h>
#include <optional>
#include <vector>
#include <vulkan/vulkan_core.h>

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

    bool Recreate();

    // #JOHN lose out on a fence here but don't really mind
    VkResult PresentImage(VkQueue queue, VkSemaphore waitSemaphore, uint32_t& imageIndex);
    VkResult AcquireNextImage(VkSemaphore imageAcquiredSemaphore, uint32_t& imageIndex, uint32_t timeout = 1000000000);
    VkImage GetImage(uint32_t imageIndex) {return swapchainInfo->swapchainImages[imageIndex];}
    VkSemaphore GetImageAvailableSemaphore(uint32_t imageIndex) { return imageAvailableSemaphores[imageIndex]; }
  private:
    void DestroySwapchainResources();
    bool CreateSwapchainResources();

    bool rebuildAtTopOfFrame = false;

    VkDevice device;
    VkPhysicalDevice physicalDevice;
    VkSurfaceKHR surface;

    std::optional<SwapchainInfo> swapchainInfo;
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkImageView> swapchainImageViews;

    VkExtent2D fallbackExtentSize;
};

#endif // VULKANCREATESWAPCHAIN_H
