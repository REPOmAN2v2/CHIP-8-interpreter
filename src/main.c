/**
 * http://en.wikipedia.org/wiki/CHIP-8
 * A CHIP-8 emulator is one of the easiest emulator to write. It was commonly
 * implemented on systems using 4096 bytes of memory. It has 16 one-byte 
 * registers from V0 to VF and 35 two-bytes opcodes.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/time.h>
#include "chip8.h"
#include "sdl.h"

static float getTime();

int main(int argc, char **argv)
{
	if (argc == 1) {
		fprintf(stderr, "Usage: chip8 game.c8\n");
		exit(EXIT_FAILURE);
	}

	initialiseSDL();	
	initialiseChip();

	// Copy the program into the memory
	loadGame(argv[1]);

	float t = 0.0f;
	float dt = 0.1f;
	float currentTime = 0.0f;
	float accumulator = 0.0f;

	while (1) {
		SDL_Event event;

		const float newTime = getTime();
		float deltaTime = newTime - currentTime;
		currentTime = newTime;

		if (deltaTime > 0.25f)
			deltaTime = 0.25f;

		accumulator += deltaTime;

		while (accumulator >= dt) {
			accumulator -= dt;
			t += dt;
		}

		// Iterate one cycle
		cycle();

		// Draw the screen if needed
		if (getDrawFlag()) {
			drawGraphics();
		}

		// If we press or release a key, store the state
		if (SDL_PollEvent(&event)) {
			setKeys(event.key.keysym.sym);
		}
	}
}

float getTime()
{
	static uint_least64_t start = 0;
	struct timeval tv;

	if (start == 0) {
		gettimeofday(&tv,NULL);
		start = 1000000 * tv.tv_sec + tv.tv_usec;
		return 0.0f;
	}
	
	uint_least64_t counter = 0;
	gettimeofday(&tv,NULL);
	counter = 1000000 * tv.tv_sec + tv.tv_usec;

	return (counter - start) / 1000000.0f;
}