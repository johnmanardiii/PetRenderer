//
// Created by johnm on 5/30/2026.
//

#ifndef VULKANDEBUGUTIL_H
#define VULKANDEBUGUTIL_H

#include <volk.h>

// #JOHN might need some logic tying when this is compiled to when I have the extension that does this
template<typename HandleType>
void DebugSetName(VkDevice device, VkObjectType type, HandleType handle, const char* name)
{
    VkDebugUtilsObjectNameInfoEXT nameInfo{};
    nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
    nameInfo.objectType = type;
    // #JOHN hopefully this is right... hard to tell since it is just asking for a uint64
    nameInfo.objectHandle = reinterpret_cast<uint64_t>(handle);
    nameInfo.pObjectName = name;

    vkSetDebugUtilsObjectNameEXT(device, &nameInfo);
}

#endif //VULKANDEBUGUTIL_H
