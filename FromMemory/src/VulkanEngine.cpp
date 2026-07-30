//
// Created by johnm on 4/1/2026.
//
#include "VulkanEngine.h"

#include "SDL3/SDL_timer.h"
#include "VulkanSwapchainResources.h"
#include "VulkanUtil.h"
#include <fmt/printf.h>
#include <volk.h>

VulkanEngine::~VulkanEngine()
{
    // once we should close, we wait for the device to be idle:
    if (gpuContext && gpuContext->Device() != VK_NULL_HANDLE)
    {
        // #JOHN maybe... make this a thing that on destruct waits idle and make it a member that way tis all in one
        // place and maybe consider... moving it into a more explicitly managed struct, but we are locked in
        // on this pattern for nwo.
        vkDeviceWaitIdle(gpuContext->Device());

        if (offscreenRenderImageInfo)
        {
            DestroyOffscreenRenderImageInfo(gpuContext->Device(), *offscreenRenderImageInfo);
            offscreenRenderImageInfo.reset();
        }

        pipelineObjects.reset();

        DestroyDescriptorInformation(gpuContext->Device(), descriptorInfo);

        frameInFlightStructures.reset(); // not sure if this is the right semantics but it works
        swapchainResources.reset();
    }
}

bool VulkanEngine::init()
{
    volkInitialize();

    // #JOHN - temp code b4 we remove this awful optional pattern
    int width = 640;
    int height = 480;

    platform = Platform::Create(width, height);
    if (!platform)
        return false;

    gpuContext = GpuContext::Create(*platform);
    if (!gpuContext)
        return false;

    // #JOHN - not a fan of this "initialize" freakin thing man... maybe just put it in gpu context
    j3a.Initialize(gpuContext->PhysicalDevice(), gpuContext->Device());

    // #JOHN - should these be based on some window properties? Feel like the whole resize should be based on window.
    // #JOHN - this should probably take the context itself and not all dis
    swapchainResources.emplace(
        gpuContext->Device(), gpuContext->PhysicalDevice(), gpuContext->Surface(),
        VkExtent2D{.width = static_cast<uint32_t>(width), .height = static_cast<uint32_t>(height)});

    // #JOHN this doesn't belong directly in the engine, but rather whatever represents our "draw passes" and this goes
    // into the "compute shader draws a picture with 16x16 lines" logic
    AllocateDescriptorInfo(gpuContext->Device(), descriptorInfo);

    pipelineObjects.emplace(gpuContext->Device(), descriptorInfo.descriptorSetLayout);

    // #JOHN - hidden dependency on the "pipeline objects" and descriptor set stuff.
    // #JOHN - kinda gross ordering depdendency right here.
    offscreenRenderImageInfo.emplace(
        createOffscreenRenderImageInfo(gpuContext->Device(), swapchainResources->CurrentSwapchainExtent(), &j3a));

    // #JOHN - manually do this for now, just because things are messy, don't submit until a design has been worked out
    // for this
    UpdateOffscreenRenderImageDescriptorSet(gpuContext->Device(), *offscreenRenderImageInfo, descriptorInfo.descriptorSet);

    frameInFlightStructures =
        FrameInFlightStructures::Create(gpuContext->Device(), gpuContext->GraphicsPresentQueueIndex(), 3);

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
    if (platform)
    {
        int width, height;
        // there is window size, and window size in pixels... for us we are just going to use the pixel size for now for the
        // renderer
        SDL_GetWindowSizeInPixels(platform->GetWindow(), &width, &height);

        swapchainResources->SetDefaultExtent({static_cast<uint32_t>(width), static_cast<uint32_t>(height)});
    }
}

void VulkanEngine::RenderToOffscreenImage(VkCommandBuffer commandBuffer)
{
    if (!offscreenRenderImageInfo)
    {
        return;
    }

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
        .image = offscreenRenderImageInfo->offscreenImage,
    });

    // bind the pipeline
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineObjects->pipeline);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineObjects->layout, 0, 1,
                            &descriptorInfo.descriptorSet, 0, nullptr);

    auto [width, height] = swapchainResources->CurrentSwapchainExtent();
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
        .image = offscreenRenderImageInfo->offscreenImage,
    });
}

