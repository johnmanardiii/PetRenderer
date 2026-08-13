//
// Created by johnm on 4/1/2026.
//
// #JOHN - for now engine will be the one that defines the mem allocator but eventually this should probably be
// somewhere else #JOHN - this include order is touchy... should maybe be outside of here.
#include <volk.h>
#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

#include "VulkanEngine.h"

#include "OffscreenRenderImage.h"
#include "SDL3/SDL_timer.h"
#include "VulkanDebugUtil.h"
#include "VulkanSwapchainResources.h"
#include "VulkanUtil.h"

#include <array>
#include <fmt/printf.h>

VulkanEngine *VulkanEngine::Create()
{
    ScopedFunctionalQueue deletionQueue;
    vkchk(volkInitialize());

    int width = 640;
    int height = 480;

    std::optional<Platform> platform = CreatePlatform(width, height, deletionQueue.GetFunctionalQueueRef());
    if (!platform)
    {
        return nullptr;
    }

    // gpu context loads the volk instance / device functions... should be more explicit but whatever
    std::optional<GpuContext> gpuContext = CreateGpuContext(*platform, deletionQueue.GetFunctionalQueueRef());
    if (!gpuContext)
    {
        return nullptr;
    }

    VmaAllocatorCreateInfo allocatorCreateInfo = {};
    allocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
    allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_3; // #JOHN - this should be specified in a single place
    allocatorCreateInfo.physicalDevice = gpuContext->selectedPhysicalDeviceInfo.physicalDevice;
    allocatorCreateInfo.device = gpuContext->device;
    allocatorCreateInfo.instance = gpuContext->instance;

    VmaVulkanFunctions vulkanFunctions;
    vkchk(vmaImportVulkanFunctionsFromVolk(&allocatorCreateInfo, &vulkanFunctions));
    allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;

    VmaAllocator vmaAllocator;
    vkchk(vmaCreateAllocator(&allocatorCreateInfo, &vmaAllocator));

    deletionQueue.Register([vmaAllocator] {
        vmaDestroyAllocator(vmaAllocator);
    });

    return new VulkanEngine(*platform, *gpuContext, vmaAllocator, VkExtent2D{.width = static_cast<uint32_t>(width),
                                                                             .height = static_cast<uint32_t>(height)},
                            std::move(deletionQueue));
}

VulkanEngine::VulkanEngine(Platform inPlatform, GpuContext inGpuContext, VmaAllocator inVmaAllocator,
                           VkExtent2D inInitialExtent,
                           ScopedFunctionalQueue &&inDeletionQueue)
    : platform(inPlatform), gpuContext(inGpuContext), vmaAllocator(inVmaAllocator),
// #JOHN - this "gpu context ref" is super duper dangerous seeming and a bad pattern...
// should read modern C++ design...
      swapchainResources(gpuContext, inInitialExtent), deletionQueue(std::move(inDeletionQueue))

{
    deletionQueue.Register([this] {
       swapchainResources.ReleaseSwapchainResources();
    });
}

