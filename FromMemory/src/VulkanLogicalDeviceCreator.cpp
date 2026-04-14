//
// Created by johnm on 4/13/2026.
//

#include "VulkanLogicalDeviceCreator.h"

#include "VulkanUtil.h"

#include <vector>

// #JOHN technically this signature doesn't make any sense because we assert if the device fails to be created
// but whatevs.
std::optional<VkDevice> VulkanLogicalDeviceCreator::CreateLogicalDevice(const PhysicalDeviceInfoWeCareAbout &deviceInfo)
{
    // Ok, seems like the hard part of writing this function is getting the device create info right,
    // like querying the extensions we need ahead of time and making sure they are there and appropriately
    // created within the creation info struct.

    VkDeviceCreateInfo createInfo = {.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};

    // create the queue create info stuff (for our one queue that shall do it all)
    // (bad) hardcoded one queue

    // hardcode priorities for fun
    float queuePriority = 1.0f;

    VkDeviceQueueCreateInfo queueCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = deviceInfo.graphicsPresentQueueFamilyIndex,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority,
    };

    createInfo.queueCreateInfoCount = 1;
    createInfo.pQueueCreateInfos = &queueCreateInfo;

    // extensions we want
    // #JOHN we hardcoded swapchain extension in the physical device selector... move that to somewhere shared if you
    // even cared. technicall guaranteed to have it but not nice.
    const std::vector<const char*> deviceExtensions = {"VK_KHR_swapchain"};
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();
    createInfo.enabledExtensionCount = deviceExtensions.size();

    // physical device features? gotta query this later
    VkDevice device;
    vkchk(vkCreateDevice(deviceInfo.physicalDevice, &createInfo, nullptr, &device));

    return device;
}