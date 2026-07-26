//
// Created by johnm on 7/25/2026.
//

#ifndef PLATFORM_H
#define PLATFORM_H

#include "SDL3/SDL_video.h"
#include <optional>
#include <vector>

/*
 *  Handles loading volk and SDL, and then can be used to  know the surface properties when
 *
 *  #JOHN - this whole class and pattern not really working for this resources that are one off is making me question
 *  everything I thought I knew about OOP... need to read more code.
 */
class Platform
{
  public:
    static std::optional<Platform> Create(int width, int height);

    ~Platform();

    // for now just delete the move / copy constructors, though we may eventually want the move
    Platform(const Platform &) = delete;
    Platform &operator=(const Platform &) = delete;

    // these would probably need some "owns sdl and should quit on destruction var
    Platform(Platform && other) noexcept;
    Platform &operator=(Platform && other) noexcept;

    [[nodiscard]] std::vector<const char *> GetRequiredExtensions() const;

    // #JOHN - is this really const if it is returning a non const pointer...
    [[nodiscard]] SDL_Window *GetWindow() const
    {
        return window;
    }

  private:
    Platform(SDL_Window *win) : window(win)
    {
    }
    SDL_Window *window = nullptr;
    bool loadedVulkan = false;
    bool bOwningInstance = true;
};

#endif // PLATFORM_H