bool VulkanEngine::init()
{
    int width = 640;
    int height = 480;

    // #JOHN this doesn't belong directly in the engine, but rather whatever represents our "draw passes" and this goes
    // into the "compute shader draws a picture with 16x16 lines" logic
    AllocateDescriptorInfo(gpuContext.device, descriptorInfo);

    pipelineObjects = CreatePipelineObjects(gpuContext.device, descriptorInfo.descriptorSetLayout);
    deletionQueue.Register([this] {
        DestroyPipelineObjects(gpuContext.device, pipelineObjects);
    });

    babysFirstGraphicsPipelineInfo = CreateBabysFirstGraphicsPipelineInfo();

    VkExtent2D initialOffscreenRenderImage = {.width = static_cast<uint32_t>(width),
                                              .height = static_cast<uint32_t>(height)};

    offscreenRenderImageInfo =
        createOffscreenRenderImageInfo(gpuContext.device, initialOffscreenRenderImage, vmaAllocator);

    deletionQueue.Register(
        [this] {
            DestroyOffscreenRenderImageInfo(gpuContext.device, offscreenRenderImageInfo, vmaAllocator);
        });

    // #JOHN - manually do this for now, just because things are messy, don't submit until a design has been worked out
    // for this
    UpdateOffscreenRenderImageDescriptorSet(gpuContext.device, offscreenRenderImageInfo, descriptorInfo.descriptorSet);

    deletionQueue.Register([this] {
        DestroyDescriptorInformation(gpuContext.device, descriptorInfo);
    });

    frameInFlightStructures = CreateFrameInFlightStructures(
        gpuContext.device, gpuContext.selectedPhysicalDeviceInfo.graphicsPresentQueueFamilyIndex, 3);
    deletionQueue.Register([this] {
        DestroyFrameInFlightStructures(gpuContext.device, frameInFlightStructures);
    });

    VkSemaphoreTypeCreateInfo globalTimelineSemaphoreTypeCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
        .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
        .initialValue = 0 // starts at 0 so that 1 can mean that the first one is done
    };
    VkSemaphoreCreateInfo globalTimelineSemaphoreCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = &globalTimelineSemaphoreTypeCreateInfo,
        .flags = 0,
    };
    vkchk(vkCreateSemaphore(gpuContext.device, &globalTimelineSemaphoreCreateInfo, nullptr, &globalTimelineSemaphore));

    DebugSetName(gpuContext.device, VK_OBJECT_TYPE_SEMAPHORE, globalTimelineSemaphore, "Global Timeline Semaphore");

    deletionQueue.Register([this] {
        vkDestroySemaphore(gpuContext.device, globalTimelineSemaphore, nullptr);
    });

    return true;
}

void VulkanEngine::Iterate()
{
    // render image and present to swapchain
    RenderAndPresentToSwapchain();

    // update what frame we are on for the frames in flight structures
    currentFrame++;
    elapsedTime = SDL_GetTicksNS() / 1000000000.0;
}

void VulkanEngine::OnWindowSizeChanged()
{
    int width, height;
    // there is window size, and window size in pixels... for us we are just going to use the pixel size for now for
    // the renderer
    if (SDL_GetWindowSizeInPixels(platform.window, &width, &height))
    {
        swapchainResources.SetDefaultExtent({static_cast<uint32_t>(width), static_cast<uint32_t>(height)});
    }
    else
    {
        fmt::printf("Failed to get window size: %s\n", SDL_GetError());
        std::abort();
    }
}

void VulkanEngine::WaitForDeviceIdle() const
{
    // once we should close, we wait for the device to be idle:
    if (gpuContext.device != VK_NULL_HANDLE)
    {
        vkDeviceWaitIdle(gpuContext.device);
    }
}

void VulkanEngine::RenderToOffscreenImage(VkCommandBuffer commandBuffer)
{
    // transfer our offscreen render image to be optimal for transfer dst
    // transition the swapchain image from undefined to "writable"
    // wait for "blit" to finish, transfer must be done before clear
    TransitionVkImageFormat({
        .cmd = commandBuffer,
        .srcStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT,
        .srcAccessMask = 0, // don't need to make any data available, its gonna be cleared
        .dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
        .dstAccessMask = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_GENERAL,
        .image = offscreenRenderImageInfo.offscreenImage,
    });

    // bind the pipeline
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineObjects.pipeline);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineObjects.layout, 0, 1,
                            &descriptorInfo.descriptorSet, 0, nullptr);

    // #JOHN - this should be.... the size of the offscreen render target actually like the image itself...
    auto [width, height] = offscreenRenderImageInfo.extent;
    vkCmdDispatch(commandBuffer, ceil(width / 16.0), ceil(height / 16.0), 1);

    // after the offscreen image has been written to, convert it to be transferred over to the swapchain image
    TransitionVkImageFormat({
        .cmd = commandBuffer,
        .srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
        .srcAccessMask = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT,
        .dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_GENERAL,
        .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        .image = offscreenRenderImageInfo.offscreenImage,
    });
}

