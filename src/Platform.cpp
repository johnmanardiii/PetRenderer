//
// Created by johnm on 7/25/2026.
//
#include "Platform.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include "SDL3/SDL_log.h"

static_assert(std::is_move_constructible<Platform>());

// #JOHN - should probably make this a 2d type.
std::optional<Platform> CreatePlatform(int width, int height, FunctionalQueue& deletionQueue)
{
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK))
    {
        SDL_Log("Failed to initialize GLFW\n");
        return std::nullopt;
    }

    deletionQueue.Register([]{SDL_Quit();});

    if (!SDL_Vulkan_LoadLibrary(nullptr))
    {
        SDL_Log("Failed to initialize SDL_Vulkan_LoadLibrary: %s\n", SDL_GetError());
        return std::nullopt;
    }

    deletionQueue.Register([]{SDL_Vulkan_UnloadLibrary();});

    // #JOHN - can this fail? if so crash because we need a window I think (might be a dumb idea)
    SDL_Window *window = SDL_CreateWindow("Pet Renderer", width, height, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
    if (!window)
    {
        SDL_Log("Failed to create window: %s\n", SDL_GetError());
        return std::nullopt;
    }

    deletionQueue.Register([window]{SDL_DestroyWindow(window);});

    Platform platform {.window = window};
    return platform;
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