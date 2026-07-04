//
// Created by johnm on 4/1/2026.
//
#include "VulkanEngine.h"

#include "VulkanDebugUtil.h"
#include "VulkanInstanceBuilder.h"
#include "VulkanLogicalDeviceCreator.h"
#include "VulkanPhysicalDeviceSelector.h"
#include "VulkanSwapchainResources.h"
#include "VulkanUtil.h"

#include "glm/trigonometric.hpp"

#include <fmt/printf.h>
#include <vector>
#include <volk.h>

VulkanEngine::~VulkanEngine()
{
    // once we should close, we wait for the device to be idle:
    if (device != VK_NULL_HANDLE)
    {
        vkDeviceWaitIdle(device);
        pipelineObjects.reset();

        DestroyDescriptorInformation(device, descriptorInfo);

        frameInFlightStructures.reset(); // not sure if this is the right semantics but it works
        swapchainResources.reset();

        vkDestroyDevice(device, nullptr);
        device = VK_NULL_HANDLE;
    }

    if (instance != VK_NULL_HANDLE && surface.has_value())
    {
        vkDestroySurfaceKHR(instance, *surface, nullptr);
    }

    // cleanup vulkan handle stuff (move this to RAII class engine) #JOHN
    // (vulkan handles NULL handle for us)
    vkDestroyInstance(instance, nullptr);
    instance = VK_NULL_HANDLE;

    if (window)
    {
        SDL_DestroyWindow(window);
    }

    if (loadedVulkan)
    {
        SDL_Vulkan_UnloadLibrary();
    }

    SDL_Quit(); // #JOHN honestly wrap this in RAII too since it is annoying to manage
}

bool VulkanEngine::init()
{
    // setup volk
    volkInitialize();

    // #JOHN there may need to be some loader madness that happens here (volk)
    // but it is working without it so we move on
    /* Initialize the library */
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        fmt::printf("Failed to initialize GLFW\n");
        return false;
    }

    // load vulkan (can be passed in a path here)
    if (!SDL_Vulkan_LoadLibrary(nullptr))
    {
        fmt::printf("Failed to initialize SDL_Vulkan_LoadLibrary: %s\n", SDL_GetError());
        return false;
    }
    loadedVulkan = true;

    uint32_t count;
    char const *const *extensions = SDL_Vulkan_GetInstanceExtensions(&count);
    // convert to vector since I don't like that double pointer stuff
    std::vector<const char *> requiredExtensions;
    for (uint32_t i = 0; i < count; i++)
    {
        requiredExtensions.push_back(extensions[i]);
    }
    requiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    requiredExtensions.push_back(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME);
    // #JOHN need to do some querying before you can push this back... vkEnumerateInstanceExtensionProperties
    //   https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkEnumerateInstanceExtensionProperties.html
    // something something need to query the layers or something to enable the extensions in the actual list
    // something about "implicitly enable layers / explitly enabled layers"
    requiredExtensions.push_back(VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME);

    // #JOHN you need to pass the vulkan version here.
    instance = VulkanInstanceBuilder()
                   .SetRequiredLayers({"VK_LAYER_KHRONOS_validation"})
                   .SetRequiredExtensions(requiredExtensions)
                   .EnableDebugMessengerForInstanceCreation()
                   .GetInstance();

    if (!instance)
    {
        fmt::printf("Failed to get vulkan instance. exiting");
        return false;
    }

    volkLoadInstanceOnly(instance);

    // #JOHN move these values out to the class itself
    int width = 640;
    int height = 480;
    window =
        SDL_CreateWindow("Pet Renderer", width, height,
                         SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE); // #JOHN if I ever want resizability it goes here

    // Get a vulkan window surface from glfw
    VkSurfaceKHR surface_handle;

    if (!SDL_Vulkan_CreateSurface(window, instance, nullptr, &surface_handle))
    {
        fmt::printf("Failed to create vulkan surface\n");
        return false;
    }

    // #JOHN doesn't seem like it follows the most awesome pattern but whatevs.
    // really can't fail in this way... why optional?
    surface = surface_handle;

    // gather a valid physical device
    VulkanPhysicalDeviceSelector selector;
    physDeviceinfo = selector.selectPhysicalDevice(instance, *surface);

    if (!physDeviceinfo)
    {
        fmt::printf("Failed to find a valid physical device.\n");
        return false;
    }

    // create a logical device from the physical one
    device = CreateLogicalDevice(*physDeviceinfo);
    if (!device)
    {
        fmt::printf("Failed to create logical device.\n");
        return false;
    }

    volkLoadDevice(device);

    j3a.Initialize(physDeviceinfo->physicalDevice, device);

    // assign our queue created from the device
    vkGetDeviceQueue(device, physDeviceinfo->graphicsPresentQueueFamilyIndex, 0, &oneQueueToRuleThemAll);

    // Name this one queue
    DebugSetName(device, VK_OBJECT_TYPE_QUEUE, oneQueueToRuleThemAll, "One Graphics Queue I am using for everything");

    swapchainResources.emplace(
        device, physDeviceinfo->physicalDevice, *surface,
        VkExtent2D{.width = static_cast<uint32_t>(width), .height = static_cast<uint32_t>(height)}, &j3a);

    frameInFlightStructures = VulkanCommandStructures(device, physDeviceinfo->graphicsPresentQueueFamilyIndex, 3);

    // #JOHN this doesn't belong directly in the engine, but rather whatever represents our "draw passes" and this goes
    // into the "compute shader draws a picture with 16x16 lines" logic
    AllocateDescriptorInfo(device, descriptorInfo);

    pipelineObjects.emplace(device, descriptorInfo.descriptorSetLayout);

    // #JOHN - kind of inefficient here but update the descriptor set to the image, feel like i could do this once at
    // startup
    std::optional<OffscreenRenderImageInfo> offscreenRenderImageInfo =
        swapchainResources->GetOffscreenRenderImageInfo();
    assert(offscreenRenderImageInfo.has_value());
    const VkDescriptorImageInfo imageInfo = {.imageView = offscreenRenderImageInfo->offscreenImageView,
                                             .imageLayout = VK_IMAGE_LAYOUT_GENERAL};
    VkWriteDescriptorSet writeDescriptorSet = {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                               .dstSet = descriptorInfo.descriptorSet,
                                               .dstBinding = 0,
                                               .dstArrayElement = 0,
                                               .descriptorCount = 1,
                                               .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                                               .pImageInfo = &imageInfo};
    // #JOHN in a real program... when is this type of thing called?
    vkUpdateDescriptorSets(device, 1, &writeDescriptorSet, 0, nullptr);

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

