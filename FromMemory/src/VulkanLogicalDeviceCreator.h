//
// Created by johnm on 4/13/2026.
//

#ifndef VULKANLOGICALDEVICECREATOR_H
#define VULKANLOGICALDEVICECREATOR_H

#include "VulkanPhysicalDeviceSelector.h"
#include <volk.h>
#include <optional>

class VulkanLogicalDeviceCreator {
public:
    static std::optional<VkDevice> CreateLogicalDevice(const PhysicalDeviceInfoWeCareAbout& deviceInfo);
private:

};



#endif //VULKANLOGICALDEVICECREATOR_H
