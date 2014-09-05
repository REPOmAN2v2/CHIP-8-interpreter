#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "sdl.h"
#include "chip8.h"

#define SWIDTH (WIDTH * 10)
#define SHEIGHT (HEIGHT * 10) 

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;

static void printError(const char *format, ...);

void initialiseSDL()
{
	if (SDL_Init(SDL_INIT_EVENTS) != 0) {
		printError("Failed to initialise SDL (%s)\n", SDL_GetError());
	}

	if (!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1")) {
		printError("Failed to initialise linear texture rendering\n");
	}

	window = SDL_CreateWindow("CHIP-8", SDL_WINDOWPOS_UNDEFINED,
										SDL_WINDOWPOS_UNDEFINED,
										SWIDTH,
										SHEIGHT,
										SDL_WINDOW_SHOWN);

	if (window == NULL) {
		printError("Failed to create the window: (%s)\n", SDL_GetError());
	}

	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED); //| SDL_RENDERER_PRESENTVSYNC

	if (renderer == NULL) {
		printError("Failed to initialise renderer: (%s)\n", SDL_GetError());
	}

	SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
}

void drawGraphics()
{
	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
	SDL_RenderClear(renderer);
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

	for (int y = 0; y < HEIGHT; ++y) {
		for (int x = 0; x < WIDTH; ++x) {
			if (getPixel(x, y)) {
				//fprintf(stdout, "Drawing pixel (%d, %d)\n", x, y);
				//SDL_RenderDrawPoint(renderer, x, y);
				SDL_Rect pixel = {x * 10, y * 10, 10, 10};
				SDL_RenderFillRect(renderer, &pixel);
			}
		}
	}

	SDL_RenderPresent(renderer);
}

// so ugly
void setKeys(int key, bool flag)
{
	unsigned char *keyboard = getKeyboard();

	switch (key) {
		case SDLK_ESCAPE:
			exit(EXIT_SUCCESS);

		case SDLK_1:
			keyboard[0x1] = flag;
		break;

		case SDLK_2:
			keyboard[0x2] = flag;
		break;

		case SDLK_3:
			keyboard[0x3] = flag;
		break;

		case SDLK_4:
			keyboard[0xC] = flag;
		break;

		case SDLK_a:
			keyboard[0x4] = flag;
		break;

		case SDLK_z:
			keyboard[0x5] = flag;
		break;

		case SDLK_e:
			keyboard[0x6] = flag;
		break;

		case SDLK_r:
			keyboard[0xD] = flag;
		break;

		case SDLK_q:
			keyboard[0x7] = flag;
		break;

		case SDLK_s:
			keyboard[0x8] = flag;
		break;

		case SDLK_d:
			keyboard[0x9] = flag;
		break;

		case SDLK_f:
			keyboard[0xE] = flag;
		break;

		case SDLK_w:
			keyboard[0xA] = flag;
		break;

		case SDLK_x:
			keyboard[0x0] = flag;
		break;

		case SDLK_c:
			keyboard[0xB] = flag;
		break;

		case SDLK_v:
			keyboard[0xF] = flag;
		break;

		default:
			;
	}
}

void printError(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	fprintf(stderr, format, args);
	va_end(args);

	exit(EXIT_FAILURE);
}