void VulkanEngine::BlitOffscreenImageToSwapchainImage(VkCommandBuffer commandBuffer,
                                                      AcquiredSwapchainImageInfo &acquiredSwapchainInfo)
{
    // Transition our acquired image to be ready for blit
    TransitionVkImageFormat({
        .cmd = commandBuffer,
        .srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
        .srcAccessMask = 0,
        .dstStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT,
        .dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .image = acquiredSwapchainInfo.image,
    });

    {
        // now it is time to set up the actual blit from the offscreen image into the swapchain image
        // same subresource layers for both since we don't care about mipmaps and just want color
        VkImageSubresourceLayers blitLayers = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, // we only care about the color by the time we are at the swapchain
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1,
        };

        // src == offscreen render target
        VkOffset3D srcOffsets[2];
        srcOffsets[0] = {.x = 0, .y = 0, .z = 0};
        srcOffsets[1] = {.x = static_cast<int32_t>(offscreenRenderImageInfo.extent.width),
                         .y = static_cast<int32_t>(offscreenRenderImageInfo.extent.height),
                         .z = 1};

        VkOffset3D dstOffsets[2];
        dstOffsets[0] = {.x = 0, .y = 0, .z = 0};
        dstOffsets[1] = {.x = static_cast<int32_t>(acquiredSwapchainInfo.extent.width),
                         .y = static_cast<int32_t>(acquiredSwapchainInfo.extent.height),
                         .z = 1};

        VkImageBlit2 offscreenToSwapchainBlit = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2,
            .pNext = nullptr,
            .srcSubresource = blitLayers,
            .srcOffsets = {srcOffsets[0], srcOffsets[1]},
            .dstSubresource = blitLayers,
            .dstOffsets = {dstOffsets[0], dstOffsets[1]},
        };

        // le sigh, another struct to make here
        VkBlitImageInfo2 blitImageInfo = {
            .sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2,
            .srcImage = offscreenRenderImageInfo.offscreenImage,
            .srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            .dstImage = acquiredSwapchainInfo.image,
            .dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .regionCount = 1,
            .pRegions = &offscreenToSwapchainBlit,
            .filter = VK_FILTER_LINEAR,
        };

        vkCmdBlitImage2(commandBuffer, &blitImageInfo);
    }
}

