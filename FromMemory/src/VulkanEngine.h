//
// Created by johnm on 4/1/2026.
//

#ifndef VULKAN_ENGINE_H
#define VULKAN_ENGINE_H

#define GLFW_INCLUDE_VULKAN
#include "VulkanCommandBufferCreator.h"
#include "VulkanPhysicalDeviceSelector.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include "VulkanSwapchainResources.h"

#include <chrono>
#include <optional>

class VulkanEngine
{
  public:
    VulkanEngine() = default;
    ~VulkanEngine();

    bool init();

    // I don't want to think about what would happen if you copied / moved this thing
    VulkanEngine(const VulkanEngine &) = delete;
    VulkanEngine &operator=(const VulkanEngine &) = delete;
    VulkanEngine(VulkanEngine &&) = delete;
    VulkanEngine &operator=(VulkanEngine &&) = delete;

    void Iterate();

  private:

    void RenderAndPresentToSwapchain();
    bool loadedVulkan = false;

    std::optional<VkInstance> instance;
    std::optional<VkDevice> device;
    std::optional<PhysicalDeviceInfoWeCareAbout> physDeviceinfo;
    VkQueue oneQueueToRuleThemAll = VK_NULL_HANDLE; // #JOHN rename once we have a better idea for this
    SDL_Window *window = nullptr;
    VulkanCommandStructures* frameInFlightStructures;

    // could probably group these together into a logical class #JOHN and then destroy it as one thing
    std::optional<SwapchainInfo> swapchainInfo;
    std::optional<VkSurfaceKHR> surface;
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::optional<std::vector<VkImageView>> swapchainImageViews;

    uint32_t currentFrame = 0; // always incrementing, doesn't loop around w/ modulo
    float elapsedTime = 0.0f;
};

#endif // VULKAN_ENGINE_H
