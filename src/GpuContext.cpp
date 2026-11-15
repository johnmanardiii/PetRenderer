//
// Created by johnm on 7/26/2026.
//

#include "GpuContext.h"

#include "SDL3/SDL_vulkan.h"
#include "SDL3/SDL_log.h"
#include "ScopedFunctionalQueue.h"
#include "VulkanDebugUtil.h"
#include "VulkanInit.h"
#include "fmt/printf.h"
#include "volk.h"

static_assert(std::is_move_constructible<GpuContext>());

std::optional<GpuContext> CreateGpuContext(const Platform &platform, FunctionalQueue& deletionQueue)
{
    std::vector<const char *> requiredExtensions = GetRequiredExtensions();
    requiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    requiredExtensions.push_back(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME);
    requiredExtensions.push_back(VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME);

    VkInstance instance = CreateInstance({"VK_LAYER_KHRONOS_validation"}, requiredExtensions);

    if (!instance)
    {
        SDL_Log("Failed to get vulkan instance. exiting");
        return std::nullopt;
    }

    // load instance functions
    volkLoadInstanceOnly(instance);

    deletionQueue.Register([instance] { vkDestroyInstance(instance, nullptr); });

    VkSurfaceKHR surface;

    if (!SDL_Vulkan_CreateSurface(platform.window, instance, nullptr, &surface))
    {
        SDL_Log("Failed to create vulkan surface\n");
        return std::nullopt;
    }

    deletionQueue.Register([instance, surface] { vkDestroySurfaceKHR(instance, surface, nullptr); });

    // gather a valid physical device
    std::optional<PhysicalDeviceInfo> physDeviceinfo = selectPhysicalDevice(instance, surface);

    if (!physDeviceinfo)
    {
        SDL_Log("Failed to find a valid physical device.\n");
        return std::nullopt;
    }

    // logical device creation
    VkDevice device =
        CreateLogicalDevice(physDeviceinfo->physicalDevice, physDeviceinfo->graphicsPresentQueueFamilyIndex);
    if (!device)
    {
        SDL_Log("Failed to create logical device.\n");
        return std::nullopt;
    }

    deletionQueue.Register([device] { vkDestroyDevice(device, nullptr); });

    volkLoadDevice(device);

    // assign our queue created from the device
    VkQueue graphicsPresentQueue;
    vkGetDeviceQueue(device, physDeviceinfo->graphicsPresentQueueFamilyIndex, 0, &graphicsPresentQueue);

    // Name this one queue
    DebugSetName(device, VK_OBJECT_TYPE_QUEUE, graphicsPresentQueue,
                 "One Graphics / Present Queue I am using for everything");

    return GpuContext(instance, surface, *physDeviceinfo, device, graphicsPresentQueue);
}