//
// Created by johnm on 4/1/2026.
//

#ifndef VULKAN_ENGINE_H
#define VULKAN_ENGINE_H

#define GLFW_INCLUDE_VULKAN

#include "BabysFirstGraphicsPipeline.h"
#include "CreatePipeline.h"
#include "DescriptorAllocator.h"
#include "FlyCamera.h"
#include "FrameInFlightStructures.h"
#include "GpuContext.h"
#include "Model.h"
#include "OffscreenRenderImage.h"
#include "Platform.h"
#include "ImmediateSubmit.h"
#include "VulkanSwapchainResources.h"
#include "vk_mem_alloc.h"
#include "SDL3/SDL_events.h"    // #JOHN - not sure how I feel about handling this here.

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

    void HandleJoyStickEvent(SDL_Event* event);

private:
    VulkanEngine(Platform platform, GpuContext gpuContext, VmaAllocator vmaAllocator, ImmediateSubmitInfo copyQueue,
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

    ImmediateSubmitInfo immediateSubmit;

    VmaAllocator vmaAllocator;

    FrameInFlightStructures frameInFlightStructures;

    VulkanSwapchainResources swapchainResources;

    // global submit state tracking (actual engine stuff that is important)
    VkSemaphore globalTimelineSemaphore = VK_NULL_HANDLE; // used to track what submissions on the gpu are done
    uint64_t nextSubmitValue = 1;

    // #JOHN - this should not be an optional, just need to convert out of swapchain for now.
    AttachmentImageInfo offscreenRenderImageInfo;

    // random rendering stuff for the compute shader
    AttachmentImageDescriptorInformation attachmentImageDescriptorInfo;   // #JOHN - this name is pretty terrible since it is managing exactly 1 descriptor set for the background image
    PipelineObjects pipelineObjects;

    BabyFirstGraphicsPipelineInfo babysFirstGraphicsPipelineInfo;

    // #JOHN - this doesn't belong here but need to track it for now
    BigMeshBufferDescriptorInformation mesh_buffer_descriptor_information;
    GPUMesh exampleMesh;

    ScopedFunctionalQueue deletionQueue;

    FlyCamera camera;

    uint32_t currentFrame = 0; // always incrementing, doesn't loop around w/ modulo
    float elapsedTime = 0.0f;
    float deltaTime = 0.0f;
};

#endif // VULKAN_ENGINE_H