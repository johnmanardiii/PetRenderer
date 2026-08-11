//
// Created by johnm on 7/25/2026.
//

#ifndef PLATFORM_H
#define PLATFORM_H

#include "SDL3/SDL_video.h"
#include "ScopedFunctionalQueue.h"
#include <optional>
#include <vector>



/*
 *  Handles loading volk and SDL, and then can be used to  know the surface properties when
 */

// #JOHN - wow holy moly un-classifying this made me realize how little it is doing
struct Platform
{
    SDL_Window *window = nullptr;
};

// #JOHN - platform is a vague, weird name that doesn't capture this work.
std::optional<Platform> CreatePlatform(int width, int height, FunctionalQueue& deletionQueue);

std::vector<const char *> GetRequiredExtensions();

#endif // PLATFORM_H
