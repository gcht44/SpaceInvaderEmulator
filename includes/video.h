#ifndef VIDEO__H
#define VIDEO__H

#define W 224
#define H 256

#include <../includes/SDL3/SDL.h>

void print_version_sdl3();
void draw_pixels(const uint32_t* framebuffer);
int init_sdl();
void SDL_exit();

#endif