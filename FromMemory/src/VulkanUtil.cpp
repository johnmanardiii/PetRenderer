//
// Created by johnm on 7/19/2026.
//
#include "VulkanUtil.h"


void TransitionFormatSimple(const ImageTransitionArgs &args)
{
    VkImageSubresourceRange subresourceRange = {
        .aspectMask = args.aspectMask,
        .baseMipLevel = args.baseMipLevel,
        .levelCount = args.levelCount,
        .baseArrayLayer = args.baseArrayLayer,
        .layerCount = args.layerCount,
    };

    VkImageMemoryBarrier2 memoryBarrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .pNext = nullptr,
        .srcStageMask = args.srcStageMask,
        .srcAccessMask = args.srcAccessMask,
        .dstStageMask = args.dstStageMask,
        .dstAccessMask = args.dstAccessMask,
        .oldLayout = args.oldLayout,
        .newLayout = args.newLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = args.image,
        .subresourceRange = subresourceRange
    };
    VkDependencyInfo dependencyInfo = {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &memoryBarrier,
    };

    vkCmdPipelineBarrier2(args.cmd, &dependencyInfo);
}