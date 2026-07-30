//
// Created by johnm on 4/1/2026.
//

#ifndef VULKAN_ENGINE_H
#define VULKAN_ENGINE_H

#define GLFW_INCLUDE_VULKAN
#include "CreatePipeline.h"
#include "DescriptorAllocator.h"
#include "FrameInFlightStructures.h"
#include "GpuContext.h"
#include "J3Alloc.h"
#include "Platform.h"
#include "VulkanPhysicalDeviceSelector.h"

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
    void OnWindowSizeChanged();

  private:
    void RegisterOnSwapchainCreatedCallbacks();
    void RenderToOffscreenImage(VkCommandBuffer commandBuffer);
    void BlitOffscreenImageToSwapchainImage(VkCommandBuffer commandBuffer, VkImage acquiredImage);
    void RenderAndPresentToSwapchain();
    void HandleSwapchainOutOfDate();

    // This should be destructed last because it kinda owns vulkan. should have more guarantees on that #JOHN
    std::optional<Platform> platform;

    std::optional<GpuContext> gpuContext;

    std::optional<FrameInFlightStructures> frameInFlightStructures;

    J3Alloc j3a;

    std::optional<VulkanSwapchainResources> swapchainResources;

    // #JOHN - this should not be an optional, just need to convert out of swapchain for now.
    std::optional<OffscreenRenderImageInfo> offscreenRenderImageInfo;

    // random rendering stuff
    DescriptorInformation descriptorInfo;
    std::optional<PipelineObjects> pipelineObjects;

    uint32_t currentFrame = 0; // always incrementing, doesn't loop around w/ modulo
    float elapsedTime = 0.0f;


};

#endif // VULKAN_ENGINE_H
