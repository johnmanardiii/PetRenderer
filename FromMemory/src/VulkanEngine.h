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
#include <chrono>

class VulkanEngine
{
  public:
    VulkanEngine() = default;
    ~VulkanEngine();

    bool init();

    // da main draw loop
    void mainLoop();

    // I don't want to think about what would happen if you copied this thing
    VulkanEngine(const VulkanEngine &) = delete;
    VulkanEngine &operator=(const VulkanEngine &) = delete;
    VulkanEngine(VulkanEngine &&) = delete;
    VulkanEngine &operator=(VulkanEngine &&) = delete;

  private:
    // #JOHN this probably belongs within a different file... but for now just want to see something
    void RenderAndPresentToSwapchain(VulkanCommandStructures& structures);

    // probably where the instance and resources go.
    // #JOHN reconsider the optionals at this point since like... they are required?
    // #JOHN in the long run, the bool valid check doesn't matter since they are all valid or this whole thing crashed
    // with vkchk
    std::optional<VkInstance> instance;
    std::optional<VkSurfaceKHR> surface;
    std::optional<VkDevice> device;
    VkQueue oneQueueToRuleThemAll = VK_NULL_HANDLE; // #JOHn this is a funny name but it is long and a lot

    // could probably group these together into a logical class #JOHN and then destroy it as one thing
    std::optional<SwapchainInfo> swapchainInfo;
    std::optional<std::vector<VkImageView>> swapchainImageViews;
    std::optional<PhysicalDeviceInfoWeCareAbout> physDeviceinfo;

    // #JOHN when I have more energy think through how I could make GLFW calls RAII since I don't like these raw
    // pointers
    GLFWwindow *window = nullptr;
    uint32_t currentFrame = 0; // always incrementing, doesn't loop around w/ modulo
    float elapsedTime = 0.0f;
};

#endif // VULKAN_ENGINE_H
