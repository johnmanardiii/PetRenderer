//
// Created by johnm on 7/25/2026.
//

#ifndef PLATFORM_H
#define PLATFORM_H

#include "SDL3/SDL_video.h"
#include "ScopedFunctionalQueue.h"
#include <optional>
#include <vector>

std::vector<const char *> GetRequiredExtensions();

/*
 *  Handles loading volk and SDL, and then can be used to  know the surface properties when
 */
class Platform
{
  public:
    static std::optional<Platform> Create(int width, int height);

    // for now just delete the move / copy constructors, though we may eventually want the move
    Platform(const Platform &) = delete;
    Platform &operator=(const Platform &) = delete;

    // these would probably need some "owns sdl and should quit on destruction var
    Platform(Platform && other) noexcept;
    Platform &operator=(Platform && other) noexcept;

    SDL_Window *window = nullptr;

  private:
    Platform(SDL_Window *win, ScopedFunctionalQueue &&deletionQueue);

    ScopedFunctionalQueue deletionQueue;
};

#endif // PLATFORM_H
