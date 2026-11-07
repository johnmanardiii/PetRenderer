//
// Created by johnm on 4/1/2026.
//

#ifndef VULKAN_ENGINE_H
#define VULKAN_ENGINE_H

#define GLFW_INCLUDE_VULKAN

#include "Application.h"
#include "BabysFirstGraphicsPipeline.h"
#include "CreatePipeline.h"
#include "DescriptorAllocator.h"
#include "FrameInFlightStructures.h"
#include "GpuContext.h"
#include "OffscreenRenderImage.h"
#include "Platform.h"

#include "VulkanSwapchainResources.h"
#include "vk_mem_alloc.h"

class VulkanEngine
{
public:
    // #JOHN - maybe initialize with the width / height
    static VulkanEngine *Create();

    bool init();

    // I don't want to think about what would happen if you copied / moved this thing
    VulkanEngine(const VulkanEngine &) = delete;
    VulkanEngine &operator=(const VulkanEngine &) = delete;
    VulkanEngine(VulkanEngine &&) = delete;
    VulkanEngine &operator=(VulkanEngine &&) = delete;

    void Iterate();
    void OnWindowSizeChanged();
    void WaitForDeviceIdle() const;

private:
    VulkanEngine(Platform platform, GpuContext gpuContext, VmaAllocator vmaAllocator,
                 VkExtent2D initialExtent,
                 ScopedFunctionalQueue &&deletionQueue);

    void RenderToOffscreenImage(VkCommandBuffer commandBuffer);
    void BlitOffscreenImageToSwapchainImage(VkCommandBuffer commandBuffer,
                                            AcquiredSwapchainImageInfo &acquiredSwapchainInfo);
    void RenderAndPresentToSwapchain();
    void ResizeOffscreenRenderTarget();

    // This should be destructed last because it kinda owns vulkan. should have more guarantees on that #JOHN
    Platform platform;

    GpuContext gpuContext;

    VmaAllocator vmaAllocator;

    FrameInFlightStructures frameInFlightStructures;

    VulkanSwapchainResources swapchainResources;

    // global submit state tracking (actual engine stuff that is important)
    VkSemaphore globalTimelineSemaphore = VK_NULL_HANDLE; // used to track what submissions on the gpu are done
    uint64_t nextSubmitValue = 1;

    // #JOHN - this should not be an optional, just need to convert out of swapchain for now.
    OffscreenRenderImageInfo offscreenRenderImageInfo;

    // random rendering stuff for the compute shader
    DescriptorInformation descriptorInfo;
    PipelineObjects pipelineObjects;

    BabyFirstGraphicsPipelineInfo babysFirstGraphicsPipelineInfo;

    // #JOHN - this doesn't belong here but need to track it for now
    StuffINeedToTrackForAVertexIndexBuffer vertexBufferInfo;

    ScopedFunctionalQueue deletionQueue;

    uint32_t currentFrame = 0; // always incrementing, doesn't loop around w/ modulo
    float elapsedTime = 0.0f;

    Application app;
};

#endif // VULKAN_ENGINE_H