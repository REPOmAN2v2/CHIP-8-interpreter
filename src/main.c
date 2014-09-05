/**
 * http://en.wikipedia.org/wiki/CHIP-8
 * A CHIP-8 emulator is one of the easiest emulator to write. It was commonly
 * implemented on systems using 4096 bytes of memory. It has 16 one-byte 
 * registers from V0 to VF and 35 two-bytes opcodes.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "chip8.h"
#include "sdl.h"

#define FRAMERATE (100/6)		// 60Hz => 1000/60ms

const int ticksperframe = 10;	// 10 ops per frame, so 600Hz

int main(int argc, char **argv)
{
	if (argc == 1) {
		fprintf(stderr, "Usage: chip8 game.c8\n");
		exit(EXIT_FAILURE);
	}

	// Copy the program into the memory
	loadGame(argv[1]);

	initialiseSDL();
	SDL_Event event;

	int ticks = 0;
	uint_least64_t start = SDL_GetTicks();

	while (1) {
		// If we press or release a key, store the state
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_KEYDOWN) {
				setKeys(event.key.keysym.sym, 1);
			} else if (event.type == SDL_KEYUP) {
				setKeys(event.key.keysym.sym, 0);
			}		
		}

		// Iterate one cycle
		if (ticks < ticksperframe) {
			execute();
			++ticks;
		}

		if ((SDL_GetTicks() - start) >= FRAMERATE) {
			start = SDL_GetTicks();
			updateTimers();
			drawGraphics();
			ticks = 0;
		}
	}
}