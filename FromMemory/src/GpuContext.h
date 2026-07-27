//
// Created by johnm on 7/26/2026.
//

#ifndef GPUCONTEXT_H
#define GPUCONTEXT_H

#include "Platform.h"
#include "ScopedFunctionalQueue.h"
#include "VulkanPhysicalDeviceSelector.h"
#include "VulkanUtil.h"

#include <optional>

class GpuContext
{
  public:
    static std::optional<GpuContext> Create(const Platform &platform);

    [[nodiscard]] VkInstance Instance() const
    {
        return instance.get();
    };
    [[nodiscard]] VkSurfaceKHR Surface() const
    {
        return surface.get();
    };
    [[nodiscard]] VkPhysicalDevice PhysicalDevice() const
    {
        return selectedPhysicalDeviceInfo.physicalDevice;
    }
    [[nodiscard]] uint32_t GraphicsPresentQueue() const
    {
        return selectedPhysicalDeviceInfo.graphicsPresentQueueFamilyIndex;
    }

    [[nodiscard]] VkDevice Device() const
    {
        return device.get();
    }

  private:
    GpuContext(VkInstance instance, VkSurfaceKHR surface, PhysicalDeviceInfo selectedPhysicalDeviceInfo,
               VkDevice device, ScopedFunctionalQueue &&deletionQueue);

    VkHandleRef<VkInstance> instance;
    // surface may be rebuild on some platforms but not handled right now.
    VkHandleRef<VkSurfaceKHR> surface;
    PhysicalDeviceInfo selectedPhysicalDeviceInfo;
    VkHandleRef<VkDevice> device;

    ScopedFunctionalQueue deletionQueue;
};

#endif // GPUCONTEXT_H
