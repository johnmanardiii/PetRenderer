//
// Created by johnm on 4/13/2026.
//

#ifndef VULKANLOGICALDEVICECREATOR_H
#define VULKANLOGICALDEVICECREATOR_H

#include <volk.h>

// #JOHN - this is currently not doing enough work to warrant its own file and single function.
VkDevice CreateLogicalDevice(VkPhysicalDevice physDevice, uint32_t queueFamilyIndex);



#endif //VULKANLOGICALDEVICECREATOR_H