void VulkanEngine::BlitOffscreenImageToSwapchainImage(VkCommandBuffer commandBuffer, VkImage acquiredImage)
{
    if (!swapchainResources)
    {
        return;
    }

    if (!offscreenRenderImageInfo)
    {
        // #JOHN - not a huge fan of this pattern w/ the optionals.
        return;
    }

    // Transition our acquired image to be ready for blit
    TransitionVkImageFormat({
        .cmd = commandBuffer,
        .srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
        .srcAccessMask = 0,
        .dstStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT,
        .dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .image = acquiredImage,
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
        srcOffsets[1] = {.x = static_cast<int32_t>(offscreenRenderImageInfo->extent.width),
                         .y = static_cast<int32_t>(offscreenRenderImageInfo->extent.height),
                         .z = 1};

        VkExtent2D swapchainExtent = swapchainResources->CurrentSwapchainExtent();
        VkOffset3D dstOffsets[2];
        dstOffsets[0] = {.x = 0, .y = 0, .z = 0};
        dstOffsets[1] = {.x = static_cast<int32_t>(swapchainExtent.width),
                         .y = static_cast<int32_t>(swapchainExtent.height),
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
            .srcImage = offscreenRenderImageInfo->offscreenImage,
            .srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            .dstImage = acquiredImage,
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
    PerFrameInFlightStructure &frameStruct = frameInFlightStructures->getPerFrameInFlightStructure(currentFrame);

    // wait on "this frame in flight" to be signaled
    vkWaitForFences(gpuContext->Device(), 1, &frameStruct.queueFinishedFence, VK_TRUE, UINT64_MAX);

    uint32_t imageAcquiredIndex = 0;
    if (!swapchainResources->AcquireNextImage(frameStruct.imageAcquiredSemaphore, imageAcquiredIndex))
    {
        return;
    }

    // reset the fence, valid
    vkResetFences(gpuContext->Device(), 1, &frameStruct.queueFinishedFence);

    // reset the command buffer associated with this frame
    vkResetCommandBuffer(frameStruct.commandBuffer, 0);

    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    };

    vkBeginCommandBuffer(frameStruct.commandBuffer, &beginInfo);

    RenderToOffscreenImage(frameStruct.commandBuffer);

    // transfer swapchain image to blit transfer dst
    VkImage acquiredImage = swapchainResources->GetImage(imageAcquiredIndex);

    BlitOffscreenImageToSwapchainImage(frameStruct.commandBuffer, acquiredImage);

    // transition swapchain image from being blit to present
    TransitionVkImageFormat({
        .cmd = frameStruct.commandBuffer,
        .srcStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT,
        .srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_2_NONE,
        .dstAccessMask = 0,
        .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .image = acquiredImage,
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

    VkSemaphore swapchainImageReadySemaphore =
        swapchainResources->GetSwapchainImageReadyToPresentSemaphore(imageAcquiredIndex);

    VkSemaphoreSubmitInfo semaphoreSubmitInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .semaphore = swapchainImageReadySemaphore,
        .stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
    };

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
        .signalSemaphoreInfoCount = 1,
        .pSignalSemaphoreInfos = &semaphoreSubmitInfo,
    };
    vkchk(vkQueueSubmit2(gpuContext->GraphicsPresentLogicalQueue(), 1, &submitInfo, frameStruct.queueFinishedFence));

    swapchainResources->PresentImage(gpuContext->GraphicsPresentLogicalQueue(),
                                                              swapchainImageReadySemaphore, imageAcquiredIndex);
}

void VulkanEngine::HandleSwapchainOutOfDate()
{
    // #JOHN - this actually needs to rebuild the image...
    UpdateOffscreenRenderImageDescriptorSet(gpuContext->Device(), *offscreenRenderImageInfo, descriptorInfo.descriptorSet);
}