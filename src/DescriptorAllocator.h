//
// Created by johnm on 6/21/2026.
//

#ifndef DESCRIPTORALLOCATOR_H
#define DESCRIPTORALLOCATOR_H

#include <volk.h>
// #JOHN - this whole file is problematically named LOL now that I'm looking back at it.
// basically the whole thing assumes only one descriptor pool set layout

// the cycle of violence continues with this struct design...
struct BigMeshBufferDescriptorInformation
{
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
};

void BigMeshBufferAllocateDescriptorInfo(VkDevice device, BigMeshBufferDescriptorInformation &descriptorInfo);

void BigMeshBufferDestroyDescriptorInformation(VkDevice device, BigMeshBufferDescriptorInformation &descriptorInfo);

/*
 * The purpose of this class is just to give me an entry point into making descriptor stuff while I get started.
 * It is probably going to be very bespoke for my first shader and will need to be reworked over time to be used in a
 * more general way.
 */
struct AttachmentImageDescriptorInformation
{
    // #JOHN name pending
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
};

void AttachmentImageAllocateDescriptorInfo(VkDevice device, AttachmentImageDescriptorInformation &descriptorInfo);

void AttachmentImageDestroyDescriptorInformation(VkDevice device, AttachmentImageDescriptorInformation &descriptorInfo);

#endif //DESCRIPTORALLOCATOR_H
