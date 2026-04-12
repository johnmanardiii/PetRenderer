#include "VulkanEngine.h"
#include "fmt/printf.h"

int main()
{
    VulkanEngine engine;

    if (!engine.init())
    {
        fmt::printf("Failed to initialize Vulkan engine\n");
        return -1;
    }

    return 0;
}
