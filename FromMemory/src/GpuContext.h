//
// Created by johnm on 7/26/2026.
//

#ifndef GPUCONTEXT_H
#define GPUCONTEXT_H

#include "Platform.h"
#include "VulkanUtil.h"

#include <optional>

class GpuContext {
public:
    static std::optional<GpuContext> Create(const Platform& platform);
    ~GpuContext();

    GpuContext(const GpuContext& other) = delete;
    GpuContext& operator=(const GpuContext& other) = delete;

    GpuContext(GpuContext&& other) = default;
    GpuContext& operator=(GpuContext&& other) = default;

    [[nodiscard]] VkInstance Instance() const { return instance.get(); };

private:
    GpuContext(VkInstance instance);

    VkHandleRef<VkInstance> instance;
};



#endif //GPUCONTEXT_H
