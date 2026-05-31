//
// Created by johnm on 3/29/2026.
//

#ifndef VULKAN_UTIL_PR_H
#define VULKAN_UTIL_PR_H

#include <fmt/printf.h>
#include <volk.h>
#include <vulkan/vk_enum_string_helper.h>

// macro so I don't have to check the VkResult every single time
inline void vkchk(VkResult result)
{
    // #JOHN is this right? i don't want to fail out badly on "warnings"
    // that should be handled
    // HONESTLY, this whole thing is like an interesting idea. Always "hard
    // failing" on any kind of error isn't a very mature way to handle this kind
    // of thing
    // #JOHN maybe add "file" "line" to this thing...
    if (result < 0)
    {
        fmt::printf("Error: %s when executing vulkan command", string_VkResult(result));
        std::abort();
    }
}

#endif // VULKAN_UTIL_PR_H
