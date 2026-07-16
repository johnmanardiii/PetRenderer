//
// Created by johnm on 6/7/2026.
//

#include "CreateOffscreenRenderImage.h"

#include "VulkanDebugUtil.h"
#include "VulkanUtil.h"

void DestroyOffscreenRenderImageInfo(VkDevice device, OffscreenRenderImageInfo &offscreenRenderImageInfo)
{
    vkDestroyImageView(device, offscreenRenderImageInfo.offscreenImageView, nullptr);

    vkDestroyImage(device, offscreenRenderImageInfo.offscreenImage, nullptr);

    vkFreeMemory(device, offscreenRenderImageInfo.memory, nullptr);
}

OffscreenRenderImageInfo createOffscreenRenderImageInfo(VkDevice device, VkExtent2D swapchainExtent, J3Alloc *allocator)
{
    OffscreenRenderImageInfo offscreenRenderImageInfo;
    offscreenRenderImageInfo.device = device;
    offscreenRenderImageInfo.extent = swapchainExtent;

    // Create the image struct based on the params
    VkImageCreateInfo imageCreateInfo = {.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    imageCreateInfo.flags = 0;
    // these... should probably be passed in... and honestly should the
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
    imageCreateInfo.extent = VkExtent3D{swapchainExtent.width, swapchainExtent.height, 1};
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
    imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageCreateInfo.queueFamilyIndexCount = 0;
    // queue family not needed since we are doing exclusive

    vkchk(vkCreateImage(device, &imageCreateInfo, nullptr, &offscreenRenderImageInfo.offscreenImage));

    VkImageMemoryRequirementsInfo2 memoryRequirementsInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2, .image = offscreenRenderImageInfo.offscreenImage};
    VkMemoryRequirements2 memoryRequirements{
        .sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2,
    };
    vkGetImageMemoryRequirements2(device, &memoryRequirementsInfo, &memoryRequirements);

    VkMemoryAllocateInfo memoryAllocateInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memoryRequirements.memoryRequirements.size,
        .memoryTypeIndex = allocator->FindMemoryType(memoryRequirements.memoryRequirements.memoryTypeBits,
                                                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)};
    // #JOHN for now, going to do a 1 allocation per resource horrible method. though it does give me alignment for free
    // I think it will start on 0

    vkchk(vkAllocateMemory(device, &memoryAllocateInfo, nullptr, &offscreenRenderImageInfo.memory));

    // ok now we have the memory so we bind it:
    vkchk(vkBindImageMemory(device, offscreenRenderImageInfo.offscreenImage, offscreenRenderImageInfo.memory, 0));

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
        .image = offscreenRenderImageInfo.offscreenImage,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = VK_FORMAT_R16G16B16A16_SFLOAT,
        .subresourceRange = subresourceRange,
    };

    vkchk(vkCreateImageView(device, &viewCreateInfo, nullptr, &offscreenRenderImageInfo.offscreenImageView));

    DebugSetName(device, VK_OBJECT_TYPE_IMAGE, offscreenRenderImageInfo.offscreenImage, "Offscreen Render Image");

    return offscreenRenderImageInfo;
}