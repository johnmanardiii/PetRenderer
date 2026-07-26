//
// Created by johnm on 7/25/2026.
//
#include "fmt/printf.h"
#include "Platform.h"

// #JOHN - should probably make this a 2d type.
std::optional<Platform> Platform::Create(int width, int height)
{
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        fmt::printf("Failed to initialize GLFW\n");
        return std::nullopt;
    }

    if (!SDL_Vulkan_LoadLibrary(nullptr))
    {
        SDL_Quit();
        fmt::printf("Failed to initialize SDL_Vulkan_LoadLibrary: %s\n", SDL_GetError());
        return std::nullopt;
    }

    // #JOHN - can this fail? if so crash because we need a window I think (might be a dumb idea)
    SDL_Window *window = SDL_CreateWindow("Pet Renderer", width, height, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
    if (!window)
    {
        fmt::printf("Failed to create window: %s\n", SDL_GetError());
        SDL_Vulkan_UnloadLibrary();
        SDL_Quit();
    }
    return Platform(window);
}

Platform::~Platform()
{
    // currently tying the window the SDL lifetime, maybe undesirable but fine for now where
    // window == application
    if (bOwningInstance)
    {
        // we have guarantees that window is valid, vulkan loaded, sdl initialized from the create func
        SDL_DestroyWindow(window);

        SDL_Vulkan_UnloadLibrary();

        SDL_Quit();
    }
}

Platform::Platform(Platform && other) noexcept : window(std::exchange(other.window, nullptr)), bOwningInstance(std::exchange(other.bOwningInstance, false))
{
}

Platform &Platform::operator=(Platform && other) noexcept
{
    if (this != &other)
    {
        window = std::exchange(other.window, nullptr);
        bOwningInstance = std::exchange(other.bOwningInstance, false);
    }
    return *this;
}

std::vector<const char *> Platform::GetRequiredExtensions() const
{
    uint32_t count;

    char const *const *extensions = SDL_Vulkan_GetInstanceExtensions(&count);
    std::vector<const char *> requiredExtensions;
    for (uint32_t i = 0; i < count; i++)
    {
        requiredExtensions.push_back(extensions[i]);
    }
    return requiredExtensions;
}