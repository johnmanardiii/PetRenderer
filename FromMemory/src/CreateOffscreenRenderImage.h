//
// Created by johnm on 6/7/2026.
//

#ifndef CREATEOFFSCREENRENDERIMAGE_H
#define CREATEOFFSCREENRENDERIMAGE_H

#include "J3Alloc.h"

#include <volk.h>

// #JOHN - eventually this could be abstracted into whatever image but for now its offscreen image
struct OffscreenRenderImageInfo
{
    VkImageView offscreenImageView = VK_NULL_HANDLE;
    VkImage offscreenImage = VK_NULL_HANDLE;

    // lowkey, this memory being tied to it is how I am doing now but I'll probably want to untie it later #JOHN
    VkDeviceMemory memory = VK_NULL_HANDLE; // one memory alloc per image (bad but ok for now)

    VkDevice device;
};

OffscreenRenderImageInfo createOffscreenRenderImageInfo(VkDevice device, VkExtent2D swapchainExtent, J3Alloc *allocator);

void DestroyOffscreenRenderImageInfo(VkDevice device, OffscreenRenderImageInfo &offscreenRenderImageInfo);

#endif // CREATEOFFSCREENRENDERIMAGE_H
