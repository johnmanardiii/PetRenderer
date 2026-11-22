#define SDL_MAIN_USE_CALLBACKS

#include "VulkanEngine.h"
#include <SDL3/SDL_main.h> // basing it off of this: https://wiki.libsdl.org/SDL3/README-main-functions

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv) {
    // do any one-time startup
    VulkanEngine *engine = VulkanEngine::Create();
    if (!engine) {
        return SDL_APP_FAILURE;
    }

    // #JOHN - todo, remove this
    engine->init();

    *appstate = engine;
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {
    // called repeatedly
    if (auto *engine = static_cast<VulkanEngine *>(appstate)) {
        engine->Iterate();
    }
    return SDL_APP_CONTINUE; // go to next frame
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
    // called whenever an app event arrives (should probably set app state to have something to forward this stuff to)
    // #JOHN eventually should forward this to the engine directly...
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    }
    // #JOHN - eventually need to handle minimized / restored
    if (event->type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED) {
        if (auto *engine = static_cast<VulkanEngine *>(appstate)) {
            engine->OnWindowSizeChanged();
        }
    }

    // #JOHN - should maybe just forward the SDL events to something in the "engine" and let it handle it, though
    // it is doing a lot of work already. This check feels kind of like it already belongs in "engine" or another level
    // than main
    // #JOHN - very hacky, going to open and close all joysticks that come to us here...
    // move this once you move the event stuff out of main
    if (event->type == SDL_EVENT_JOYSTICK_ADDED) {
        const SDL_JoystickID which = event->jdevice.which;
        SDL_Joystick *joystick = SDL_OpenJoystick(which);
        // some failure handling here eventually if nullptr
        if (joystick) {
            // assert(SDL_GetNumJoystickAxes(joystick) == 4);
            // #JOHN - for now assert that all joysticks are like the one on this device I am using,
            // should probably test with a controller on PC
            // #JOHN - once I have a better idea I should pass the joysticks to the system since this is assumign one with 4 axes.
        }
    } else if (event->type == SDL_EVENT_JOYSTICK_REMOVED) {
        const SDL_JoystickID which = event->jdevice.which;
        SDL_Joystick *joystick = SDL_GetJoystickFromID(which);
        if (joystick) {
            SDL_CloseJoystick(joystick);
        }
    }

    if (event->type == SDL_EVENT_JOYSTICK_AXIS_MOTION) {
        if (auto *engine = static_cast<VulkanEngine *>(appstate)) {
            engine->HandleJoyStickEvent(event);
        }
    }

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
    // called once before terminating app, chance to clean up. where you should also free appstate if needed
    if (VulkanEngine *engine = static_cast<VulkanEngine *>(appstate)) {
        engine->WaitForDeviceIdle();
        delete engine;
    }
}