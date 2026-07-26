//
// Created by johnm on 7/26/2026.
//

#include "GpuContext.h"

#include "SDL3/SDL_vulkan.h"
#include "ScopedFunctionalQueue.h"
#include "VulkanInstanceBuilder.h"
#include "fmt/printf.h"
#include "volk.h"

std::optional<GpuContext> GpuContext::Create(const Platform &platform)
{
    std::vector<const char *> requiredExtensions = platform.GetRequiredExtensions();
    requiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    requiredExtensions.push_back(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME);
    requiredExtensions.push_back(VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME);

    VkInstance instance = VulkanInstanceBuilder()
                              .SetRequiredLayers({"VK_LAYER_KHRONOS_validation"})
                              .SetRequiredExtensions(requiredExtensions)
                              .EnableDebugMessengerForInstanceCreation()
                              .GetInstance();

    if (!instance)
    {
        fmt::printf("Failed to get vulkan instance. exiting");
        return std::nullopt;
    }

    // load instance functions
    volkLoadInstanceOnly(instance);

    ScopedFunctionalQueue deletionQueue;
    deletionQueue.Register([instance]() { vkDestroyInstance(instance, nullptr); });

    VkSurfaceKHR surface;

    if (!SDL_Vulkan_CreateSurface(platform.GetWindow(), instance, nullptr, &surface))
    {
        fmt::printf("Failed to create vulkan surface\n");
        return std::nullopt;
    }

    deletionQueue.Register([instance, surface] {
        vkDestroySurfaceKHR(instance, surface, nullptr);
    });

    // gather a valid physical device
    VulkanPhysicalDeviceSelector selector;
    std::optional<PhysicalDeviceInfo> physDeviceinfo = selector.selectPhysicalDevice(instance, surface);

    if (!physDeviceinfo)
    {
        fmt::printf("Failed to find a valid physical device.\n");
        return std::nullopt;
    }

    return GpuContext(instance, surface, *physDeviceinfo, std::move(deletionQueue));
}

GpuContext::GpuContext(VkInstance instance, VkSurfaceKHR surface, PhysicalDeviceInfo selectedPhysicalDeviceInfo,
                       ScopedFunctionalQueue &&deletionQueue)
    : instance(instance), surface(surface), selectedPhysicalDeviceInfo(selectedPhysicalDeviceInfo),
      deletionQueue(std::move(deletionQueue))
{
}