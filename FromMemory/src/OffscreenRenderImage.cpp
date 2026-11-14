//
// Created by johnm on 6/7/2026.
//

#include "OffscreenRenderImage.h"

#include "VulkanDebugUtil.h"
#include "VulkanUtil.h"
#include "vk_mem_alloc.h"

void DestroyAttachmentImageInfo(VkDevice device, AttachmentImageInfo &offscreenRenderImageInfo,
                                     VmaAllocator allocator)
{
    vkDestroyImageView(device, offscreenRenderImageInfo.offscreenImageView, nullptr);

    vmaDestroyImage(allocator, offscreenRenderImageInfo.offscreenImage, offscreenRenderImageInfo.offscreenImageAllocation);

    vkDestroyImageView(device, offscreenRenderImageInfo.depthAttachmentImageView, nullptr);

    vmaDestroyImage(allocator, offscreenRenderImageInfo.depthAttachmentImage,
                    offscreenRenderImageInfo.depthAttachmentAllocation);
}

AttachmentImageInfo createAttachmentImageInfo(VkDevice device, VkExtent2D swapchainExtent,
                                                        VmaAllocator allocator, VkFormat offscreenImageFormat)
{
    AttachmentImageInfo offscreenRenderImageInfo;

    offscreenRenderImageInfo.offscreenImageFormat = offscreenImageFormat;
    offscreenRenderImageInfo.extent = swapchainExtent;

    VkImageCreateInfo offscreenImgCreateInfo = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    offscreenImgCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    offscreenImgCreateInfo.extent.width = swapchainExtent.width;
    offscreenImgCreateInfo.extent.height = swapchainExtent.height;
    offscreenImgCreateInfo.extent.depth = 1;
    offscreenImgCreateInfo.mipLevels = 1;
    offscreenImgCreateInfo.arrayLayers = 1;
    offscreenImgCreateInfo.format = offscreenImageFormat;
    offscreenImgCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    offscreenImgCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    offscreenImgCreateInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                          VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    offscreenImgCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    offscreenImgCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    offscreenImgCreateInfo.queueFamilyIndexCount = 0;

    VmaAllocationCreateInfo vmaOffscreenImageAllocationCreateInfo{};
    vmaOffscreenImageAllocationCreateInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
    vmaOffscreenImageAllocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    vmaOffscreenImageAllocationCreateInfo.priority = 1.0f;
    // NOT DOING any funny pool business yet

    vkchk(vmaCreateImage(allocator, &offscreenImgCreateInfo, &vmaOffscreenImageAllocationCreateInfo,
                         &offscreenRenderImageInfo.offscreenImage,
                         &offscreenRenderImageInfo.offscreenImageAllocation, nullptr));

    VkImageSubresourceRange offscreenSubresourceRange = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1,
    };
    // image view create section:
    VkImageViewCreateInfo offscreenViewCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .flags = 0,
        .image = offscreenRenderImageInfo.offscreenImage,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = VK_FORMAT_R16G16B16A16_SFLOAT,
        .subresourceRange = offscreenSubresourceRange,
    };

    vkchk(vkCreateImageView(device, &offscreenViewCreateInfo, nullptr, &offscreenRenderImageInfo.offscreenImageView));

    DebugSetName(device, VK_OBJECT_TYPE_IMAGE, offscreenRenderImageInfo.offscreenImage, "Offscreen Render Image");
    // #JOHN - you should also give the view a debug name.
    
    // Depth buffer stuff (going to make a function when all is said and done)
    VkImageCreateInfo depthImgCreateInfo = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    depthImgCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    depthImgCreateInfo.extent.width = swapchainExtent.width;
    depthImgCreateInfo.extent.height = swapchainExtent.height;
    depthImgCreateInfo.extent.depth = 1;
    depthImgCreateInfo.mipLevels = 1;
    depthImgCreateInfo.arrayLayers = 1;
    depthImgCreateInfo.format = VK_FORMAT_D32_SFLOAT;
    depthImgCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    depthImgCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthImgCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    depthImgCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    depthImgCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    depthImgCreateInfo.queueFamilyIndexCount = 0;

    VmaAllocationCreateInfo vmaDepthBufferAllocationCreateInfo{};
    vmaDepthBufferAllocationCreateInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
    vmaDepthBufferAllocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    vmaDepthBufferAllocationCreateInfo.priority = 1.0f;
    // NOT DOING any funny pool business yet

    vkchk(vmaCreateImage(allocator, &depthImgCreateInfo, &vmaDepthBufferAllocationCreateInfo,
                         &offscreenRenderImageInfo.depthAttachmentImage,
                         &offscreenRenderImageInfo.depthAttachmentAllocation, nullptr));

    // #JOHN - todo: give this and the other one a name and honestly pass it into the shared function
    VkImageSubresourceRange depthSubresourceRange = {
        .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1,
    };
    // image view create section:
    VkImageViewCreateInfo depthViewCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .flags = 0,
        .image = offscreenRenderImageInfo.depthAttachmentImage,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = VK_FORMAT_D32_SFLOAT,
        .subresourceRange = depthSubresourceRange,
    };

    vkchk(vkCreateImageView(device, &depthViewCreateInfo, nullptr, &offscreenRenderImageInfo.depthAttachmentImageView));

    DebugSetName(device, VK_OBJECT_TYPE_IMAGE, offscreenRenderImageInfo.depthAttachmentImage,
                 "Depth Buffer Attachment");

    return offscreenRenderImageInfo;
}


void UpdateAttachmentImageDescriptorSet(VkDevice device, AttachmentImageInfo &offscreenRenderImageInfo,
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

    // #JOHN - see when this gets called, I think it should be chill but maybe we need to 
}