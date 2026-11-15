#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>  // basing it off of this: https://wiki.libsdl.org/SDL3/README-main-functions
#include "VulkanEngine.h"

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv)
{
    // do any one-time startup
    VulkanEngine* engine = VulkanEngine::Create();
    if (!engine)
    {
        return SDL_APP_FAILURE;
    }

    // #JOHN - todo, remove this
    engine->init();

    *appstate = engine;
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
    // called repeatedly
    if (auto *engine = static_cast<VulkanEngine *>(appstate))
    {
        engine->Iterate();
    }
    return SDL_APP_CONTINUE;    // go to next frame
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    // called whenever an app event arrives (should probably set app state to have something to forward this stuff to)
    // #JOHN eventually should forward this to the engine directly...
    if (event->type == SDL_EVENT_QUIT)
    {
        return SDL_APP_SUCCESS;
    }
    // #JOHN - eventually need to handle minimized / restored
    if (event->type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED)
    {
        if (auto *engine = static_cast<VulkanEngine *>(appstate))
        {
            engine->OnWindowSizeChanged();
        }
    }
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    // called once before terminating app, chance to clean up. where you should also free appstate if needed
    if (VulkanEngine *engine = static_cast<VulkanEngine *>(appstate))
    {
        engine->WaitForDeviceIdle();
        delete engine;
    }
}