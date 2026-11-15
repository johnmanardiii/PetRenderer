//
// Created by johnm on 4/15/2026.
//

#include "VulkanSwapchainResources.h"

#include "VulkanDebugUtil.h"
#include "VulkanUtil.h"
#include "SDL3/SDL_log.h"
#include <array>
#include <cassert>
#include <vector>

namespace VulkanCreateSwapchain
{
void PrintSupportedUsageFlags(VkImageUsageFlags availableUsageFlags)
{
    constexpr uint32_t numBinaryDigitsInFlags = 4 * 8;
    for (int i = 0; i < numBinaryDigitsInFlags; i++)
    {
        uint32_t bitTest = 1 << (i);
        if (bitTest & availableUsageFlags)
        {
            fmt::print("{:#08X}\n", bitTest);
        }
    }

    // on my machine, this displayed:
    // VK_IMAGE_USAGE_TRANSFER_SRC_BIT - image used as source of transfer command
    //
    // VK_IMAGE_USAGE_TRANSFER_DST_BIT - image can be used as destination of a transfer command
    //
    // VK_IMAGE_USAGE_SAMPLED_BIT - image can be used to create image view suitable for occupying a VkDescriptorSet slot
    // either of type VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE or VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, and be sampled by a
    // shader.
    //
    // VK_IMAGE_USAGE_STORAGE_BIT - image can be used to create a image view suitable for occupying a descriptor set
    // slot of type storage image
    //
    // VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT - image can be used to create an image view
    // suitable for use as a color or resolve attachment by framebuffer
    //
    // VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT - image can
    // be used to create img view suitable occupying descriptorset of type input attachment (read from shader as input
    // attachment and used as an input attachment in a framebuffer.
}

static VkSurfaceFormat2KHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormat2KHR> &formats)
{
    // this is a list of formats to go for, in the order I want them. I am gonna make my application prefer srgb
    // and then pick one of these depending on which is available
    // woah b8g8r8a8 srgb is wayyy more common... it is because that is the order windows represents things so
    // potentially one less swizzle: https://vulkan.gpuinfo.org/listsurfaceformats.php?platform=windows
    std::vector<std::pair<VkFormat, VkColorSpaceKHR>> priorityFormats{
        {VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR}};
    // look for our real desired format if we can find it:
    for (const auto &priorityFormat : priorityFormats)
    {
        for (const auto &format : formats)
        {
            if (priorityFormat.first == format.surfaceFormat.format &&
                priorityFormat.second == format.surfaceFormat.colorSpace)
            {
                return format;
            }
        }
    }

    assert(false && "No Priority image format found... not sure what is going to happen here.");
    return formats[0];
}

