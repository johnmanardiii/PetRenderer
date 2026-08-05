//
// Created by johnm on 6/7/2026.
//

#ifndef CREATEOFFSCREENRENDERIMAGE_H
#define CREATEOFFSCREENRENDERIMAGE_H

#include "vk_mem_alloc.h"

#include <volk.h>

/*
 *  #JOHN - in the future, replace this whole file. it is more of a learning excercise in how to allocate memory
 *  manually from vulkan... it is very inflexible and prone to errors and the API just isn't right...
 *
 *  Also for this to be 100 percent right, we would need to have the ability to maintain multiple offscreen render targets
 *  on resize since some could be in use on outdated swapchains.
 */

// #JOHN - eventually this could be abstracted into whatever image but for now its offscreen image
struct OffscreenRenderImageInfo
{
    VkExtent2D extent;
    VkImageView offscreenImageView = VK_NULL_HANDLE;
    VkImage offscreenImage = VK_NULL_HANDLE;

    VmaAllocation allocation = VK_NULL_HANDLE;
};

// #JOHN this used to be a RAII class but then I was getting annoyed at the move / copy behavior by default and realized
// i didn't need it and that if I didn't need it, I might as well use free funcitons. I can see though how if I wanted
// to construct that it could maybe be useful./
// oh woah big fan of how this is pass around by value... hole mole
OffscreenRenderImageInfo createOffscreenRenderImageInfo(VkDevice device, VkExtent2D swapchainExtent,
                                                        VmaAllocator allocator);

void DestroyOffscreenRenderImageInfo(VkDevice device, OffscreenRenderImageInfo &offscreenRenderImageInfo, VmaAllocator allocator);

void UpdateOffscreenRenderImageDescriptorSet(VkDevice device, OffscreenRenderImageInfo &offscreenRenderImageInfo,
                                             VkDescriptorSet descriptorSet);

#endif // CREATEOFFSCREENRENDERIMAGE_H
