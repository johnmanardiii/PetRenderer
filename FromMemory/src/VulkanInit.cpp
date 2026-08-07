//
// Created by johnm on 8/7/2026.
//

#include "VulkanInit.h"
#include "VulkanUtil.h"
#include "fmt/printf.h"

VKAPI_ATTR VkBool32 VKAPI_CALL DebugCreateInstanceCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                           VkDebugUtilsMessageTypeFlagsEXT messageTypes,
                                                           const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                                           void *pUserData)
{
    fmt::print("DebugCreateInstanceCallback: {}\n", pCallbackData->pMessage);

    return VK_FALSE;
}


VkInstance CreateInstance(std::vector<std::string> requiredLayers,
    const std::vector<const char *> &requiredExtensions)
{
    // #JOHN break out all of this instance stuff into a class / funciton in
    // a different file where you can just pass a "release" bool.
    // query the layers available
    uint32_t layerCount = 0;
    vkchk(vkEnumerateInstanceLayerProperties(&layerCount, nullptr));
    std::vector<VkLayerProperties> availableLayers(layerCount);

    vkchk(vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data()));

    // list of layers we want to include on our instance

    std::vector<const char *> presentDesiredLayers;
    // this double loop is pissing me off and #JOHN I want it to be "set-like"
    // but I really shouldn't care because it is so small.
    for (VkLayerProperties& layerInfo : availableLayers)
    {
        for (const std::string& layerName : requiredLayers)
        {
            if (layerInfo.layerName == layerName)
            {
                presentDesiredLayers.push_back(layerName.c_str());
            }
        }
    }

    if (presentDesiredLayers.size() != requiredLayers.size())
    {
        fmt::printf("Failed to retrieve present desired layers.\n");
        return VK_NULL_HANDLE;
    }

    // #JOHN figure out extensions that we want
    uint32_t instanceExtensionCount = 0;
    vkchk(vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, nullptr));

    std::vector<VkExtensionProperties> availableExtensions(instanceExtensionCount);
    vkchk(vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, availableExtensions.data()));

    // retrieve all extensions from the found and enabled layers and add them to available extensions
    std::vector<VkExtensionProperties> layerExtensions;
    for (const char* layer : presentDesiredLayers)
    {
        uint32_t propertyCount = 0;
        vkchk(vkEnumerateInstanceExtensionProperties(layer, &propertyCount, nullptr));
        layerExtensions.resize(propertyCount);
        vkchk(vkEnumerateInstanceExtensionProperties(layer, &propertyCount, layerExtensions.data()));

        // append to total list of available extensions
        availableExtensions.insert(availableExtensions.end(), layerExtensions.begin(), layerExtensions.end());
    }

    // de-duplicate required extensions
    std::sort(requiredLayers.begin(), requiredLayers.end());
    auto ret = std::ranges::unique(requiredLayers);
    requiredLayers.erase(ret.begin(), ret.end());

    // return early if not all extensions in the required extensions are present
    for (const auto& extensionName : requiredExtensions)
    {
        bool bFound = false;
        for (const auto& extensionInfo : availableExtensions)
        {
            if (strcmp(extensionInfo.extensionName, extensionName) == 0)
            {
                bFound = true;
                break;
            }
        }

        if (!bFound)
        {
            // dependent on the above break but it should be chill
            fmt::printf("Failed to retrieve debug extension names.\n");
            return VK_NULL_HANDLE;
        }
    }

    VkApplicationInfo applicationInfo = {.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                                         .pApplicationName = "Pet Renderer",
                                         .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
                                         .pEngineName = "Pet Renderer",
                                         .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        // #JOHN you really need to pass this explicitly to the builder when we get more time
                                         .apiVersion = VK_MAKE_VERSION(1, 3, 0)}; // #JOHN need to fill this out

    const void *pNext = nullptr;

    VkDebugUtilsMessengerCreateInfoEXT debugcallbackCreateInfo = {};

    bool bEnableDebugMessengerForInstanceCreation = true;
    if (bEnableDebugMessengerForInstanceCreation)
    {
        debugcallbackCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debugcallbackCreateInfo.messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debugcallbackCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                              VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                              VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debugcallbackCreateInfo.pfnUserCallback = DebugCreateInstanceCallback;
        pNext = &debugcallbackCreateInfo;
    }

    VkValidationFeatureEnableEXT enables = {
        VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT
    };
    // enable syncrhonization validation
    VkValidationFeaturesEXT validationFeatures = {
        .sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT,
        .enabledValidationFeatureCount = 1,
        .pEnabledValidationFeatures = &enables
    };

    const bool bUseSyncValidation = true;
    if (bUseSyncValidation)
    {
        validationFeatures.pNext = pNext;
        pNext = &validationFeatures;
    }


    // create the vulkan instance and try to make validation layers enabled
    VkInstanceCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = pNext,
        .pApplicationInfo = &applicationInfo,
        .enabledLayerCount = static_cast<uint32_t>(presentDesiredLayers.size()),
        .ppEnabledLayerNames = presentDesiredLayers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size()),
        .ppEnabledExtensionNames = requiredExtensions.data(),
    };

    VkInstance instance = VK_NULL_HANDLE;
    // #JOHN should I use a custom alloc or nah
    vkchk(vkCreateInstance(&info, NULL, &instance));
    return instance;
}