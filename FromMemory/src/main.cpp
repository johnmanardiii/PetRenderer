#include "VulkanHelpers.h"
#include "vulkan/vulkan_core.h"
#include "vulkan_util_pr.h"
#include <GLFW/glfw3.h>
#include <vector>

int main()
{
    std::optional<VkInstance> instance
            = VulkanInstanceBuilder()
                      .SetRequiredLayers({"VK_LAYER_KHRONOS_validation"})
                      .RequiresDebugUtilsExtension()
    .EnableDebugMessengerForInstanceCreation()
                      .GetInstance();

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
