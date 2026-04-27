//
// Created by johnm on 4/15/2026.
//

#include "VulkanCreateSwapchain.h"

#include "VulkanUtil.h"

#include <vector>
std::optional<VkSwapchainKHR> VulkanCreateSwapchain::CreateSwapchain(VkPhysicalDevice physicalDevice,
                                                                     VkSurfaceKHR surface,
                                                                     VkExtent2D fallbackExtentSize)
{
    // Present Modes:
    // https://docs.vulkan.org/refpages/latest/refpages/source/vkGetPhysicalDeviceSurfacePresentModesKHR.html
    uint32_t presentModesCount;
    vkchk(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModesCount, nullptr));
    std::vector<VkPresentModeKHR> presentModes(presentModesCount);
    vkchk(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModesCount, presentModes.data()));

    VkPresentModeKHR desiredPresentMode = VK_PRESENT_MODE_FIFO_KHR;
    std::optional<VkPresentModeKHR> selectedPresentMode;

    for (auto &presentMode : presentModes)
    {
        if (presentMode == desiredPresentMode)
        {
            selectedPresentMode = presentMode;
        }
    }

    if (!selectedPresentMode)
    {
        fmt::printf("desired present mode not found, refusing to create swapchain.\n");
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
    uint32_t minImageCount = 3; // should specify somewhere that I have locked us into triple buffering
    if (surfaceCapabilities.surfaceCapabilities.maxImageCount != 0)
    {
        minImageCount = std::max(minImageCount, surfaceCapabilities.surfaceCapabilities.maxImageCount);
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
        // #JOHN absolutely NO idea if GLFW needs to change if this is something other than what we passed...
        // (initial GLFW surface outside actual surface bounds or somethign).
        // we have to do some funny business to figure out what the swapchain should be...
        swapChainExtent.width = std::clamp(fallbackExtentSize.width, surfaceCapabilities.surfaceCapabilities.minImageExtent.width, surfaceCapabilities.surfaceCapabilities.maxImageExtent.width);
        swapChainExtent.height = std::clamp(fallbackExtentSize.height, surfaceCapabilities.surfaceCapabilities.minImageExtent.height, surfaceCapabilities.surfaceCapabilities.maxImageExtent.height);
    }

    // #JOHN i get the feeling that it would be extremely painful to handle resizing at this stage of the project
    // so we will save that for a later date.
    VkSwapchainCreateInfoKHR swapchainInfo = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = surface,
        .minImageCount = minImageCount,
        .imageFormat = swapChainSurfaceFormat.surfaceFormat.format,
        .imageColorSpace = swapChainSurfaceFormat.surfaceFormat.colorSpace,
        .imageExtent = swapChainExtent,

    };

    return std::nullopt;
}

VkSurfaceFormat2KHR VulkanCreateSwapchain::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormat2KHR> &formats)
{
    // this is a list of formats to go for, in the order I want them. I am gonna make my application prefer srgb
    // and then pick one of these depending on which is available
    // woah b8g8r8a8 srgb is wayyy more common... it is because that is the order windows represents things so
    // potentially one less swizzle: https://vulkan.gpuinfo.org/listsurfaceformats.php?platform=windows
    // std::vector<VkSurfaceFormat
    VkSurfaceFormat2KHR desiredFormat = formats[0];
    std::vector<std::pair<VkFormat, VkColorSpaceKHR>> priorityFormats{
        {VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR},
        {VK_FORMAT_R8G8B8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR}};
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

    return formats[0];
}