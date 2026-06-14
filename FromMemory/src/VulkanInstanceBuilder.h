//
// Created by johnm on 3/31/2026.
//

#ifndef VULKANHELPERS_H
#define VULKANHELPERS_H

#include <volk.h>
#include <optional>
#include <string>
#include <vector>

class VulkanInstanceBuilder {
public:
    // copies em over but that's ok
    VulkanInstanceBuilder&
    SetRequiredLayers(const std::vector<std::string>& requiredLayers)
    {
        RequiredLayers = requiredLayers;
        return *this;
    }

    // copies em over but that's ok
    VulkanInstanceBuilder&
    SetRequiredExtensions(const std::vector<const char*>& requiredExtensions)
    {
        RequiredExtensions = requiredExtensions;
        return *this;
    }

    // sets if we want to capture debug messenges when we create this:
    VulkanInstanceBuilder& EnableDebugMessengerForInstanceCreation()
    {
        bEnableDebugMessengerForInstanceCreation = true;
        return *this;
    }

    VkInstance GetInstance();

private:
    std::vector<std::string> RequiredLayers;
    std::vector<const char*> RequiredExtensions;
    bool bEnableDebugMessengerForInstanceCreation = false;
};

#endif // VULKANHELPERS_H
