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
    // ensure that this device supports at least a 1.3 vulkan instance version (hard requirement for now that the
    // instance and device are of at least this version)
    //
    const uint32_t instanceApiVersion = volkGetInstanceVersion();
    // god bless volk I didn't like the get proc address thing
    if (VK_API_VERSION_VARIANT(instanceApiVersion) != 0
        || VK_API_VERSION_MAJOR(instanceApiVersion) != 1
        || VK_API_VERSION_MINOR(instanceApiVersion) < 3)
    {
        // we need instance version at least 1.3 guaranteed
        fmt::printf("Application requires instance version at least 1.3.\n");
        return VK_NULL_HANDLE;
    }

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
    for (VkLayerProperties &layerInfo : availableLayers)
    {
        for (const std::string &layerName : requiredLayers)
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
    for (const char *layer : presentDesiredLayers)
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
    for (const auto &extensionName : requiredExtensions)
    {
        bool bFound = false;
        for (const auto &extensionInfo : availableExtensions)
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

static bool HasSwapChainExtension(VkPhysicalDevice device)
{
    // enumerate the physical device extensions enabled (gut is telling me this should really be shared when
    // creating the logical device but whatevs
    uint32_t extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, extensions.data());

    return std::ranges::find_if(extensions, [](const VkExtensionProperties &extension) {
        return strcmp(extension.extensionName, "VK_KHR_swapchain") == 0;
    }) != extensions.end();
}

std::optional<PhysicalDeviceInfo> selectPhysicalDevice(VkInstance instance, VkSurfaceKHR surface)
{
    std::optional<PhysicalDeviceInfo> info;

    // get the number of devices
    uint32_t physicalDeviceCount = 0;
    vkchk(vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr));
    std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);

    vkchk(vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.data()));

    // we now have all physical devices to iterate over.
    // for each physical device, get it's properties
    VkPhysicalDeviceProperties2 physicalDeviceProperties{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};

    for (uint32_t physicalDeviceIndex = 0; physicalDeviceIndex < physicalDeviceCount; physicalDeviceIndex++)
    {
        VkPhysicalDevice device = physicalDevices[physicalDeviceIndex];
        vkGetPhysicalDeviceProperties2(device, &physicalDeviceProperties);

        // let's also make sure that this physical device also supports at least vulkan 1.3
        const uint32_t physDeviceApiVersion = physicalDeviceProperties.properties.apiVersion;
        if (VK_API_VERSION_VARIANT(physDeviceApiVersion) != 0
            || VK_API_VERSION_MAJOR(physDeviceApiVersion) != 1
            || VK_API_VERSION_MINOR(physDeviceApiVersion) < 3)
        {
            continue; // we require at least version 1.3
        }

        // ensure that the physical device works with the swapchain extension, if not return early
        // #JOHN this could very well be some "shared guarantee" between physical and logical and honestly
        // the entire program itself if it ends up being more than just the swapchain extension
        if (!HasSwapChainExtension(device))
        {
            continue;
        }

        // get the number of queue family properties
        uint32_t queueFamilyPropertyCount = 0;

        vkGetPhysicalDeviceQueueFamilyProperties2(device, &queueFamilyPropertyCount, nullptr);
        std::vector<VkQueueFamilyProperties2> queueFamilyProperties(queueFamilyPropertyCount);

        for (uint32_t queueFamilyPropertyIndex = 0; queueFamilyPropertyIndex < queueFamilyPropertyCount;
             queueFamilyPropertyIndex++)
        {
            queueFamilyProperties[queueFamilyPropertyIndex].sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2;
        }

        vkGetPhysicalDeviceQueueFamilyProperties2(device, &queueFamilyPropertyCount, queueFamilyProperties.data());

        // iterate over the queue family properties, looking for a device that has a queue family that supports
        // both graphics and presentation, then return that physical device if we have found it.
        // this is like a super simplified version of checking what our GPU can do but I just want to get to rendering
        for (uint32_t queueFamilyIndex = 0; queueFamilyIndex < queueFamilyPropertyCount; queueFamilyIndex++)
        {
            // check whether the queue family supports "presentation" first
            VkBool32 presentSupport = false;
            vkchk(vkGetPhysicalDeviceSurfaceSupportKHR(device, queueFamilyIndex, surface, &presentSupport));
            if (presentSupport)
            {
                // check whether this queue also supports graphics operations
                if (queueFamilyProperties[queueFamilyIndex].queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT)
                {
                    // this device is good enough for me:
                    fmt::printf("Found device %s with a queue that supports graphics and presentation, good enough"
                                " for me.\n",
                                physicalDeviceProperties.properties.deviceName);
                    info = {.physicalDevice = device, .graphicsPresentQueueFamilyIndex = queueFamilyIndex};
                    return info;
                }
            }
        }
    }

    return info;
}

