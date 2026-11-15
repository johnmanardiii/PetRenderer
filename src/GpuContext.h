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

struct GpuContext
{
    VkInstance instance = VK_NULL_HANDLE;
    // surface may be rebuild on some platforms but not handled right now.
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    PhysicalDeviceInfo selectedPhysicalDeviceInfo;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue graphicsPresentQueue = VK_NULL_HANDLE;
};

std::optional<GpuContext> CreateGpuContext(const Platform &platform, FunctionalQueue& deletionQueue);

#endif // GPUCONTEXT_H
