//
// Created by johnm on 7/26/2026.
//

#ifndef GPUCONTEXT_H
#define GPUCONTEXT_H

#include "Platform.h"
#include "ScopedFunctionalQueue.h"
#include "VulkanUtil.h"
#include "VulkanInit.h"
#include <optional>

class GpuContext
{
  public:
    static std::optional<GpuContext> Create(const Platform &platform);

    VkInstance instance = VK_NULL_HANDLE;
    // surface may be rebuild on some platforms but not handled right now.
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    PhysicalDeviceInfo selectedPhysicalDeviceInfo;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue graphicsPresentQueue = VK_NULL_HANDLE;

  private:
    GpuContext(VkInstance instance, VkSurfaceKHR surface, PhysicalDeviceInfo selectedPhysicalDeviceInfo,
               VkDevice device, VkQueue graphicsPresentQueue, ScopedFunctionalQueue &&deletionQueue);



    ScopedFunctionalQueue deletionQueue;
};

#endif // GPUCONTEXT_H
