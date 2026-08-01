//
// Created by johnm on 3/29/2026.
//

#ifndef VULKAN_UTIL_PR_H
#define VULKAN_UTIL_PR_H

#include <fmt/printf.h>
#include <volk.h>
#include <vulkan/vk_enum_string_helper.h>
#include <utility>
#include <cassert>

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

// currently assumes no queue ownership transfer
struct ImageTransitionArgs
{
    VkCommandBuffer cmd;

    VkPipelineStageFlags2 srcStageMask;
    VkAccessFlags2 srcAccessMask;
    VkPipelineStageFlags2 dstStageMask;
    VkAccessFlags2 dstAccessMask;

    VkImageLayout oldLayout;
    VkImageLayout newLayout;

    VkImage image;

    // usually assumed to be these values
    VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    uint32_t baseMipLevel = 0;
    uint32_t levelCount = 1;
    uint32_t baseArrayLayer = 0;
    uint32_t layerCount = 1;
};

void TransitionVkImageFormat(const ImageTransitionArgs &args);

// helper so I don't have to manually write the move constructor on all of these vulkan objects and things get
// nulled out properly
template <typename T>
concept VulkanHandleConstructible = requires (T t)
{
    T{VK_NULL_HANDLE};
    t = VK_NULL_HANDLE;
};

template <VulkanHandleConstructible T>
class VkHandleRef
{
public:
    // #JOHN - is there some way I could test yes this thing can be assigned to vk null handle appropriately?
    // like some sort of modern cpp type check right here?
    explicit VkHandleRef(T vulkanHandle) : handle(vulkanHandle) {}
    VkHandleRef() : handle(VK_NULL_HANDLE) {}
    ~VkHandleRef() = default;

    // we don't do copying with these vulkan things
    VkHandleRef(const VkHandleRef&) = delete;
    VkHandleRef& operator=(const VkHandleRef&) = delete;

    VkHandleRef(VkHandleRef&& other) noexcept : handle(std::exchange(other.handle, VK_NULL_HANDLE)) {};
    VkHandleRef& operator=(VkHandleRef&& other) noexcept
    {
        // assert that our handle isn't null because... you don't really want to move over these resources imo
        if (handle != VK_NULL_HANDLE)
        {
            fmt::printf("Moving handle ref over existing handle ref without clearing");
        }

        handle = std::exchange(other.handle, VK_NULL_HANDLE);
        return *this;
    }

    // accessor so it can be used like normal
    [[nodiscard]] T get() const { return handle; }
    explicit operator bool() const { return handle != VK_NULL_HANDLE; }

    void reset() { handle = VK_NULL_HANDLE; }

private:
    T handle;
};

#endif // VULKAN_UTIL_PR_H
