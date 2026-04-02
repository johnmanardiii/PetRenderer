#include "VulkanHelpers.h"
#include "vulkan/vulkan_core.h"
#include "vulkan_util_pr.h"
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

int main()
{
    // #JOHN there may need to be some loader madness that happens here (volk)
    // but it is working without it so we move on
    /* Initialize the library */
    if (!glfwInit())
        return -1;

    // check to make sure glfw vulkan is supported
    if (glfwVulkanSupported() == GLFW_API_UNAVAILABLE)
    {
        fmt::printf("Vulkan is not supported on this platform, exiting!");
        return -1;
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

    // #JOHN you need to pass the vulkan version here.
    std::optional<VkInstance> instance = VulkanInstanceBuilder()
                                             .SetRequiredLayers({"VK_LAYER_KHRONOS_validation"})
                                             .SetRequiredExtensions(requiredExtensions)
                                             .EnableDebugMessengerForInstanceCreation()
                                             .GetInstance();

    if (!instance)
    {
        glfwTerminate(); // #JOHN honestly wrap this in RAII too since it is annoying to manage
        fmt::printf("Failed to get vulkan instance. exiting");
        return -1;
    }

    // #JOHN look at glfw sample code for a good loop
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow *window = glfwCreateWindow(640, 480, "Window Title", NULL, NULL);

    // Get a vulkan window surface from glfw
    VkSurfaceKHR surface;
    vkchk(glfwCreateWindowSurface(*instance, window, nullptr, &surface));

    // #JOHN in the future, once you have the device, query present support with
    // glfwGetPhysicalDevicePresentationSupport.
    // https://www.glfw.org/docs/3.3/group__vulkan.html#gaff3823355cdd7e2f3f9f4d9ea9518d92

    glfwTerminate(); // #JOHN honestly wrap this in RAII too since it is annoying to manage

    vkDestroySurfaceKHR(*instance, surface, NULL);   // #JOHN I would like everything to be in RAII so I don't have to handle
    // one million code paths trying to clean everything up. Also this optional is getting awkward

    // cleanup vulkan handle stuff (move this to RAII class engine) #JOHN
    if (instance)
    {
        vkDestroyInstance(*instance, nullptr);
    }

    return 0;
}
