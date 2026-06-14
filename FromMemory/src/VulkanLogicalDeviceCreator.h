//
// Created by johnm on 4/13/2026.
//

#ifndef VULKANLOGICALDEVICECREATOR_H
#define VULKANLOGICALDEVICECREATOR_H

#include "VulkanPhysicalDeviceSelector.h"
#include <volk.h>

VkDevice CreateLogicalDevice(const PhysicalDeviceInfoWeCareAbout& deviceInfo);



#endif //VULKANLOGICALDEVICECREATOR_H
