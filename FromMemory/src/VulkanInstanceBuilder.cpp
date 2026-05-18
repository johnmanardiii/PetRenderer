//
// Created by johnm on 3/31/2026.
//
#include "VulkanInstanceBuilder.h"
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

std::optional<VkInstance> VulkanInstanceBuilder::GetInstance()
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
        for (const std::string& layerName : RequiredLayers)
        {
            if (layerInfo.layerName == layerName)
            {
                presentDesiredLayers.push_back(layerName.c_str());
            }
        }
    }

    if (presentDesiredLayers.size() != RequiredLayers.size())
    {
        fmt::printf("Failed to retrieve present desired layers.\n");
        return std::nullopt;
    }

    // #JOHN figure out extensions that we want
    uint32_t instanceExtensionCount = 0;
    vkchk(vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, nullptr));

    std::vector<VkExtensionProperties> availableExtensions(instanceExtensionCount);
    vkchk(vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, availableExtensions.data()));

    std::vector<const char *> extensionNames = {};

    std::vector<const char *> presentDesiredExtensions;
    for (const auto &extensionInfo : availableExtensions)
    {
        for (const auto &extensionName : RequiredExtensions)
        {
            if (strcmp(extensionInfo.extensionName, extensionName) == 0)
            {
                presentDesiredExtensions.push_back(extensionName);
                fmt::print("Found present desired extension: {}\n", extensionName);
            }
        }
    }

    if (presentDesiredExtensions.size() != RequiredExtensions.size())
    {
        fmt::printf("Failed to retrieve debug extension names.\n");
        return std::nullopt;
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

    // create the vulkan instance and try to make validation layers enabled
    VkInstanceCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = pNext,
        .pApplicationInfo = &applicationInfo,
        .enabledLayerCount = static_cast<uint32_t>(presentDesiredLayers.size()),
        .ppEnabledLayerNames = presentDesiredLayers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(presentDesiredExtensions.size()),
        .ppEnabledExtensionNames = presentDesiredExtensions.data(),
    };

    VkInstance instance = VK_NULL_HANDLE;
    // #JOHN should I use a custom alloc or nah
    vkchk(vkCreateInstance(&info, NULL, &instance));
    return instance;
}