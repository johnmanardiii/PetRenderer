//
// Created by johnm on 6/21/2026.
//

#include "DescriptorAllocator.h"

#include "VulkanUtil.h"

// ABSOLUTELY horrendous duplication of code but its OK. - john
void BigMeshBufferAllocateDescriptorInfo(VkDevice device, BigMeshBufferDescriptorInformation &descriptorInfo)
{
    // pool
    VkDescriptorPoolSize poolSize = {.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .descriptorCount = 1};

    VkDescriptorPoolCreateInfo poolCreateInfo{};
    poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolCreateInfo.maxSets = 1;
    poolCreateInfo.poolSizeCount = 1;
    poolCreateInfo.pPoolSizes = &poolSize;

    vkchk(vkCreateDescriptorPool(device, &poolCreateInfo, nullptr, &descriptorInfo.descriptorPool));

    // layout
    VkDescriptorSetLayoutBinding binding{};
    binding.binding = 0;
    binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    binding.descriptorCount = 1;
    binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
    descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorSetLayoutCreateInfo.bindingCount = 1;
    descriptorSetLayoutCreateInfo.pBindings = &binding;

    vkchk(vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCreateInfo, nullptr, &descriptorInfo.descriptorSetLayout));

    // actual descriptor set
    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo{};
    descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorSetAllocateInfo.descriptorPool = descriptorInfo.descriptorPool;
    descriptorSetAllocateInfo.descriptorSetCount = 1;
    descriptorSetAllocateInfo.pSetLayouts = &descriptorInfo.descriptorSetLayout;

    vkchk(vkAllocateDescriptorSets(device, &descriptorSetAllocateInfo, &descriptorInfo.descriptorSet));
}

void BigMeshBufferDestroyDescriptorInformation(VkDevice device, BigMeshBufferDescriptorInformation &descriptorInfo)
{
    vkDestroyDescriptorPool(device, descriptorInfo.descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(device, descriptorInfo.descriptorSetLayout, nullptr);

    descriptorInfo.descriptorPool = VK_NULL_HANDLE;
    descriptorInfo.descriptorSetLayout = VK_NULL_HANDLE;
    descriptorInfo.descriptorSet = VK_NULL_HANDLE;
}

/*
 *
 *  #JOHN this whole file is super specific to a pipeline that has one write only storage image and isn't very flexible
 *  going forward but thats why we are taking one step forward at a time.
 */
static VkDescriptorPool CreateDescriptorPool(VkDevice device)
{
    // #JOHN the concept of what a descriptor pool is a little fuzzy to me in terms of these parameters for creating it.
    // #JOHN this mayyy need to be shared. I am curious how I would write this file if I needed something more general
    // in the future but for now do as straightforward as possible
    VkDescriptorPoolSize poolSize = {.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, .descriptorCount = 1};

    VkDescriptorPoolCreateInfo poolCreateInfo{};
    poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    // going to leave the flags blank for now... apparently one is needed for "bindless" stuff but i don't care about
    // that in this super simple demo where I'll probably throw most of this away

    // only going to create *exactly what I need* #JOHN this might bite me later
    poolCreateInfo.maxSets = 1;
    poolCreateInfo.poolSizeCount = 1;
    poolCreateInfo.pPoolSizes = &poolSize;

    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    vkchk(vkCreateDescriptorPool(device, &poolCreateInfo, nullptr, &descriptorPool));
    return descriptorPool;
}

VkDescriptorSetLayout CreateDescriptorSetLayout(VkDevice device)
{
    VkDescriptorSetLayoutBinding binding{};
    binding.binding = 0;
    binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    binding.descriptorCount = 1;
    binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    // Create the descriptor set layout: #JOHN we are going to have to destroy this later
    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
    descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    // #JOHN don't think I want any flags right now...
    descriptorSetLayoutCreateInfo.bindingCount = 1;
    descriptorSetLayoutCreateInfo.pBindings = &binding;

    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    vkchk(vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCreateInfo, nullptr, &descriptorSetLayout));
    return descriptorSetLayout;
}

VkDescriptorSet AllocateDescriptorSet(VkDevice device, VkDescriptorPool descriptorPool,
                                      VkDescriptorSetLayout descriptorSetLayout)
{
    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo{};
    descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorSetAllocateInfo.descriptorPool = descriptorPool;
    descriptorSetAllocateInfo.descriptorSetCount = 1;
    descriptorSetAllocateInfo.pSetLayouts = &descriptorSetLayout;

    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    vkchk(vkAllocateDescriptorSets(device, &descriptorSetAllocateInfo, &descriptorSet));
    return descriptorSet;
}

void AttachmentImageAllocateDescriptorInfo(VkDevice device, AttachmentImageDescriptorInformation &descriptorInfo)
{
    descriptorInfo.descriptorPool = CreateDescriptorPool(device);
    descriptorInfo.descriptorSetLayout = CreateDescriptorSetLayout(device);
    descriptorInfo.descriptorSet =
        AllocateDescriptorSet(device, descriptorInfo.descriptorPool, descriptorInfo.descriptorSetLayout);
}

void AttachmentImageDestroyDescriptorInformation(VkDevice device, AttachmentImageDescriptorInformation &descriptorInfo)
{
    vkDestroyDescriptorPool(device, descriptorInfo.descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(device, descriptorInfo.descriptorSetLayout, nullptr);

    descriptorInfo.descriptorPool = VK_NULL_HANDLE;
    descriptorInfo.descriptorSetLayout = VK_NULL_HANDLE;
    descriptorInfo.descriptorSet = VK_NULL_HANDLE;
}