// kind of hacky but for my purposes I want my project to have these
// WAIT SHOULD THESE BE CHECKED AT THE PHYSICAL DEVICE SELECTION WHAT
// #JOHN - when I get back move this to that... section what are you doing dude.
// also next time you ad this use the "13 parameters" type thing it seems way easier to check instead of chaining all this
static void AssertPhysicalDeviceHasRequestedFeatures(VkPhysicalDevice physDevice)
{
    VkPhysicalDeviceDynamicRenderingFeatures dynRenderingFeatures = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES
    };

    VkPhysicalDeviceSynchronization2Features syncFeatures = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES,
        .pNext = &dynRenderingFeatures
    };

    VkPhysicalDeviceShaderDrawParametersFeatures draw_parameters_features =
    {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETERS_FEATURES,
        .pNext = &syncFeatures
    };

    VkPhysicalDeviceFeatures2 features2;
    features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    features2.pNext = &draw_parameters_features;
    vkGetPhysicalDeviceFeatures2(physDevice, &features2);

    // when you move this to physical device selection, && these together dude
    assert(syncFeatures.synchronization2 == VK_TRUE);
    assert(dynRenderingFeatures.dynamicRendering == VK_TRUE);
    assert(draw_parameters_features.shaderDrawParameters == VK_TRUE);
}

// #JOHN technically this signature doesn't make any sense because we assert if the device fails to be created
// but whatevs.
VkDevice CreateLogicalDevice(VkPhysicalDevice physDevice, uint32_t queueFamilyIndex)
{
    // guarantee we have sync 2 or assert on this device... could probably be part of the whole "device selection thing" #JOHN, but I
    //  am pretty sure this is gonna work OK for my needs...
    AssertPhysicalDeviceHasRequestedFeatures(physDevice);

    // Ok, seems like the hard part of writing this function is getting the device create info right,
    // like querying the extensions we need ahead of time and making sure they are there and appropriately
    // created within the creation info struct.

    VkDeviceCreateInfo createInfo = {.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};

    // create the queue create info stuff (for our one queue that shall do it all)
    // (bad) hardcoded one queue

    // hardcode priorities for fun
    float queuePriority = 1.0f;

    VkDeviceQueueCreateInfo queueCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = queueFamilyIndex,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority,
    };

    createInfo.queueCreateInfoCount = 1;
    createInfo.pQueueCreateInfos = &queueCreateInfo;

    // extensions we want
    // #JOHN we hardcoded swapchain extension in the physical device selector... move that to somewhere shared if you
    // even cared. technicall guaranteed to have it but not nice.
    const std::vector<const char *> deviceExtensions = {"VK_KHR_swapchain"};
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();
    createInfo.enabledExtensionCount = deviceExtensions.size();

    VkPhysicalDeviceVulkan13Features vk13Features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .synchronization2 = VK_TRUE,
        .dynamicRendering = VK_TRUE,
    };

    // allows for timeline semaphore
    VkPhysicalDeviceVulkan12Features vk12features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
        .pNext = &vk13Features,
        .timelineSemaphore = VK_TRUE,
        .bufferDeviceAddress = VK_TRUE
    };

    VkPhysicalDeviceVulkan11Features vk_physical_device11_features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
        .pNext = &vk12features,
        .shaderDrawParameters = VK_TRUE,
    };

    createInfo.pNext = &vk_physical_device11_features;

    // physical device features? gotta query this later
    VkDevice device;
    vkchk(vkCreateDevice(physDevice, &createInfo, nullptr, &device));

    return device;
}