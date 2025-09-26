#include <stdio.h>

#include "../includes/video.h"

static SDL_Window *win = NULL;
static SDL_Renderer *ren = NULL;
static SDL_Texture *ptex = NULL;


void draw_test_checker(uint32_t* framebuffer)
{
    // 1) Remplir un damier visible (8x8 pixels)
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            int v = ((x >> 3) ^ (y >> 3)) & 1;
            framebuffer[y*W + x] = v ? 0xFFFFFFFFu : 0xFF000000u; // blanc/noir opaques
        }
    }

    draw_pixels(framebuffer);
}

int init_sdl()
{
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        SDL_Log("SDL_Init: %s", SDL_GetError());
        return 1;
    }

    win = SDL_CreateWindow("Intel8080 - SpaceInvader", W, H, 0);
    if (!win) {
        SDL_Log("CreateWindow: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    ren = SDL_CreateRenderer(win, NULL);
    if (!ren) {
        SDL_Log("CreateRenderer: %s", SDL_GetError());
        SDL_DestroyWindow(win);
        SDL_Quit();
        return 1;
    }
    SDL_SetRenderVSync(ren, 1);
    /*int running = 1;
    while (running) {
        // 1) événements
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) running = 0;
            if (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_ESCAPE) running = 0;
        }
    }*/
    return 0;
}

void SDL_Quit()
{
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
}


// W et H sont les dimensions logiques de l’image, p.ex. 224x256
// framebuffer est un tableau W*H en ARGB8888 (uint32_t par pixel)
void draw_pixels(const uint32_t* framebuffer)
{
    // Créer la texture une fois si besoin (STATIC suffit si on utilise UpdateTexture)
    if (!ptex) {
        ptex = SDL_CreateTexture(ren,
                                  SDL_PIXELFORMAT_ARGB8888,
                                  SDL_TEXTUREACCESS_STATIC, // ou STREAMING
                                  W, H);
        if (!ptex) {
            SDL_Log("CreateTexture: %s", SDL_GetError());
            return;
        }
    }

    // Met à jour toute la texture depuis le framebuffer (copie interne SDL)
    int pitch = W * 4; // octets par ligne dans framebuffer
    if (SDL_UpdateTexture(ptex, NULL, framebuffer, pitch)) {
        // Succès en SDL3: renvoie true
        SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
        SDL_RenderClear(ren);
        SDL_RenderTexture(ren, ptex, NULL, NULL); // plein écran fenêtre
        SDL_RenderPresent(ren);
    } else {
        SDL_Log("UpdateTexture: %s", SDL_GetError());
    }
}

void print_version_sdl3()
{
    printf("%d", SDL_GetVersion());
}