void VulkanEngine::RenderAndPresentToSwapchain()
{
    // Get this frame's sync primitives and command buffer and stuff:
    // #JOHN this really needs to not be an optional... also maybe it should have it's own deletion queue?
    PerFrameInFlightStructure &frameStruct = GetFrameInFlightStructure(frameInFlightStructures, currentFrame);

    // ensure that the gpu is done up until the frame we are trying to access:
    // so it waits until it "hits" a given value... so == at least
    if (frameStruct.frameSubmitted != 0)
    {
        // 0 means uninitialized... only "valid" value we should use for an unsigned int. Otherwise don't wait nothing
        // submitted for this frame of work
        VkSemaphoreWaitInfo waitInfo = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
            .semaphoreCount = 1,
            .pSemaphores = &globalTimelineSemaphore,
            .pValues = &frameStruct.frameSubmitted // #JOHN this isn't submitted yet... needs to be uhhh
        };
        vkchk(vkWaitSemaphores(gpuContext.device, &waitInfo, 1000000000));
    }

    std::optional<AcquiredSwapchainImageInfo> acquiredImageInfo =
        swapchainResources.AcquireNextImage(frameStruct.imageAcquiredSemaphore);
    if (!acquiredImageInfo)
    {
        return;
    }

    if (acquiredImageInfo->extent.width != offscreenRenderImageInfo.extent.width ||
        acquiredImageInfo->extent.height != offscreenRenderImageInfo.extent.height)
    {
        ResizeOffscreenRenderTarget();
    }

    // reset the command buffer associated with this frame
    vkResetCommandBuffer(frameStruct.commandBuffer, 0);

    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    };

    vkBeginCommandBuffer(frameStruct.commandBuffer, &beginInfo);

    RenderToOffscreenImage(frameStruct.commandBuffer);

    BlitOffscreenImageToSwapchainImage(frameStruct.commandBuffer, *acquiredImageInfo);

    // transition swapchain image from being blit to present
    TransitionVkImageFormat({
        .cmd = frameStruct.commandBuffer,
        .srcStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT,
        .srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_2_NONE,
        .dstAccessMask = 0,
        .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .image = acquiredImageInfo->image,
    });

    vkEndCommandBuffer(frameStruct.commandBuffer);

    // Submit command buffer to the queue
    VkCommandBufferSubmitInfo cmdSubmitInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
        .commandBuffer = frameStruct.commandBuffer,
    };

    VkSemaphoreSubmitInfo semaphoreWaitInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .semaphore = frameStruct.imageAcquiredSemaphore,
        .stageMask = VK_PIPELINE_STAGE_TRANSFER_BIT,
    };

    const uint64_t frameComplete = nextSubmitValue++;
    frameStruct.frameSubmitted = frameComplete; // ngl this is a littttle confusing with how I did the indexing.

    const std::array signalInfos{VkSemaphoreSubmitInfo{
                                     .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                                     .semaphore = acquiredImageInfo->imageReadyToPresentSemaphore,
                                     .stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                                 },
                                 VkSemaphoreSubmitInfo{
                                     .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                                     .semaphore = globalTimelineSemaphore,
                                     .value = frameComplete,
                                     .stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                                 }};

    // gonna use the V1 submit info since that seems more simple
    VkSubmitInfo2 submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .pNext = nullptr,
        .flags = 0, // don't care about protected submissions :D
        .waitSemaphoreInfoCount = 1,
        .pWaitSemaphoreInfos = &semaphoreWaitInfo,
        .commandBufferInfoCount = 1,
        .pCommandBufferInfos = &cmdSubmitInfo,
        // when this is signaled, then the image is presentable to a swapchain and in the right format
        .signalSemaphoreInfoCount = static_cast<uint32_t>(signalInfos.size()),
        .pSignalSemaphoreInfos = signalInfos.data(),
    };
    vkchk(vkQueueSubmit2(gpuContext.graphicsPresentQueue, 1, &submitInfo, VK_NULL_HANDLE));

    swapchainResources.PresentImage(gpuContext.graphicsPresentQueue, acquiredImageInfo->imageReadyToPresentSemaphore,
                                    acquiredImageInfo->imageIndex);
}

void VulkanEngine::ResizeOffscreenRenderTarget()
{
    if (swapchainResources.GetExtent().width == 0 || swapchainResources.GetExtent().height == 0)
    {
        // on 0 size, just like... don't do anything and keep the old one around in memory
        return;
    }
    // recreate the offscreen image to be the same size as the swapchain...
    // if the image size is 0 in any dimension, just delete it and set it to nullopt for now
    // #JOHN - the logic here is messed up, keep going for now.

    // #JOHN - this is a per-swapchain resource, so the only reason this works at the moment is because the swapchain
    // currently waits on idle before doing anything, which means that old resources aren't in use and can be destroyed
    // before creation. The real pattern for these swapchain resources you want is more like:
    // swapchain resize -> create new ones -> mark old ones as retired -> top of frame where no longer in use delete
    DestroyOffscreenRenderImageInfo(gpuContext.device, offscreenRenderImageInfo, vmaAllocator);

    offscreenRenderImageInfo =
        createOffscreenRenderImageInfo(gpuContext.device, swapchainResources.GetExtent(), vmaAllocator);
    UpdateOffscreenRenderImageDescriptorSet(gpuContext.device, offscreenRenderImageInfo, descriptorInfo.descriptorSet);
}