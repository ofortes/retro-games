#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include "Config.h"
#include "Game.h"



int main(int argc, char* argv[]) {
    
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("No se pudo iniciar SDL: %s", SDL_GetError());
    }

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    
    if (!SDL_CreateWindowAndRenderer(WINDOW_TITLE, GAME_W * WINDOW_SCALE, GAME_H * WINDOW_SCALE,
        SDL_WINDOW_RESIZABLE, &window, &renderer)) {
        SDL_Log("Error creando ventana: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_SetRenderVSync(renderer, 1);
    SDL_SetRenderLogicalPresentation(renderer, GAME_W, GAME_H, SDL_LOGICAL_PRESENTATION_INTEGER_SCALE);

    {
        Game game(renderer); // entre llaves: Game se destruye ANTES que el renderer

        Uint64 lastime = SDL_GetTicksNS();
        
        while (game.isRunning()) {
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                game.handleEvent(event);
            }

            Uint64 now = SDL_GetTicksNS();
            float dt = (now - lastime) / 1'000'000'000.0f;
            lastime = now;
            if (dt > MAX_DT) dt = MAX_DT;

            game.update(dt);
            game.render();
        }
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;

}