std::optional<VkPresentModeKHR> SelectAvailablePresentMode(std::vector<VkPresentModeKHR> presentModes)
{
    // VkPresentModeKHR desiredPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
    std::array desiredPresentModes = {VK_PRESENT_MODE_MAILBOX_KHR, VK_PRESENT_MODE_FIFO_KHR};

    for (auto &requestedPresentMode : desiredPresentModes)
    {
        for (auto &presentMode : presentModes)
        {
            if (presentMode == requestedPresentMode)
            {
                return presentMode;
            }
        }
    }

    return std::nullopt;
}
// #JOHN this should probably take an old swapchain as an option here.
std::optional<SwapchainInfo> CreateSwapchain(VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface,
                                             VkExtent2D fallbackExtentSize)
{
    // Present Modes:
    // https://docs.vulkan.org/refpages/latest/refpages/source/vkGetPhysicalDeviceSurfacePresentModesKHR.html
    uint32_t presentModesCount;
    vkchk(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModesCount, nullptr));
    std::vector<VkPresentModeKHR> presentModes(presentModesCount);
    vkchk(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModesCount, presentModes.data()));

    // VkPresentModeKHR desiredPresentMode = VK_PRESENT_MODE_FIFO_KHR;
    // #JOHN - this should be controllable from something and then be able to be changed at runtime I think
    // based on what is queried to be available.
    std::optional<VkPresentModeKHR> selectedPresentMode = SelectAvailablePresentMode(presentModes);

    if (!selectedPresentMode)
    {
        SDL_Log("desired present mode not found, refusing to create swapchain.\n");
        return std::nullopt;
    }

    // query that we have FIFO and set that for now (#JOHN in the future, add an "option to select by preference"
    // when this funciton is called or object is constructed.

    // Surface Capabilities:
    // https://docs.vulkan.org/refpages/latest/refpages/source/vkGetPhysicalDeviceSurfaceCapabilities2KHR.html
    const VkPhysicalDeviceSurfaceInfo2KHR surfaceInfo = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR,
                                                         .surface = surface};

    // Surface Formats (color formats supported):
    // https://docs.vulkan.org/refpages/latest/refpages/source/vkGetPhysicalDeviceSurfaceFormats2KHR.html
    // resources I found useful when trying to pick a format:
    // https://docs.piston.rs/conrod/vulkano/swapchain/enum.ColorSpace.html
    // https://poniesandlight.co.uk/reflect/gamma/
    // after reading a whole lot about linear nonlinear i finally kinda get where this is in the process of displaying

    uint32_t surfaceFormatsCount = 0;
    vkchk(vkGetPhysicalDeviceSurfaceFormats2KHR(physicalDevice, &surfaceInfo, &surfaceFormatsCount, nullptr));
    std::vector<VkSurfaceFormat2KHR> surfaceFormats(surfaceFormatsCount,
                                                    {.sType = VK_STRUCTURE_TYPE_SURFACE_FORMAT_2_KHR});
    vkchk(vkGetPhysicalDeviceSurfaceFormats2KHR(physicalDevice, &surfaceInfo, &surfaceFormatsCount,
                                                surfaceFormats.data()));

    VkSurfaceFormat2KHR swapChainSurfaceFormat = ChooseSwapSurfaceFormat(surfaceFormats);

    // oof there is a lot to read about this struct:
    // https://docs.vulkan.org/refpages/latest/refpages/source/VkSurfaceCapabilitiesKHR.html
    VkSurfaceCapabilities2KHR surfaceCapabilities = {.sType = VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES_2_KHR};
    vkchk(vkGetPhysicalDeviceSurfaceCapabilities2KHR(physicalDevice, &surfaceInfo, &surfaceCapabilities));

    // go for more than one image than minimum if that is possible
    // there is this weird part in the API where max image count being 0 actually means no limit on image count
    uint32_t passedMinImageCount =
        std::max(static_cast<uint32_t>(3), surfaceCapabilities.surfaceCapabilities.minImageCount);
    if (surfaceCapabilities.surfaceCapabilities.maxImageCount != 0)
    {
        passedMinImageCount = std::min(passedMinImageCount, surfaceCapabilities.surfaceCapabilities.maxImageCount);
    }

    // ok figuring out this whole "choose extent for your swapchain" was a PAIN to figure out what it means.
    // apparently on windows the 0xFFFF... case for the current Extent doesn't happen, but we will have a fallback
    // in case it does. basically it is "does the OS give you a FB size to work with on the surface, or do you
    // determine it in vulkan". in some linux system or something you have to figure it out and clamp it between
    // min and max sizes.
    VkExtent2D swapChainExtent = {};
    if (surfaceCapabilities.surfaceCapabilities.currentExtent.height != 0xFFFFFFFF &&
        surfaceCapabilities.surfaceCapabilities.currentExtent.width != 0xFFFFFFFF)
    {
        swapChainExtent = surfaceCapabilities.surfaceCapabilities.currentExtent;
    }
    else
    {
        swapChainExtent.width =
            std::clamp(fallbackExtentSize.width, surfaceCapabilities.surfaceCapabilities.minImageExtent.width,
                       surfaceCapabilities.surfaceCapabilities.maxImageExtent.width);
        swapChainExtent.height =
            std::clamp(fallbackExtentSize.height, surfaceCapabilities.surfaceCapabilities.minImageExtent.height,
                       surfaceCapabilities.surfaceCapabilities.maxImageExtent.height);
    }

    const std::vector<VkImageUsageFlags> requiredUsageFlags = {
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT // shall be copied over
    };

    VkImageUsageFlags usages = 0;
    for (VkImageUsageFlags usage : requiredUsageFlags)
    {
        if ((usage & surfaceCapabilities.surfaceCapabilities.supportedUsageFlags) == 0)
        {
            // ehhh this is a strange way to present the error
            fmt::print("Usage flag : {:#08X} not found\n", usage);
            vkchk(VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR);
        }

        usages |= usage;
    }

    // if i really cared about this transform thing:
    // https://android-developers.googleblog.com/2020/02/handling-device-orientation-efficiently.html

    VkSurfaceTransformFlagsKHR supportedTransforms = surfaceCapabilities.surfaceCapabilities.supportedTransforms;
    // if we are not identity (or identity isn't supported), assert since we can't currently handle that, see readme for
    //      what to do if this becomes relevant:
    VkSurfaceTransformFlagBitsKHR surfaceTransform = surfaceCapabilities.surfaceCapabilities.currentTransform;
    if (surfaceTransform != VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR ||
        (surfaceCapabilities.surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) == 0)
    {
        SDL_Log("Engine currently only supports identity pretransform and will handle rotation later if needed");
        std::abort();
        return std::nullopt;
    }

    if (!(surfaceCapabilities.surfaceCapabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR))
    {
        fmt::print("Composite alpha opaque is not supported... what are we doing here\n");
        std::abort();
        return std::nullopt;
    }

    if (swapChainExtent.width == 0 || swapChainExtent.height == 0)
    {
        // early return here, valid but also shouldn't create the swapchain during this:
        // non-fatal error
        return std::nullopt;
    }

    // #JOHN i get the feeling that it would be extremely painful to handle resizing at this stage of the project
    // so we will save that for a later date.
    VkSwapchainCreateInfoKHR swapchainInfo = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = surface,
        .minImageCount = passedMinImageCount,
        .imageFormat = swapChainSurfaceFormat.surfaceFormat.format,
        .imageColorSpace = swapChainSurfaceFormat.surfaceFormat.colorSpace,
        .imageExtent = swapChainExtent,
        .imageArrayLayers = 1,
        .imageUsage = usages,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0, // not concurrent sharing mode
        .pQueueFamilyIndices = nullptr,
        .preTransform = surfaceTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = *selectedPresentMode,
        .clipped = VK_TRUE,
        .oldSwapchain =
            VK_NULL_HANDLE, // #JOHN if we need to re-call this (which we will), the old swapchain will go here
    };

    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    vkchk(vkCreateSwapchainKHR(device, &swapchainInfo, nullptr, &swapchain));

    // get the list of images so we don't need to get them every time:
    uint32_t swapchainImageCount = 0;
    vkchk(vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, nullptr));

    std::vector<VkImage> swapchainImages(swapchainImageCount);
    vkchk(vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, swapchainImages.data()));

    return SwapchainInfo{.swapchain = swapchain,
                         .swapchainImageFormat = swapchainInfo.imageFormat,
                         .swapchainImages = swapchainImages,
                         .swapchainExtent = swapChainExtent};
}

