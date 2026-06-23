//
// Created by johnm on 6/21/2026.
//

#ifndef DESCRIPTORALLOCATOR_H
#define DESCRIPTORALLOCATOR_H

#include <volk.h>

/*
 * The purpose of this class is just to give me an entry point into making descriptor stuff while I get started.
 * It is probably going to be very bespoke for my first shader and will need to be reworked over time to be used in a
 * more general way.
 */
struct DescriptorInformation
{
    // #JOHN name pending
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
};

void AllocateDescriptorInfo(VkDevice device, DescriptorInformation &descriptorInfo);

void DestroyDescriptorInformation(VkDevice device, DescriptorInformation &descriptorInfo);

#endif //DESCRIPTORALLOCATOR_H
