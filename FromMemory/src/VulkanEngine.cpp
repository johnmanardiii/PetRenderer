//
// Created by johnm on 4/1/2026.
//
#include "VulkanEngine.h"

#include "VulkanCreateSwapchain.h"
#include "VulkanInstanceBuilder.h"
#include "VulkanLogicalDeviceCreator.h"
#include "VulkanPhysicalDeviceSelector.h"
#include "VulkanUtil.h"
#include "vulkan/vulkan_core.h"
#include <fmt/printf.h>
#include <vector>

VulkanEngine::~VulkanEngine()
{
    // clean up vulkan resources if we have them
    // #JOHN in the future, once you have the device, query present support with
    // glfwGetPhysicalDevicePresentationSupport.
    // https://www.glfw.org/docs/3.3/group__vulkan.html#gaff3823355cdd7e2f3f9f4d9ea9518d92

    // clean up image views of swapchain before destroying the swapchain:
    // lowkey the swapchain clean up should be abstracted since it will need to happen before destructor in rebuild
    // #JOHN
    if (swapchainImageViews)
    {
        for (VkImageView swapchainImageView : *swapchainImageViews)
        {
            vkDestroyImageView(*device, swapchainImageView, nullptr);
        }
    }

    if (swapchainInfo)
    {
        // #JOHN kinda interesting, like I know that *device is valid since otherwise the swapchain couldn't have
        // been created in the control flow below, but still makes me 2% nervous.
        vkDestroySwapchainKHR(*device, swapchainInfo->swapchain, nullptr);
    }

    if (device)
    {
        vkDestroyDevice(*device, nullptr);
    }

    if (instance.has_value() && surface.has_value())
    {
        vkDestroySurfaceKHR(*instance, *surface, nullptr);
    }

    // cleanup vulkan handle stuff (move this to RAII class engine) #JOHN
    if (instance)
    {
        vkDestroyInstance(*instance, nullptr);
    }

    glfwDestroyWindow(window);

    glfwTerminate(); // #JOHN honestly wrap this in RAII too since it is annoying to manage
}

bool VulkanEngine::init()
{
    // #JOHN there may need to be some loader madness that happens here (volk)
    // but it is working without it so we move on
    /* Initialize the library */
    if (!glfwInit())
    {
        fmt::printf("Failed to initialize GLFW\n");
        return false;
    }
    // check to make sure glfw vulkan is supported
    if (glfwVulkanSupported() == GLFW_API_UNAVAILABLE)
    {
        fmt::printf("Vulkan is not supported on this platform, exiting!");
        return false;
    }

    uint32_t count;
    const char **extensions = glfwGetRequiredInstanceExtensions(&count);
    // convert to vector since I don't like that double pointer stuff
    std::vector<const char *> requiredExtensions;
    for (uint32_t i = 0; i < count; i++)
    {
        requiredExtensions.push_back(extensions[i]);
    }
    requiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    requiredExtensions.push_back(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME);

    // #JOHN you need to pass the vulkan version here.
    instance = VulkanInstanceBuilder()
                   .SetRequiredLayers({"VK_LAYER_KHRONOS_validation"})
                   .SetRequiredExtensions(requiredExtensions)
                   .EnableDebugMessengerForInstanceCreation()
                   .GetInstance();

    if (!instance)
    {
        fmt::printf("Failed to get vulkan instance. exiting");
        return false;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    // #JOHN move these values out to the class itself
    int width = 640;
    int height = 480;
    window = glfwCreateWindow(640, 480, "Window Title", nullptr, nullptr);

    // Get a vulkan window surface from glfw
    VkSurfaceKHR surface_handle;
    vkchk(glfwCreateWindowSurface(*instance, window, nullptr, &surface_handle));

    // #JOHN doesn't seem like it follows the most awesome pattern but whatevs.
    // really can't fail in this way... why optional?
    surface = surface_handle;

    // gather a valid physical device
    VulkanPhysicalDeviceSelector selector;
    std::optional<PhysicalDeviceInfoWeCareAbout> physDeviceinfo = selector.selectPhysicalDevice(*instance, *surface);

    if (!physDeviceinfo)
    {
        fmt::printf("Failed to find a valid physical device.\n");
        return false;
    }

    // create a logical device from the physical one
    device = VulkanLogicalDeviceCreator::CreateLogicalDevice(*physDeviceinfo);
    if (!device)
    {
        fmt::printf("Failed to create logical device.\n");
        return false;
    }

    swapchainInfo = VulkanCreateSwapchain::CreateSwapchain(
        *device, physDeviceinfo->physicalDevice, *surface,
        {.width = static_cast<uint32_t>(width), .height = static_cast<uint32_t>(height)});

    if (!swapchainInfo)
    {
        fmt::printf("Failed to create swapchain.\n");
        return false;
    }

    fmt::print("Created swapchain successfully.\n");

    swapchainImageViews = VulkanCreateSwapchain::CreateSwapchainImageViews(*device, *swapchainInfo);

    if (!swapchainImageViews)
    {
        fmt::printf("Failed to create swapchain image views.\n");
        return false;
    }


    // #JOHN need to verify that this is right. and also I should create N of these...
    // these say something you need to figure out by reading this tomorrow; https://docs.vulkan.org/guide/latest/swapchain_semaphore_reuse.html
    VkSemaphoreCreateInfo semaphoreCreateInfo{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
    };
    for (int i = 0; i < swapchainImageViews->size(); i++)
    {
        VkSemaphore semaphore = VK_NULL_HANDLE;
        vkchk(vkCreateSemaphore(*device, &semaphoreCreateInfo, nullptr, &semaphore));
        imageAvailableSemaphores.emplace_back(semaphore);
    }

    return true;
}

void VulkanEngine::RenderAndPresentToSwapchain()
{
    // wait on the fence for this frame (per frame data thing) to say "hey I have handed this stuff off and I am ready"

    VkAcquireNextImageInfoKHR acquireInfo {
        .sType = VK_STRUCTURE_TYPE_ACQUIRE_NEXT_IMAGE_INFO_KHR,
        .pNext = nullptr,
        .swapchain = swapchainInfo->swapchain,
        // one second timeout on getting the swapchain image
        .timeout = 1000000000,

    };

    vkAcquireNextImage2KHR(*device, )
}

void VulkanEngine::draw()
{
    // the classic GLFW loop:
    while (!glfwWindowShouldClose(window))
    {
        // render image and present to swapchain
        RenderAndPresentToSwapchain();

        // #JOHN is this happening at the right rate... not sure
        glfwPollEvents();
    }
}