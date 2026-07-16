//
// Created by johnm on 4/1/2026.
//

#ifndef VULKAN_ENGINE_H
#define VULKAN_ENGINE_H

#define GLFW_INCLUDE_VULKAN
#include "CreatePipeline.h"
#include "DescriptorAllocator.h"
#include "J3Alloc.h"
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
    void RegisterOnSwapchainRebuildCallbacks();
    void RenderToOffscreenImage(VkCommandBuffer commandBuffer);
    void BlitOffscreenImageToSwapchainImage(VkCommandBuffer commandBuffer, VkImage acquiredImage);
    void RenderAndPresentToSwapchain();
    bool loadedVulkan = false;

    VkInstance instance = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue oneQueueToRuleThemAll = VK_NULL_HANDLE; // #JOHN rename once we have a better idea for this
    std::optional<VulkanCommandStructures> frameInFlightStructures;
    std::optional<PhysicalDeviceInfoWeCareAbout> physDeviceinfo;
    J3Alloc j3a;

    SDL_Window *window = nullptr;

    std::optional<VkSurfaceKHR> surface;
    std::optional<VulkanSwapchainResources> swapchainResources;

    // random rendering stuff
    DescriptorInformation descriptorInfo;
    std::optional<PipelineObjects> pipelineObjects;

    uint32_t currentFrame = 0; // always incrementing, doesn't loop around w/ modulo
    float elapsedTime = 0.0f;
};

#endif // VULKAN_ENGINE_H
