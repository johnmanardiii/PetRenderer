//
// Created by johnm on 3/31/2026.
//

#ifndef VULKANHELPERS_H
#define VULKANHELPERS_H

#include "vulkan/vulkan.h"
#include <optional>
#include <string>
#include <vector>

class VulkanInstanceBuilder {
public:
    VulkanInstanceBuilder& RequiresDebugUtilsExtension()
    {
        bRequiresDebugUtilsExtension = true;
        return *this;
    }
    // copies em over but that's ok
    VulkanInstanceBuilder&
    SetRequiredLayers(const std::vector<std::string>& requiredLayers)
    {
        RequiredLayers = requiredLayers;
        return *this;
    }

    // sets if we want to capture debug messenges when we create this:
    VulkanInstanceBuilder& EnableDebugMessengerForInstanceCreation()
    {
        bEnableDebugMessengerForInstanceCreation = true;
        return *this;
    }

    std::optional<VkInstance> GetInstance();

private:
    std::vector<std::string> RequiredLayers;
    bool bRequiresDebugUtilsExtension = false;
    bool bEnableDebugMessengerForInstanceCreation = false;
};

#endif // VULKANHELPERS_H
