#include "vulkan/vulkan_core.h"
#include "vulkan_util_pr.h"
#include <GLFW/glfw3.h>
#include <vector>
#include <string>

void SetupVulkan()
{
    // #JOHN break out all of this instance stuff into a class / funciton in
    // a different file where you can just pass a "release" bool.
    // query the layers available
    uint32_t layerCount = 0;
    vkchk(vkEnumerateInstanceLayerProperties(&layerCount, NULL));
    std::vector<VkLayerProperties> availableLayers(layerCount);

    vkchk(vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data()));

    // list of layers we want to include on our instance
    std::vector<std::string> desiredLayers = {
    "VK_LAYER_KHRONOS_validation"
    };

    std::vector<const char*> presentDesiredLayers;
    // this double loop is pissing me off and #JOHN I want it to be "set-like"
    // but I really shouldn't care.
    for (const auto& layerInfo : availableLayers) {
        for (const auto& layerName : desiredLayers) {
            if (layerInfo.layerName == layerName) {
                presentDesiredLayers.push_back(layerName.c_str());
            }
        }
    }

    // #JOHN figure out extensions that we want
    uint32_t instanceExtensionCount = 0;
    vkchk(vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, nullptr));

    std::vector<VkExtensionProperties> availableExtensions(instanceExtensionCount);
    vkchk(vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, availableExtensions.data()));


    bool debug_utils_available = false;
    for (const auto& extension : availableExtensions) {
        if (strcmp(extension.extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0) {
            debug_utils_available = true;
        }
    }

    std::vector<const char*> extensionNames = {};
    if (debug_utils_available) {
        extensionNames.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    VkApplicationInfo applicationInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Pet Renderer",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "Pet Renderer",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_MAKE_VERSION(1, 0, 0)
    };// #JOHN need to fill this out

    // create the vulkan instance and try to make validation layers enabled
    VkInstanceCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &applicationInfo,
        .enabledLayerCount = static_cast<uint32_t>(presentDesiredLayers.size()),
        .ppEnabledLayerNames = presentDesiredLayers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(extensionNames.size()),
        .ppEnabledExtensionNames = extensionNames.data(),
    };

    VkInstance instance = VK_NULL_HANDLE;
    // #JOHN should I use a custom alloc or nah
    vkchk(vkCreateInstance(&info, NULL, &instance));
}

int main()
{
    SetupVulkan();

    /* Initialize the library */
    if (!glfwInit()) return -1;

    /* Create a windowed mode window and its OpenGL context */
    GLFWwindow* window = glfwCreateWindow(640, 480, "Hello World", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window)) {
        /* Swap front and back buffers */
        glfwSwapBuffers(window);

        /* Poll for and process events */
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
