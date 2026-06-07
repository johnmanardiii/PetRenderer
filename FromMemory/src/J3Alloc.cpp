//
// Created by johnm on 5/31/2026.
//

#include "J3Alloc.h"

#include "VulkanUtil.h"

#include <cassert>

void J3Alloc::Initialize(VkPhysicalDevice physDevice, VkDevice device)
{
    this->physDevice = physDevice;
    this->device = device;
    VerifyImageSupport();
    ReadInProperties();
}
void J3Alloc::VerifyImageSupport()
{
    // #JOHN this belongs somewhere, just not here.. i am realizing

    // We are going to need an 16bit float RGBA texture for this app
    //  query support. if more is needed could be data driven.
    VkFormat requiredFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
    // What we are going to need from this image
    // #JOHN would be cool if all the tools could ahead of time register what they need,
    // and then it would all be checked against the device or something
    // format feature transfer dest - maps to this: https://docs.vulkan.org/refpages/latest/refpages/source/vkCmdClearColorImage.html#:~:text=VUID%2DvkCmdClearColorImage%2Dimage%2D01993%0AThe%20format%20features%20of%20image%20must%20contain%20VK_FORMAT_FEATURE_TRANSFER_DST_BIT
    // #JOHN no idea... how i get this to do everything I need or figure out that "yep i can use 16 bit images for this kinda thing"
    // #JOHN probably going to need to also eventually check the color attachment... wish I could assert when I tried doing that to make sure it was allowed
    VkFormatFeatureFlags     requiredFormatFlags = VK_FORMAT_FEATURE_TRANSFER_DST_BIT;

    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(physDevice, requiredFormat, &formatProperties);
    const bool bImageAttachmentFormatSupportsClear = (formatProperties.optimalTilingFeatures & requiredFormatFlags) == requiredFormatFlags;
    // #JOHN need a better assert right here
    if (!bImageAttachmentFormatSupportsClear)
    {
        fmt::printf("Failed to get 16 bit float texture for rendering to with optimal tiling features needed.");
        assert(false);
    }
}
void J3Alloc::ReadInProperties()
{
    // query th ememory properties of the physical device chosen:
    VkPhysicalDeviceMemoryProperties2 memProperties{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2};
    vkGetPhysicalDeviceMemoryProperties2(physDevice, &memProperties);

    CreateMemoryHeaps(memProperties.memoryProperties);
    int i = 0;
}

void J3Alloc::CreateMemoryHeaps(const VkPhysicalDeviceMemoryProperties &properties)
{
    memoryHeaps.resize(properties.memoryHeapCount);
    for (int i = 0; i < properties.memoryHeapCount; i++)
    {
        if (properties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
        {
            memoryHeaps[i].bIsDeviceLocal = true;
        }

        memoryHeaps[i].size = properties.memoryHeaps->size;
    }

    // now iterate over all of the memory types, assigning them to the memory heaps
    for (int i = 0; i < properties.memoryTypeCount; i++)
    {
        memoryHeaps[properties.memoryTypes[i].heapIndex].memoryTypes.push_back(properties.memoryTypes[i]);
    }
}