std::vector<VkImageView> CreateSwapchainImageViews(VkDevice device, const SwapchainInfo &swapchainInfo)
{
    size_t swapchainImageCount = swapchainInfo.swapchainImages.size();
    std::vector<VkImageView> swapchainImageViews(swapchainImageCount);

    VkComponentMapping swapchainMapping = {
        .r = VK_COMPONENT_SWIZZLE_IDENTITY,
        .g = VK_COMPONENT_SWIZZLE_IDENTITY,
        .b = VK_COMPONENT_SWIZZLE_IDENTITY,
        .a = VK_COMPONENT_SWIZZLE_IDENTITY,
    };

    // keep it simple, one image, one layer, one mip level
    VkImageSubresourceRange subresourceRange = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1,
    };

    for (uint32_t i = 0; i < swapchainImageCount; i++)
    {
        VkImageViewCreateInfo swapchainImageViewInfo = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = nullptr,
            .flags =
                0, // looked over https://docs.vulkan.org/refpages/latest/refpages/source/VkImageViewCreateFlagBits.html
                   // and didn't see anything I needed rn.
            .image = swapchainInfo.swapchainImages[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = swapchainInfo.swapchainImageFormat,
            .components = swapchainMapping,
            .subresourceRange = subresourceRange,
        };
        vkchk(vkCreateImageView(device, &swapchainImageViewInfo, nullptr, &swapchainImageViews[i]));
    }

    return swapchainImageViews;
}

} // namespace VulkanCreateSwapchain

VulkanSwapchainResources::VulkanSwapchainResources(GpuContext &gpuContext, VkExtent2D fallbackExtentSize)
    : gpuContext(gpuContext), fallbackExtentSize(fallbackExtentSize)
{
}

VulkanSwapchainResources::~VulkanSwapchainResources()
{
    ReleaseSwapchainResources();
}

bool VulkanSwapchainResources::Recreate()
{
    // going to try not to have it wait for idle that's the goal
    vkDeviceWaitIdle(gpuContext.device);
    ReleaseSwapchainResources();
    return CreateSwapchainResources();
}

