//
// Created by johnm on 4/12/2026.
//

#include "VulkanPhysicalDeviceSelector.h"

#include "VulkanUtil.h"

#include <vector>

std::optional<PhysicalDeviceInfo> VulkanPhysicalDeviceSelector::selectPhysicalDevice(VkInstance instance,
                                                                                                VkSurfaceKHR surface)
{
    std::optional<PhysicalDeviceInfo> info;

    // get the number of devices
    uint32_t physicalDeviceCount = 0;
    vkchk(vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr));
    std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);

    vkchk(vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.data()));

    // we now have all physical devices to iterate over.
    // for each physical device, get it's properties
    VkPhysicalDeviceProperties2 physicalDeviceProperties{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};

    for (uint32_t physicalDeviceIndex = 0; physicalDeviceIndex < physicalDeviceCount; physicalDeviceIndex++)
    {
        VkPhysicalDevice device = physicalDevices[physicalDeviceIndex];
        vkGetPhysicalDeviceProperties2(device, &physicalDeviceProperties);

        // ensure that the physical device works with the swapchain extension, if not return early
        // #JOHN this could very well be some "shared guarantee" between physical and logical and honestly
        // the entire program itself if it ends up being more than just the swapchain extension
        if (!HasSwapChainExtension(device))
        {
            continue;
        }

        // get the number of queue family properties
        uint32_t queueFamilyPropertyCount = 0;

        vkGetPhysicalDeviceQueueFamilyProperties2(device, &queueFamilyPropertyCount, nullptr);
        std::vector<VkQueueFamilyProperties2> queueFamilyProperties(queueFamilyPropertyCount);

        for (uint32_t queueFamilyPropertyIndex = 0; queueFamilyPropertyIndex < queueFamilyPropertyCount;
             queueFamilyPropertyIndex++)
        {
            queueFamilyProperties[queueFamilyPropertyIndex].sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2;
        }

        vkGetPhysicalDeviceQueueFamilyProperties2(device, &queueFamilyPropertyCount, queueFamilyProperties.data());

        // iterate over the queue family properties, looking for a device that has a queue family that supports
        // both graphics and presentation, then return that physical device if we have found it.
        // this is like a super simplified version of checking what our GPU can do but I just want to get to rendering
        for (uint32_t queueFamilyIndex = 0; queueFamilyIndex < queueFamilyPropertyCount; queueFamilyIndex++)
        {
            // check whether the queue family supports "presentation" first
            VkBool32 presentSupport = false;
            vkchk(vkGetPhysicalDeviceSurfaceSupportKHR(device, queueFamilyIndex, surface, &presentSupport));
            if (presentSupport)
            {
                // check whether this queue also supports graphics operations
                if (queueFamilyProperties[queueFamilyIndex].queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT)
                {
                    // this device is good enough for me:
                    fmt::printf("Found device %s with a queue that supports graphics and presentation, good enough"
                                " for me.\n",
                                physicalDeviceProperties.properties.deviceName);
                    info = {.physicalDevice = device, .graphicsPresentQueueFamilyIndex = queueFamilyIndex};
                    return info;
                }
            }
        }
    }

    return info;
}
bool VulkanPhysicalDeviceSelector::HasSwapChainExtension(VkPhysicalDevice device)
{
    // enumerate the physical device extensions enabled (gut is telling me this should really be shared when
    // creating the logical device but whatevs
    uint32_t extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, extensions.data());

    return std::ranges::find_if(extensions, [](const VkExtensionProperties &extension) {
        return strcmp(extension.extensionName, "VK_KHR_swapchain") == 0;
    }) != extensions.end();

}