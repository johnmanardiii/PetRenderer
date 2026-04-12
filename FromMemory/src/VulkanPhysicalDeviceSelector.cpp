//
// Created by johnm on 4/12/2026.
//

#include "VulkanPhysicalDeviceSelector.h"

#include "VulkanUtil.h"

#include <vector>

std::optional<VkPhysicalDevice> VulkanPhysicalDeviceSelector::selectPhysicalDevice(VkInstance instance, VkSurfaceKHR surface)
{
    // get the number of devices
    uint32_t physicalDeviceCount = 0;
    vkchk(vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr));
    std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);

    vkchk(vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.data()));

    // we now have all physical devices to iterate over.
    // for each physical device, get it's properties
    VkPhysicalDeviceProperties2 physicalDeviceProperties{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};

    for (uint32_t i = 0; i < physicalDeviceCount; i++)
    {
        VkPhysicalDevice device = physicalDevices[i];
        vkGetPhysicalDeviceProperties2(device, &physicalDeviceProperties);

        // get the number of queue family properties
        uint32_t queueFamilyPropertyCount = 0;

        vkGetPhysicalDeviceQueueFamilyProperties2(device, &queueFamilyPropertyCount, nullptr);
        std::vector<VkQueueFamilyProperties2> queueFamilyProperties(queueFamilyPropertyCount);

        for (uint32_t j = 0; j < queueFamilyPropertyCount; j++)
        {
            queueFamilyProperties[j].sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2;
        }

        vkGetPhysicalDeviceQueueFamilyProperties2(device, &queueFamilyPropertyCount, queueFamilyProperties.data());

        // iterate over the queue family properties, looking for a device that has a queue family that supports
        // both graphics and presentation, then return that physical device if we have found it.
        // this is like a super simplified version of checking what our GPU can do but I just want to get to rendering
        for (uint32_t j = 0; j < queueFamilyPropertyCount; j++)
        {
            // check whether the queue family supports "presentation" first
            VkBool32 presentSupport = false;
            vkchk(vkGetPhysicalDeviceSurfaceSupportKHR(device, j, surface, &presentSupport));
            if (presentSupport)
            {
                // check whether this queue also supports graphics operations
                if (queueFamilyProperties[j].queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT)
                {
                    // this device is good enough for me:
                    fmt::printf("Found device %s with a queue that supports graphics and presentation, good enough"
                                " for me.\n", physicalDeviceProperties.properties.deviceName);
                    return device;
                }
            }
        }
    }

    return std::nullopt; // #JOHN not done yet
}