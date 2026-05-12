//
// Created by johnm on 4/1/2026.
//

#ifndef VULKAN_ENGINE_H
#define VULKAN_ENGINE_H

#define GLFW_INCLUDE_VULKAN
#include "VulkanCommandBufferCreator.h"

#include <GLFW/glfw3.h> // #JOHn not sure why but this is making me think I haven't mastered the skill of managing my includes

#include "VulkanCreateSwapchain.h"

#include <optional>

class VulkanEngine
{
  public:
    VulkanEngine() = default;
    ~VulkanEngine();

    bool init();

    // I don't want to think about what would happen if you copied this thing
    VulkanEngine(const VulkanEngine &) = delete;
    VulkanEngine &operator=(const VulkanEngine &) = delete;
    VulkanEngine(VulkanEngine &&) = delete;
    VulkanEngine &operator=(VulkanEngine &&) = delete;

  private:
    // probably where the instance and resources go.
    // #JOHN reconsider the optionals at this point since like... they are required?
    std::optional<VkInstance> instance;
    std::optional<VkSurfaceKHR> surface;
    std::optional<VkDevice> device;

    // could probably group these together into a logical class #JOHN and then destroy it as one thing
    std::optional<SwapchainInfo> swapchainInfo;
    std::optional<std::vector<VkImageView>> swapchainImageViews;

    // stuff related to command buffers / commands for submission
    std::optional<VulkanCommandStructures> commandStructures;

    // #JOHN when I have more energy think through how I could make GLFW calls RAII since I don't like these raw
    // pointers
    GLFWwindow *window = nullptr;
};

#endif // VULKAN_ENGINE_H
