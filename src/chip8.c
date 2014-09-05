#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <time.h>
#include "chip8.h"

/**
 * Constants and typedefs
 */

#define MEM 4096
#define REG 16
#define STACK_LEVELS 16
#define KEYS 16

// Replaceable with char and short for portability
// It's also much shorter (that's the real reason)
typedef uint_least8_t ul8;
typedef uint_least16_t ul16;

/**
 * Global variables
 */

bool drawFlag;

// two bytes
ul16 opcode;
ul16 I; // address register
ul16 pc; // program counter, used to iterate through instructions
ul16 stack[STACK_LEVELS];
ul16 sp; // stack pointer

// one byte
ul8 memory[MEM];
ul8 V[REG];
ul8 screen[WIDTH*HEIGHT];
ul8 delayTimer; // both timers count at 60Hz down to zero
ul8 soundTimer;
ul8 kb[KEYS]; // keyboard

/* Writing a 7:

	0xF0 1111 ****
	0x10 0001    *
	0x20 0010   *
	0x40 0100  *
	0x40 0100  *
*/

ul8 fontset[80] = {
	0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
	0x20, 0x60, 0x20, 0x20, 0x70, // 1
	0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
	0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
	0x90, 0x90, 0xF0, 0x10, 0x10, // 4
	0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
	0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
	0xF0, 0x10, 0x20, 0x40, 0x40, // 7
	0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
	0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
	0xF0, 0x90, 0xF0, 0x90, 0x90, // A
	0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
	0xF0, 0x80, 0x80, 0x80, 0xF0, // C
	0xE0, 0x90, 0x90, 0x90, 0xE0, // D
	0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
	0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

/**
 * Local helper functions
 */

static void printError(const char *format, ...);

void initialiseChip()
{
	srand(time(NULL));

	pc = 0x200; // the program begins at 0x200, the interpreter comes before that
	drawFlag = true;

	// load fontset
	for (size_t i = 0; i < 80; ++i) {
		memory[i] = fontset[i];
	}
}

void loadGame(const char *game)
{
	initialiseChip();

	FILE *file = NULL;
	file = fopen(game, "rb");

	if (file == NULL) {
		printError("Could not open the file\n");
	} else {
		fseek(file, 0, SEEK_END);
		unsigned long fileLen = ftell(file);
		fseek(file, 0, SEEK_SET);

		if ((MEM - 512) < fileLen) {
			printError("The program is too big: %u bytes\n", fileLen, 
				__FILE__ ,__LINE__);
		}

		size_t result = fread(memory + 512, sizeof(ul8), fileLen, file);
		fclose(file);

		if (result != fileLen) {
			if (ferror(file)) {
				perror("fread()");
				printError("fread() failed\n");
            } else {
            	printError("Reading error\n");
            }
		}
	}
}

void execute()
{
	// Fetch opcode: each opcode being 2 bytes long and the memory storing
	// one-byte addresses, we need to merge two successive addresses
	opcode = memory[pc] << 8 | memory[pc + 1];
	
	// Decode the opcode according to the opcode table
	switch (opcode & 0xF000) { // Read the first 4 bits		
		case 0x0000:
			switch (opcode & 0x000F) {
				case 0x0000: // 0x00E0: clears the screen
					for (size_t i = 0; i < (WIDTH * HEIGHT); ++i) {
						screen[i] = 0x0;
					}
					drawFlag = true;
					pc += 2;
				break;

				case 0x000E: // 0x00EE: Returns from subroutine
					--sp;
					pc = stack[sp];
					pc += 2;
				break;

				default:
					fprintf(stderr, "Unknown opcode [0x0000]: 0x%X\n", opcode);
			}
		break;

		case 0x1000: // 0x1NNN: jump to address NNN
			pc = opcode & 0x0FFF;
		break;

		case 0x2000: // 0x2NNN: calls subroutine at address NNN
			stack[sp] = pc; // store the current address in the stack
			++sp;
			pc = opcode & 0x0FFF; // jump to the address
		break;

		case 0x3000: // 0x3XNN: skip the next instruction if VX == NN
			if (V[(opcode & 0x0F00) >> 8] == (opcode & 0x00FF)) {
				pc += 4;
			} else {
				pc += 2;
			}
		break;

		case 0x4000: // 0x4XNN: skip the next instruction if VX != NN
			if (V[(opcode & 0x0F00) >> 8] != (opcode & 0x00FF)) {
				pc += 4;
			} else {
				pc += 2;
			}
		break;

		case 0x5000: // 0x5XY0: skip the next instruction if VX == VY
			if (V[(opcode & 0x0F00) >> 8] == V[(opcode & 0x00F0) >> 4]) {
				pc += 4;
			} else {
				pc += 2;
			}
		break;

		case 0x6000: // 0x6XNN: set VX to NN
			V[(opcode & 0x0F00) >> 8] = opcode & 0x00FF;
			pc += 2;
		break;

		case 0x7000: // 0x7XNN: add NN to VX
			V[(opcode & 0x0F00) >> 8] += opcode & 0x00FF;
			pc += 2;
		break;

		case 0x8000:
			switch (opcode & 0x000F) {
				// 0x00Y0 >> 4 == 0x000Y == Y
				// 0x0X00 >> 8 == 0x000X == X
				case 0x0000: // 0x8XY0: set VX to VY's value
					V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4];
					pc += 2;
				break;

				case 0x0001: // 0x8XY1: set VX to "VX OR VY"
					V[(opcode & 0x0F00) >> 8] |= V[(opcode & 0x00F0) >> 4];
					pc += 2;
				break;

				case 0x0002: // 0x8XY2: set VX to "VX AND VY"
					V[(opcode & 0x0F00) >> 8] &= V[(opcode & 0x00F0) >> 4];
					pc += 2;
				break;

				case 0x0003: // 0x8XY3: set VX to "VX XOR VY"
					V[(opcode & 0x0F00) >> 8] ^= V[(opcode & 0x00F0) >> 4];
					pc += 2;
				break;

				case 0x0004: // 0x8XY4: adds VY to VX
					// if (V[X] > (255 - V[Y]))
					if (V[(opcode & 0x00F0) >> 4] 
						> (0xFF - V[(opcode & 0x0F00) >> 8])) {
						V[0xF] = 1; // set VF to 1 (carry flag)
					} else {
						V[0xF] = 0;
					}

					V[(opcode & 0x0F00) >> 8] += V[(opcode & 0x00F0) >> 4];
					pc += 2;
				break;

				case 0x0005: // 0x8XY5: VX -= VY. VF set to 0 when there is a borrow
					if (V[(opcode & 0x00F0) >> 4] > V[(opcode & 0x0F00) >> 8]) {
						V[0xF] = 0;
					} else {
						V[0xF] = 1;
					}

					V[(opcode & 0x0F00) >> 8] -= V[(opcode & 0x00F0) >> 4];
					pc += 2;
				break;

				case 0x0006: // 0x8XY6: VX >>= 1. VF set to VX's last bit
					V[0xF] = V[(opcode & 0x0F00) >> 8] & 0x1;
					V[(opcode & 0x0F00) >> 8] >>= 1;
					pc += 2;
				break;

				case 0x0007: // 0x8XY7: VX = VY - VX. VF set to 0 when there's a borrow
					if (V[(opcode & 0x0F00) >> 8] > V[(opcode & 0x00F0) >> 4]) {
						V[0xF] = 0;
					} else {
						V[0xF] = 1;
					}

					V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4] - V[(opcode & 0x0F00) >> 8];
					pc += 2;
				break;

				case 0x000E: // 0x8XYE: VX <<= 1. VF set to VX's first bit
					V[0xF] = V[(opcode & 0x0F00) >> 8] >> 7;
					V[(opcode & 0x0F00) >> 8] <<= 1;
					pc += 2;
				break;

				default:
					fprintf(stderr, "Unknown opcode [0x8000]: 0x%X\n", opcode);
			}
		break;

		case 0x9000: // 0x9XY0: skip the next instruction if VX != VY
			if (V[(opcode & 0x0F00) >> 8] != V[(opcode & 0x00F0) >> 4]) {
				pc += 4;
			} else {
				pc += 2;
			}
		break;

		case 0xA000: // 0xANNN: sets I to the address NNN
			I = opcode & 0x0FFF;
			pc += 2;
		break;

		case 0xB000: // BNNN: jump to address NNN plus V0
			pc = (opcode & 0x0FFF) + V[0];
		break;

		case 0xC000: // CXNN; set VX to a random number AND NN
			V[(opcode & 0x0F00) >> 8] = (rand() % 0xFF) & (opcode & 0x00FF);
			pc += 2;
		break;

		// 	0xDXYN: draws a sprite stored in memory at location I, at 
		//	coordinates (VX,VY) with a width of 8px, a height of Npx.
		//	Uses XOR drawing
		case 0xD000: {
			ul8 x = V[(opcode & 0x0F00) >> 8];
			ul8 y = V[(opcode & 0x00F0) >> 4];
			ul16 height = opcode & 0x000F;
			ul8 pixel;

			V[0xF] = 0;
			for (ul16 yline = 0; yline < height; ++yline) {
				// fetch the pixel value from I onwards
				pixel = memory[I + yline];

				// a row is hardcoded to 8 bits wide
				for (ul8 xline = 0; xline < 8; ++xline) {
					// 0x80 >> 8 = 0, 0x80 >> 7 = 1, 0x80 >> 6 = 10 (in binary)
					// so this checks if each of the 8 bits in pixel is equal to 1
					if ((pixel & (0x80 >> xline)) != 0) {
						// check if the pixel on the display is set to 1
						if (screen[(x + xline + ((y + yline) * WIDTH))] == 1) {
							// register the collision using the VF register
							V[0xF] = 1;
						}

						// XOR the new pixel value
						screen[x + xline + ((y + yline) * WIDTH)] ^= 1;
					}
				}
			}

			drawFlag = true;
			pc += 2;
		}
		break;

		case 0xE000:
			switch (opcode & 0x00FF) {
				case 0x009E: // 0xEX9E: skip the next instruction if the key in VX is pressed
					if (kb[V[(opcode & 0x0F00) >> 8]] != 0) {
						pc += 4;
					} else {
						pc += 2;
					}
				break;

				case 0x00A1: // 0xEXA1: skip the next instruction if the key in VX ins't pressed
					if (kb[V[(opcode & 0x0F00) >> 8]] == 0) {
						pc += 4;
					} else {
						pc += 2;
					}
				break;

				default:
					fprintf(stderr, "Unknown opcode [0xE000]: 0x%X\n", opcode);
			}
		break;

		case 0xF000:
			switch (opcode & 0x00FF) {
				case 0x0007: // 0xFX07: set VX to the value of the delay timer
					V[(opcode & 0x0F00) >> 8] = delayTimer;
					pc += 2;
				break;

				case 0x000A: { // 0xFX0A: keypress is stored in VX		
					bool keypress = false;

					for (size_t i = 0; i < 16; ++i) {
						if (kb[i] != 0) {
							V[(opcode & 0x0F00) >> 8] = i;
							keypress = true;
						}
					}

					// Stay on this instruction until we receive a keypress
					if (!keypress)
						return;

					pc += 2;
				}
				break;

				case 0x0015: // 0xFX15: set the delay timer to VX
					delayTimer = V[(opcode & 0x0F00) >> 8];
					pc += 2;
				break;

				case 0x0018: // 0xFX18: set the sound timer to VX
					soundTimer = V[(opcode & 0x0F00) >> 8];
					pc += 2;
				break;

				case 0x001E: // 0xFX1E: add VX to I, set VF to 1 if there is carry
					if (I + V[(opcode & 0x0F00) >> 8] > 0xFFF) {
						V[0xF] = 1;
					} else {
						V[0xF] = 0;
					}

					I += V[(opcode & 0x0F00) >> 8];
					pc += 2;
				break;

				case 0x0029: // 0xFX29: set I to the location of the sprite in VX
					I = V[(opcode & 0x0F00) >> 8] * 0x5;
					pc += 2;
				break;

				// 	0xFX33: store the binary-coded decimal representation 
				//	of VX at the addresses I, I + 1, and I + 2
				case 0x0033:
					// hundreds
					memory[I] = V[(opcode & 0x0F00) >> 8] / 100;
					// tens
					memory[I + 1] = (V[(opcode & 0x0F00) >> 8] / 10) % 10;
					// digits
					memory[I + 2] = (V[(opcode & 0x0F00) >> 8] % 100) % 10;					
					pc += 2;
				break;

				case 0x0055: // 0xFX55: store V0 to VX in memory starting at I
					for (ul8 i = 0; i <= (ul8)((opcode & 0x0F00) >> 8); ++i) {
						memory[I + i] = V[i];	
					}

					I += ((opcode & 0x0F00) >> 8) + 1;
					pc += 2;
				break;

				case 0x0065: // 0xFX65: fill V0 to VX with values from memory starting at I
					for (ul8 i = 0; i <= (ul8)((opcode & 0x0F00) >> 8); ++i) {
						V[i] = memory[I + i];			
					}

					I += ((opcode & 0x0F00) >> 8) + 1;
					pc += 2;
				break;

				default:
					fprintf(stderr, "Unknown opcode [0xF000]: 0x%X\n", opcode);
			}
		break;

		default:
			fprintf(stderr, "Unknown opcode: 0x%X\n", opcode);
	}	
}

void updateTimers() {
	if(delayTimer > 0) {
		--delayTimer;
	}

	if(soundTimer > 0) {
		if(soundTimer == 1) {
			fprintf(stdout, "BEEP!\n");
		}
		--soundTimer;
	}
}

bool getDrawFlag()
{
	bool old = drawFlag;
	drawFlag = drawFlag ? false : true;

	return old;
}

int getPixel(int x, int y)
{
	return (screen[(y*WIDTH) + x]);
}

unsigned char * getKeyboard()
{
	return kb;
}

void printError(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	fprintf(stderr, format, args);
	va_end(args);

	exit(EXIT_FAILURE);
}