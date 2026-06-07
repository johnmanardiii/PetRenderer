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
    if (device)
    {
        vkDeviceWaitIdle(*device);
    }

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

    volkLoadInstanceOnly(*instance);

    // #JOHN move these values out to the class itself
    int width = 640;
    int height = 480;
    window =
        SDL_CreateWindow("Pet Renderer", width, height,
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

    volkLoadDevice(*device);

    j3a.Initialize(physDeviceinfo->physicalDevice, *device);

    // assign our queue created from the device
    vkGetDeviceQueue(*device, physDeviceinfo->graphicsPresentQueueFamilyIndex, 0, &oneQueueToRuleThemAll);

    // Name this one queue
    DebugSetName(*device, VK_OBJECT_TYPE_QUEUE, oneQueueToRuleThemAll, "One Graphics Queue I am using for everything");

    swapchainResources.emplace(
        *device, physDeviceinfo->physicalDevice, *surface,
        VkExtent2D{.width = static_cast<uint32_t>(width), .height = static_cast<uint32_t>(height)});

    // #JOHN in the future... tie this to swapchain creation and destruction via observer pattern
    CreateOffscreenRenderImage(swapchainResources->CurrentSwapchainExtent());

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

VkImageView VulkanEngine::CreateOffscreenRenderImage(VkExtent2D extent)
{
    // #JOHN

    // Create the image struct based on the params
    VkImageCreateInfo imageCreateInfo = {.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    imageCreateInfo.flags = 0;
    // these... should probably be passed in... and honestly should the
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
    imageCreateInfo.extent = VkExtent3D{extent.width, extent.height, 1};
    imageCreateInfo.mipLevels = 1;
    imageCreateInfo.arrayLayers = 1;
    // #JOHN need to look into MSAA more. However for now... just going to make an image :)
    // RTR 5.4 Antialiasing
    // read https://docs.vulkan.org/spec/latest/chapters/primsrast.html#primsrast-multisampling
    // https://docs.vulkan.org/tutorial/latest/10_Multisampling.html
    // https://docs.vulkan.org/samples/latest/samples/performance/msaa/README.html
    // https://therealmjp.github.io/posts/msaa-overview/
    // https://www.rastergrid.com/blog/gpu-tech/2021/10/multisampling-primer/
    // https://learn.microsoft.com/en-us/windows/win32/direct3d11/d3d10-graphics-programming-guide-rasterizer-stage-rules
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    // render image is on GPU and never leaves... trapped there muahaha
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    // #JOHN don't forget to come back and add whatever we need to write to from a compute shader
    imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageCreateInfo.queueFamilyIndexCount = 0;
    // queue family not needed since we are doing exclusive

    // Create the actual iamge
    VkImage offscreenImage;
    vkchk(vkCreateImage(*device, &imageCreateInfo, nullptr, &offscreenImage));

    VkImageMemoryRequirementsInfo2 memoryRequirementsInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2, .image = offscreenImage};
    VkMemoryRequirements2 memoryRequirements{
        .sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2,
    };
    vkGetImageMemoryRequirements2(*device, &memoryRequirementsInfo, &memoryRequirements);

    int i = 0;

    VkMemoryAllocateInfo memoryAllocateInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memoryRequirements.memoryRequirements.size,
        .memoryTypeIndex = j3a.FindMemoryType(memoryRequirements.memoryRequirements.memoryTypeBits,
                                              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)};
    // #JOHN for now, going to do a 1 allocation per resource horrible method. though it does give me alignment for free
    // I think it will start on 0

    // #JOHN - todo: this needs to be stored and freed on the same lifetime as the swapchain (in managed object)
    VkDeviceMemory memory;

    vkchk(vkAllocateMemory(*device, &memoryAllocateInfo, nullptr, &memory));

    // ok now we have the memory so we bind it:
    vkchk(vkBindImageMemory(*device, offscreenImage, memory, 0));

    // create the image view #JOHN guhhhh I am not tracking the teardown intermediate VkImage, VKImageview, memory
    // things I need to to have this be correct and "rebuildable". I think for now, I'll work on just making everything
    // "work" and then I'll tie it's creation / deletion into an object of some sort, and then I'll make it rebuilt
    // alongside the swapchain in some pattern I can use throughout this project. And then I'll add it into the render
    // pipeline as the thing that actually gets layout -> transfer (clear color) -> layout -> blit to swapchain image ->
    // do the rest of the swapchain stuff as normal from that point on I think once swapchain image has colors.

    // #JOHN todo: need to go back and give these offscreen values a name in debug
    VkImageSubresourceRange subresourceRange = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1,
    };
    // image view create section:
    VkImageViewCreateInfo viewCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .flags = 0,
        .image = offscreenImage,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = VK_FORMAT_R16G16B16A16_SFLOAT,
        .subresourceRange = subresourceRange,
    };

    VkImageView offscreenImageView;

    vkchk(vkCreateImageView(*device, &viewCreateInfo, nullptr, &offscreenImageView));

    return offscreenImageView;
}

void VulkanEngine::RenderAndPresentToSwapchain()
{
    // Get this frame's sync primitives and command buffer and stuff:
    PerFrameInFlightStructure &frameStruct = frameInFlightStructures->getPerFrameInFlightStructure(currentFrame);

    // wait on "this frame in flight" to be signaled
    vkWaitForFences(*device, 1, &frameStruct.queueFinishedFence, VK_TRUE, UINT64_MAX);

    uint32_t imageAcquiredIndex = 0;
    // #JOHN this may need to tell the external system of an update... so that the engine itself can rebuild whatever
    // images it needs. maybe I'll have an internal rendering resolution and not resize but instead go for "settings"
    // like other games.
    VkResult acquireResult =
        (swapchainResources->AcquireNextImage(frameStruct.imageAcquiredSemaphore, imageAcquiredIndex));

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