void VulkanEngine::RenderToOffscreenImage(VkCommandBuffer commandBuffer)
{
    std::optional<OffscreenRenderImageInfo> offscreenRenderImageInfo =
        swapchainResources->GetOffscreenRenderImageInfo();
    if (!offscreenRenderImageInfo)
    {
        return;
    }
    // we have a reset command buffer and fence is ready so we are good to go

    // transfer our offscreen render image to be optimal for transfer dst
    // transition the swapchain image from undefined to "writable"
    // wait for "blit" to finish, transfer must be done before clear

    VkImageSubresourceRange subresourceRange = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1,
    };

    VkImageMemoryBarrier2 undefinedToWritable = {.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                                                 .pNext = nullptr,
                                                 .srcStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT,
                                                 .srcAccessMask =
                                                     0, // don't need to make any data available, its gonna be cleared
                                                 .dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                                                 .dstAccessMask = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
                                                 .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                                                 .newLayout = VK_IMAGE_LAYOUT_GENERAL,
                                                 .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                                 .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                                 .image = offscreenRenderImageInfo->offscreenImage,
                                                 .subresourceRange = subresourceRange};

    VkDependencyInfo dependencyInfoToWritable = {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &undefinedToWritable,
    };

    // record the pipeline barrier transitioning the image to transferrable
    vkCmdPipelineBarrier2(commandBuffer, &dependencyInfoToWritable);

    // bind the pipeline
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineObjects->pipeline);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineObjects->layout, 0, 1,
                            &descriptorInfo.descriptorSet, 0, nullptr);

    auto [width, height] = swapchainResources->CurrentSwapchainExtent();
    vkCmdDispatch(commandBuffer, ceil(width / 16.0), ceil(height / 16.0), 1);

    VkImageMemoryBarrier2 offscreenToTransferRead = {.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                                                     .pNext = nullptr,
                                                     .srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                                                     .srcAccessMask = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
                                                     .dstStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT,
                                                     .dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT,
                                                     .oldLayout = VK_IMAGE_LAYOUT_GENERAL,
                                                     .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                                     .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                                     .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                                     .image = offscreenRenderImageInfo->offscreenImage,
                                                     .subresourceRange = subresourceRange};

    VkDependencyInfo dependencyInfoToPresentable = {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &offscreenToTransferRead,
    };

    // record the pipeline barrier transitioning the image to transferrable
    vkCmdPipelineBarrier2(commandBuffer, &dependencyInfoToPresentable);
}
void VulkanEngine::BlitOffscreenImageToSwapchainImage(VkCommandBuffer commandBuffer, VkImage acquiredImage)
{
    if (!swapchainResources)
    {
        return;
    }
    std::optional<OffscreenRenderImageInfo> offscreenRenderImageInfo =
        swapchainResources->GetOffscreenRenderImageInfo();

    if (!offscreenRenderImageInfo)
    {
        // #JOHN - not a huge fan of this pattern w/ the optionals.
        return;
    }

    VkImageSubresourceRange subresourceRange = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1,
    };

    {
        // Transition the acquired swapchain image to a transfer destination (from undefined because we don't know what
        // was in it)

        // transition the swapchain image from undefined to "writable" for blit dst
        VkImageMemoryBarrier2 undefinedToTransferDst = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .pNext = nullptr,
            .srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT,
            .srcAccessMask = 0,                           // we don't care about last swapchain image contents here:
            .dstStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT, // this is just the clear one
            .dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image =
                acquiredImage, // the swapchain image we want to clear and have acquired by index from the swapchain
            .subresourceRange = subresourceRange};

        VkDependencyInfo dependencyInfoUndefinedToTransferDst = {
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers = &undefinedToTransferDst,
        };

        vkCmdPipelineBarrier2(commandBuffer, &dependencyInfoUndefinedToTransferDst);
    }

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
    vkWaitForFences(device, 1, &frameStruct.queueFinishedFence, VK_TRUE, UINT64_MAX);

    uint32_t imageAcquiredIndex = 0;
    // #JOHN this may need to tell the external system of an update... so that the engine itself can rebuild whatever
    // images it needs. maybe I'll have an internal rendering resolution and not resize but instead go for "settings"
    // like other games.
    VkResult acquireResult =
        (swapchainResources->AcquireNextImage(frameStruct.imageAcquiredSemaphore, imageAcquiredIndex));
    if (swapchainResources->bJustRebuilt)
    {
        std::optional<OffscreenRenderImageInfo> offscreenRenderImageInfo =
            swapchainResources->GetOffscreenRenderImageInfo();
        assert(offscreenRenderImageInfo.has_value());
        const VkDescriptorImageInfo imageInfo = {.imageView = offscreenRenderImageInfo->offscreenImageView,
                                                 .imageLayout = VK_IMAGE_LAYOUT_GENERAL};
        VkWriteDescriptorSet writeDescriptorSet = {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                                   .dstSet = descriptorInfo.descriptorSet,
                                                   .dstBinding = 0,
                                                   .dstArrayElement = 0,
                                                   .descriptorCount = 1,
                                                   .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                                                   .pImageInfo = &imageInfo};
        // #JOHN in a real program... when is this type of thing called?
        vkUpdateDescriptorSets(device, 1, &writeDescriptorSet, 0, nullptr);
    }

    if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR)
    {
        return;
    }
    vkchk(acquireResult);
    // once the image acquired semaphore goes off, we have the image in the pipeline

    // reset the fence, valid
    vkResetFences(device, 1, &frameStruct.queueFinishedFence);

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

    // #JOHN once done, convert this whole "turn image from this to that" insane amt of code
    // #JOHN temp little subresource range we are going to use to describe our baby image:
    VkImageSubresourceRange subresourceRange = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1,
    };

    // transition the swapchain image from undefined to "writable"
    VkImageMemoryBarrier2 writableToPresentable = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .pNext = nullptr,
        .srcStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT, // last thing we did to the swapchain image was blit
        .srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_2_NONE, // what should wait on this...? is anything else a stage? no..
        .dstAccessMask = 0,
        .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = acquiredImage,
        .subresourceRange = subresourceRange};

    VkDependencyInfo dependencyInfoToPresentable = {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &writableToPresentable,
    };

    // record the pipeline barrier transitioning the image to transferrable
    vkCmdPipelineBarrier2(frameStruct.commandBuffer, &dependencyInfoToPresentable);

    vkEndCommandBuffer(frameStruct.commandBuffer);

    // Submit command buffer to the queue
    VkCommandBufferSubmitInfo cmdSubmitInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
        .commandBuffer = frameStruct.commandBuffer,
    };

    VkSemaphoreSubmitInfo semaphoreWaitInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .semaphore = frameStruct.imageAcquiredSemaphore,
        // make the transfer wait for this semaphore... its the first thing that happens to the swapchain image is that
        // it gets transferred to
        .stageMask = VK_PIPELINE_STAGE_TRANSFER_BIT,
    };

    VkSemaphore swapchainImageReadySemaphore =
        swapchainResources->GetSwapchainImageReadyToPresentSemaphore(imageAcquiredIndex);

    VkSemaphoreSubmitInfo semaphoreSubmitInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .semaphore = swapchainImageReadySemaphore,
        .stageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT // #JOHN might be too much but whatever
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
    vkchk(vkQueueSubmit2(oneQueueToRuleThemAll, 1, &submitInfo, frameStruct.queueFinishedFence));

    vkchk(swapchainResources->PresentImage(oneQueueToRuleThemAll, swapchainImageReadySemaphore, imageAcquiredIndex));
}