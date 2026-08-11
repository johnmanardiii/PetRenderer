//
// Created by johnm on 7/25/2026.
//
#include "Platform.h"
#include "fmt/printf.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

static_assert(std::is_move_constructible<Platform>());

// #JOHN - should probably make this a 2d type.
std::optional<Platform> Platform::Create(int width, int height)
{
    ScopedFunctionalQueue deletionQueue;
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        fmt::printf("Failed to initialize GLFW\n");
        return std::nullopt;
    }

    deletionQueue.Register([]{SDL_Quit();});

    if (!SDL_Vulkan_LoadLibrary(nullptr))
    {
        fmt::printf("Failed to initialize SDL_Vulkan_LoadLibrary: %s\n", SDL_GetError());
        return std::nullopt;
    }

    deletionQueue.Register([]{SDL_Vulkan_UnloadLibrary();});

    // #JOHN - can this fail? if so crash because we need a window I think (might be a dumb idea)
    SDL_Window *window = SDL_CreateWindow("Pet Renderer", width, height, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
    if (!window)
    {
        fmt::printf("Failed to create window: %s\n", SDL_GetError());
        return std::nullopt;
    }

    deletionQueue.Register([window]{SDL_DestroyWindow(window);});

    return Platform(window, std::move(deletionQueue));
}

Platform::Platform(Platform &&other) noexcept
    : window(std::exchange(other.window, nullptr)),
      deletionQueue(std::move(other.deletionQueue))
{
}

Platform &Platform::operator=(Platform &&other) noexcept
{
    if (this != &other)
    {
        window = std::exchange(other.window, nullptr);
        deletionQueue = std::move(other.deletionQueue);
    }
    return *this;
}

Platform::Platform(SDL_Window *win, ScopedFunctionalQueue &&deletionQueue)
    : window(win), deletionQueue(std::move(deletionQueue))
{
}

std::vector<const char *> GetRequiredExtensions()
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