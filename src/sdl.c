#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "sdl.h"
#include "chip8.h"

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;

static void printError(const char *format, ...);

void initialiseSDL()
{
	if (SDL_Init(SDL_INIT_EVENTS) != 0) {
		printError("Failed to initialise SDL (%s)", SDL_GetError());
	}

	if (!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1")) {
		printError("Failed to initialise linear texture rendering");
	}

	window = SDL_CreateWindow("CHIP-8", SDL_WINDOWPOS_UNDEFINED,
										SDL_WINDOWPOS_UNDEFINED,
										64,
										32,
										SDL_WINDOW_SHOWN);

	if (window == NULL) {
		printError("Failed to create the window: (%s)\n", SDL_GetError());
	}

	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

	if (renderer == NULL) {
		printError("Failed to initialise renderer: (%s)\n", SDL_GetError());
	}

	SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
}

void drawGraphics()
{
	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
	SDL_RenderClear(renderer);

	for (int y = 0; y < HEIGHT; ++y) {
		for (int x = 0; x < WIDTH; ++x) {
			if (getPixel(x, y)) {
				SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
				SDL_RenderDrawPoint(renderer, x, y);
			}
		}
	}
}

void setKeys(int key)
{
	unsigned char *keyboard = getKeyboard();

	if(key == SDLK_1)		keyboard[0x1] = 1;
	else if(key == SDLK_2)	keyboard[0x2] = 1;
	else if(key == SDLK_3)	keyboard[0x3] = 1;
	else if(key == SDLK_4)	keyboard[0xC] = 1;

	else if(key == SDLK_a)	keyboard[0x4] = 1;
	else if(key == SDLK_z)	keyboard[0x5] = 1;
	else if(key == SDLK_e)	keyboard[0x6] = 1;
	else if(key == SDLK_r)	keyboard[0xD] = 1;

	else if(key == SDLK_q)	keyboard[0x7] = 1;
	else if(key == SDLK_s)	keyboard[0x8] = 1;
	else if(key == SDLK_d)	keyboard[0x9] = 1;
	else if(key == SDLK_f)	keyboard[0xE] = 1;

	else if(key == SDLK_w)	keyboard[0xA] = 1;
	else if(key == SDLK_x)	keyboard[0x0] = 1;
	else if(key == SDLK_c)	keyboard[0xB] = 1;
	else if(key == SDLK_v)	keyboard[0xF] = 1;
}

void printError(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	fprintf(stderr, format, args);
	va_end(args);

	exit(EXIT_FAILURE);
}