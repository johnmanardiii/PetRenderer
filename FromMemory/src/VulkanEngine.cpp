//
// Created by johnm on 4/1/2026.
//
#include "VulkanEngine.h"

#include "VulkanInstanceBuilder.h"
#include "VulkanLogicalDeviceCreator.h"
#include "VulkanPhysicalDeviceSelector.h"
#include "VulkanSwapchainResources.h"
#include "VulkanUtil.h"
#include "vulkan/vulkan_core.h"

#include "glm/trigonometric.hpp"

#include <fmt/printf.h>
#include <vector>

VulkanEngine::~VulkanEngine()
{
    // once we should close, we wait for the device to be idle:
    vkDeviceWaitIdle(*device);

    frameInFlightStructures.reset(); // not sure if this is the right semantics but it works
    swapchainResources.reset();

    if (device)
    {
        vkDestroyDevice(*device, nullptr);
    }

    if (instance.has_value() && surface.has_value())
    {
        vkDestroySurfaceKHR(*instance, *surface, nullptr);
    }

    // cleanup vulkan handle stuff (move this to RAII class engine) #JOHN
    if (instance)
    {
        vkDestroyInstance(*instance, nullptr);
    }

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

    // #JOHN move these values out to the class itself
    int width = 640;
    int height = 480;
    window = SDL_CreateWindow("Pet Renderer", width, height,
                              SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE); // #JOHN if I ever want resizability it goes here

    // Get a vulkan window surface from glfw
    VkSurfaceKHR surface_handle;

    if (!SDL_Vulkan_CreateSurface(window, *instance, nullptr, &surface_handle))
    {
        fmt::printf("Failed to create vulkan surface\n");
        return false;
    }

    // #JOHN doesn't seem like it follows the most awesome pattern but whatevs.
    // really can't fail in this way... why optional?
    surface = surface_handle;

    // gather a valid physical device
    VulkanPhysicalDeviceSelector selector;
    physDeviceinfo = selector.selectPhysicalDevice(*instance, *surface);

    if (!physDeviceinfo)
    {
        fmt::printf("Failed to find a valid physical device.\n");
        return false;
    }

    // create a logical device from the physical one
    device = VulkanLogicalDeviceCreator::CreateLogicalDevice(*physDeviceinfo);
    if (!device)
    {
        fmt::printf("Failed to create logical device.\n");
        return false;
    }

    // assign our queue created from the device
    vkGetDeviceQueue(*device, physDeviceinfo->graphicsPresentQueueFamilyIndex, 0, &oneQueueToRuleThemAll);

    swapchainResources.emplace(*device, physDeviceinfo->physicalDevice, *surface,
                                 VkExtent2D{.width = static_cast<uint32_t>(width), .height = static_cast<uint32_t>(height)});

    frameInFlightStructures = VulkanCommandStructures(*device, physDeviceinfo->graphicsPresentQueueFamilyIndex, 3);

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

void VulkanEngine::RenderAndPresentToSwapchain()
{
    // Get this frame's sync primitives and command buffer and stuff:
    PerFrameInFlightStructure &frameStruct = frameInFlightStructures->getPerFrameInFlightStructure(currentFrame);

    // wait on "this frame in flight" to be signaled
    vkWaitForFences(*device, 1, &frameStruct.queueFinishedFence, VK_TRUE, UINT64_MAX);

    uint32_t imageAcquiredIndex = 0;
    VkResult acquireResult = (swapchainResources->AcquireNextImage(frameStruct.imageAcquiredSemaphore, imageAcquiredIndex));

    if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR)
    {
        return;
    }
    vkchk(acquireResult);
    // once the image acquired semaphore goes off, we have the image in the pipeline

    // reset the fence, valid
    vkResetFences(*device, 1, &frameStruct.queueFinishedFence);

    // reset the command buffer associated with this frame
    vkResetCommandBuffer(frameStruct.commandBuffer, 0);

    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    };

    vkBeginCommandBuffer(frameStruct.commandBuffer, &beginInfo);

    // get the clear image from our list of images
    VkImage acquiredImage = swapchainResources->GetImage(imageAcquiredIndex);

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
    VkImageMemoryBarrier2 undefinedToWritable = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .pNext = nullptr,
        // #JOHN understanding of this is... the stage where "at least this must be completed by previously submitted
        // things" we want... IDK I guess all commands to be finished? I don't think there is anything super important
        // before this
        .srcStageMask = VK_PIPELINE_STAGE_2_NONE,
        .srcAccessMask = 0, // apparently this doesn't matter, we don't care about the undefined, just acquired mem
        // this means "what stages need to wait on this thing to be done" before continuing, which is like whatever
        // vkclear needs?
        .dstStageMask = VK_PIPELINE_STAGE_2_CLEAR_BIT, // this is just the clear one
        .dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        // this may be worse than general for scaling up, so #JOHn remember to change when doing more than clearing
        .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = acquiredImage, // the swapchain image we want to clear and have acquired by index from the swapchain
        .subresourceRange = subresourceRange};

    // now we wrap this monster in a uhh dependency info struct (turducken-like API design right here):
    VkDependencyInfo dependencyInfoToWritable = {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &undefinedToWritable,
    };

    // record the pipeline barrier transitioning the image to transferrable
    vkCmdPipelineBarrier2(frameStruct.commandBuffer, &dependencyInfoToWritable);

    // now we clear it (I don't care about time yet that will be easy to change let's just make it blue):
    // gonna assume everything is in BGRA for this test-like code
    const VkClearColorValue clearColor = {.float32 = {0.0f, (glm::sin(elapsedTime * 2) + 1) * .5f, 0.0, 1.0f}};

    vkCmdClearColorImage(frameStruct.commandBuffer, acquiredImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearColor, 1,
                         &subresourceRange);
    // Ok now we gotta wait on this to finish (set src to whatever it is we need here like finish transferring

    // transition the swapchain image from undefined to "writable"
    VkImageMemoryBarrier2 writableToPresentable = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .pNext = nullptr,

        // the last thing we did was clear (how does anyone keep these connections all in their head?)
        .srcStageMask = VK_PIPELINE_STAGE_2_CLEAR_BIT,
        .srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
        // the next thing we are going to do is present...
        // so the semaphore will tell the presentation engine to go forth and venture into presentation land
        .dstStageMask = VK_PIPELINE_STAGE_2_NONE, // #JOHN this doesn't seem right...
        .dstAccessMask = 0,

        .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = acquiredImage, // the swapchain image we want to clear and have acquired by index from the swapchain
        .subresourceRange = subresourceRange};

    // now we wrap this monster in a uhh dependency info struct (turducken-like API design right here):
    VkDependencyInfo dependencyInfoToPresentable = {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &writableToPresentable,
    };

    // record the pipeline barrier transitioning the image to transferrable
    vkCmdPipelineBarrier2(frameStruct.commandBuffer, &dependencyInfoToPresentable);

    VkCommandBufferSubmitInfo cmdSubmitInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
        .commandBuffer = frameStruct.commandBuffer,
    };

    VkSemaphore waitSemaphore = swapchainResources->GetImageAvailableSemaphore(imageAcquiredIndex);

    VkSemaphoreSubmitInfo semaphoreSubmitInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .semaphore = waitSemaphore,
        .stageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT // #JOHN might be too much but whatever
    };

    VkSemaphoreSubmitInfo semaphoreWaitInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .semaphore = frameStruct.imageAcquiredSemaphore,
        .stageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT // #JOHN might be too much but whatever
    };

    // gonna use the V1 submit info since that seems more simple
    VkSubmitInfo2 submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .pNext = nullptr,
        .flags = 0, // don't care about protected submissions :D
        // gonna be explicit about not needing a wait to submit the info for this, since the initial semaphores in this
        // already handle that
        .waitSemaphoreInfoCount = 1,
        .pWaitSemaphoreInfos = &semaphoreWaitInfo,
        .commandBufferInfoCount = 1,
        .pCommandBufferInfos = &cmdSubmitInfo,
        // when this is signaled, then the image is presentable to a swapchain and in the right format
        .signalSemaphoreInfoCount = 1,
        .pSignalSemaphoreInfos = &semaphoreSubmitInfo,
    };

    vkEndCommandBuffer(frameStruct.commandBuffer);

    // OK now we finally submit this thing to the queue
    // #JOHN I don't think we need the fence for this part? the semaphore signal in this being done handles that I think
    vkchk(vkQueueSubmit2(oneQueueToRuleThemAll, 1, &submitInfo, frameStruct.queueFinishedFence));

    vkchk(swapchainResources->PresentImage(oneQueueToRuleThemAll, waitSemaphore, imageAcquiredIndex));
}