#define SDL_MAIN_USE_CALLBACKS 1  /* use the callbacks instead of main() */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "Game/Game.h"
#include "Game/Profile.h"

static Game::GameInstance* game = nullptr;

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) {
    SDL_SetAppMetadata("cc-game", "0.1.2alpha", "com.karp.cc-game");

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_Window *window = nullptr;
    SDL_Renderer *renderer = nullptr;
    if (!SDL_CreateWindowAndRenderer("cc-game", 1280, 720, SDL_WINDOW_RESIZABLE, &window, &renderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    game = new Game::GameInstance(window, renderer);

    return SDL_APP_CONTINUE;
}

#include "Game/EventManager.h"

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    }
    Game::EventManager::getInstance().Push(*event);
    return SDL_APP_CONTINUE;
}

static Uint64 lastFrameTimeNs = 0;

SDL_AppResult SDL_AppIterate(void *appstate) {
    if (!game) {
        return SDL_APP_CONTINUE;
    }

    const Game::VideoSettings& vs = game->GetVideoSettings();

    Uint64 targetIntervalNs = 0;
    if (vs.vsyncEnabled) {
        targetIntervalNs = 0; // Handled by VSync blocking in renderer
    } else if (vs.enableFrameLimiter) {
        targetIntervalNs = 1'000'000'000ULL / static_cast<Uint64>(vs.maxFps);
    } else {
        targetIntervalNs = 500'000ULL; // Default cap: 2000 fps (0.5ms)
    }

    if (targetIntervalNs > 0) {
        Uint64 now = SDL_GetTicksNS();
        if (lastFrameTimeNs == 0) {
            lastFrameTimeNs = now;
        }

        if (Uint64 elapsed = now - lastFrameTimeNs; elapsed < targetIntervalNs) {
            // If remaining time is significant (> 0.7ms), sleep to yield CPU.
            // Leave a 0.4ms buffer to account for OS timer resolution limits.
            if (const Uint64 remaining = targetIntervalNs - elapsed; remaining > 700'000ULL) {
                SDL_DelayNS(remaining - 300'000ULL);
                now = SDL_GetTicksNS();
                elapsed = now - lastFrameTimeNs;
            }

            // High-precision busy wait for the final portion
            while (elapsed < targetIntervalNs) {
                now = SDL_GetTicksNS();
                elapsed = now - lastFrameTimeNs;
            }
        }
        lastFrameTimeNs = now;
    } else {
        lastFrameTimeNs = SDL_GetTicksNS();
    }

    game->Update();
    game->Render();
    Game::ProfileEndFrame();
    Game::ProfilePrintIfDue();
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
    delete game;
    game = nullptr;
    /* SDL will clean up the window/renderer for us. */
}
