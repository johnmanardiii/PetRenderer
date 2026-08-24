//
// Created by johnm on 6/7/2026.
//

#include "OffscreenRenderImage.h"

#include "VulkanDebugUtil.h"
#include "VulkanUtil.h"
#include "vk_mem_alloc.h"

void DestroyOffscreenRenderImageInfo(VkDevice device, OffscreenRenderImageInfo &offscreenRenderImageInfo,
                                     VmaAllocator allocator)
{
    vkDestroyImageView(device, offscreenRenderImageInfo.offscreenImageView, nullptr);

    vmaDestroyImage(allocator, offscreenRenderImageInfo.offscreenImage, offscreenRenderImageInfo.allocation);
}

OffscreenRenderImageInfo createOffscreenRenderImageInfo(VkDevice device, VkExtent2D swapchainExtent,
                                                        VmaAllocator allocator, VkFormat offscreenImageFormat)
{
    OffscreenRenderImageInfo offscreenRenderImageInfo;

    offscreenRenderImageInfo.offscreenImageFormat = offscreenImageFormat;
    offscreenRenderImageInfo.extent = swapchainExtent;

    VkImageCreateInfo imgCreateInfo = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    imgCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imgCreateInfo.extent.width = swapchainExtent.width;
    imgCreateInfo.extent.height = swapchainExtent.height;
    imgCreateInfo.extent.depth = 1;
    imgCreateInfo.mipLevels = 1;
    imgCreateInfo.arrayLayers = 1;
    imgCreateInfo.format = offscreenImageFormat;
    imgCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imgCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imgCreateInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                          VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    imgCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imgCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imgCreateInfo.queueFamilyIndexCount = 0;

    VmaAllocationCreateInfo vmaAllocationCreateInfo{};
    vmaAllocationCreateInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
    vmaAllocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    vmaAllocationCreateInfo.priority = 1.0f;
    // NOT DOING any funny pool business yet

    vkchk(vmaCreateImage(allocator, &imgCreateInfo, &vmaAllocationCreateInfo, &offscreenRenderImageInfo.offscreenImage,
                         &offscreenRenderImageInfo.allocation, nullptr));

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


void UpdateOffscreenRenderImageDescriptorSet(VkDevice device, OffscreenRenderImageInfo &offscreenRenderImageInfo,
                                             VkDescriptorSet descriptorSet)
{
    const VkDescriptorImageInfo imageInfo = {.imageView = offscreenRenderImageInfo.offscreenImageView,
                                             .imageLayout = VK_IMAGE_LAYOUT_GENERAL};
    VkWriteDescriptorSet writeDescriptorSet = {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                               .dstSet = descriptorSet,
                                               .dstBinding = 0,
                                               .dstArrayElement = 0,
                                               .descriptorCount = 1,
                                               .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                                               .pImageInfo = &imageInfo};
    vkUpdateDescriptorSets(device, 1, &writeDescriptorSet, 0, nullptr);
}