// #JOHN this function is shallow and is more confusing than just straight up calling the vulkan stuff.
void VulkanSwapchainResources::PresentImage(VkQueue presentQueue, VkSemaphore waitSemaphore, uint32_t &imageIndex)
{
    VkPresentInfoKHR presentInfo = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &waitSemaphore,
        .swapchainCount = 1,
        .pSwapchains = &swapchainInfo->swapchain,
        .pImageIndices = &imageIndex,
    };
    VkResult result = vkQueuePresentKHR(presentQueue, &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
    {
        bNeedsRebuild = true;
    }
}

std::optional<AcquiredSwapchainImageInfo> VulkanSwapchainResources::AcquireNextImage(VkSemaphore imageAcquiredSemaphore,
                                                                                     uint32_t timeout)
{
    if (bNeedsRebuild)
    {
        if (!Recreate())
        {
            SDL_Log("Failed to recreate swapchain");
            std::abort();
        }
        bNeedsRebuild = false;
    }

    // if swapchain is invalid, early return w/ nullopt:
    if (!swapchainInfo)
    {
        return std::nullopt;
    }

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(gpuContext.device, swapchainInfo->swapchain, timeout, imageAcquiredSemaphore,
                                            VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        bNeedsRebuild = true;
        return std::nullopt;
    }

    AcquiredSwapchainImageInfo acquiredImageInfo = {
        .image = swapchainInfo->swapchainImages[imageIndex],
        .imageView = swapchainImageViews[imageIndex],
        .imageIndex = imageIndex,
        .extent = swapchainInfo->swapchainExtent,
        .imageReadyToPresentSemaphore = swapchainImageReadyToPresentSemaphores[imageIndex],
    };
    if (result == VK_SUBOPTIMAL_KHR)
    {
        // image acquired, but needs rebuild
        bNeedsRebuild = true;
    }
    return acquiredImageInfo;
}

void VulkanSwapchainResources::ReleaseSwapchainResources()
{
    if (!swapchainInfo || gpuContext.device == VK_NULL_HANDLE)
    {
        // this should never happen but whatever
        // #JOHN - this should probably make things not work at construction time, stored whole lifetime, no more
        // checks like this... this should be something we can depend on.
        return;
    }

    // wait for the device to be idle so we aren't destroying anything in use:
    vkDeviceWaitIdle(gpuContext.device);

    deletionQueue.Flush();
    swapchainInfo.reset();
}

bool VulkanSwapchainResources::CreateSwapchainResources()
{
    swapchainInfo =
        VulkanCreateSwapchain::CreateSwapchain(gpuContext.device, gpuContext.selectedPhysicalDeviceInfo.physicalDevice,
                                               gpuContext.surface, fallbackExtentSize);

    if (swapchainInfo)
    {
        // #JOHN - maybe this should be bundled with the swapchain info instead of here
        deletionQueue.Register(
            [this]() { vkDestroySwapchainKHR(gpuContext.device, swapchainInfo->swapchain, nullptr); });

        swapchainImageViews = VulkanCreateSwapchain::CreateSwapchainImageViews(gpuContext.device, *swapchainInfo);

        deletionQueue.Register([this]() {
            for (VkImageView swapchainImageView : swapchainImageViews)
            {
                vkDestroyImageView(gpuContext.device, swapchainImageView, nullptr);
            }
            swapchainImageViews.clear();
        });

        VkSemaphoreCreateInfo semaphoreCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        };

        for (int i = 0; i < swapchainImageViews.size(); i++)
        {
            VkSemaphore semaphore;
            vkchk(vkCreateSemaphore(gpuContext.device, &semaphoreCreateInfo, nullptr, &semaphore));

            // name the object as what it is:
            std::string semaphoreName = fmt::format("Image Available Semaphore {}", i);
            DebugSetName(gpuContext.device, VK_OBJECT_TYPE_SEMAPHORE, semaphore, semaphoreName.c_str());

            swapchainImageReadyToPresentSemaphores.emplace_back(semaphore);
        }

        deletionQueue.Register([this]() {
            for (VkSemaphore semaphore : swapchainImageReadyToPresentSemaphores)
            {
                vkDestroySemaphore(gpuContext.device, semaphore, nullptr);
            }

            swapchainImageReadyToPresentSemaphores.clear();
        });
    }
    return true;
}