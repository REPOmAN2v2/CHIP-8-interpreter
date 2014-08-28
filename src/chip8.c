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

typedef struct _chip8 {
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
} Chip8;

/**
 * Global variables
 */

Chip8 *chip8;
int drawFlag;

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

/* Writing a 7:

	0xF0 1111 ****
	0x10 0001    *
	0x20 0010   *
	0x40 0100  *
	0x40 0100  *
*/

/**
 * Local helper functions
 */

static void printError(const char *format, ...);

void initialiseChip()
{
	srand(time(NULL));
	chip8 = calloc(1, sizeof(Chip8));

	if (chip8 == NULL) {
		printError("Could not allocate enough memory for the chip\n");
	}

	chip8->pc = 0x200; // the program begins at 0x200, the interpreter comes before that
	drawFlag = 1;

	// load fontset
	for (size_t i = 0; i < 80; ++i) {
		chip8->memory[i] = fontset[i];
	}
}

void loadGame(const char *game)
{
	initialiseChip();

	fprintf(stdout, "%s\n", game);
	FILE *file = NULL;
	file = fopen(game, "rb");

	if (file == NULL) {
		printError("Could not open the file\n");
	} else {
		fseek(file, 0, SEEK_END);
		unsigned long fileLen = ftell(file);
		// fseek(file, 0, SEEK_SET);
		rewind(file);

		if ((MEM - 512) < fileLen) {
			printError("The program is too big: %u bytes", fileLen);
		}

		char *buffer = malloc(sizeof(char) * fileLen);
		if (buffer == NULL) {
			printError("Could not allocate file buffer\n");
		}

		size_t result = fread(buffer, 1, fileLen, file);
		if (result != fileLen) {
			printError("Reading error: %u\n", result);
		}

		for (size_t i = 0; i < fileLen; ++i) {
			chip8->memory[i + 512] = buffer[i];
		}

		fclose(file);
		free(buffer);

		/*if (fileLen > (MEM - 512)) {
			printError("The program is using too much memory\n");
		} else {
			size_t result = fread(chip8->memory + 512, fileLen, 1, file);
			fclose(file);

			if (result != fileLen) {
				printError("Reading error\n");
			}
		}*/
	}
}

void cycle()
{
	/** 
	 * Fetch opcode: each opcode being 2 bytes long and the memory storing
	 * one-byte addresses, we need to merge two successive addresses
	 */

	chip8->opcode = chip8->memory[chip8->pc] << 8 | chip8->memory[chip8->pc + 1];
	fprintf(stdout, "Executing opcode 0x%X\n", chip8->opcode);

	// Decode the opcode according to the opcode table

	switch(chip8->opcode & 0xF000) { // Read the first 4 bits
		case 0x0000:
			switch (chip8->opcode & 0x000F) {
				case 0x0000: // 0x00E0: clears the screen
					for (size_t i = 0; i < (WIDTH * HEIGHT); ++i) {
						chip8->screen[i] = 0x0;
					}
					drawFlag = 1;
					chip8->pc += 2;
				break;

				case 0x000E: // 0x000E: returns from subroutine
					--chip8->sp;
					chip8->pc = chip8->stack[chip8->sp];
					chip8->pc += 2;
				break;

				default: 
					fprintf(stderr, "Unknown opcode [0x0000]: 0x%X\n", chip8->opcode);
			}
		break;

		case 0x1000: // 0x1NNN: jump to address NNN
			chip8->pc = chip8->opcode & 0x0FFF;
		break;

		case 0x2000: // 0x2NNN: calls subroutine at address NNN
			chip8->stack[chip8->sp] = chip8->pc; // store the current address in the stack
			++chip8->sp;
			chip8->pc = chip8->opcode & 0x0FFF; // jump to the address
		break;

		case 0x3000: // 0x3XNN: skip the next instruction if VX == NN
			if (chip8->V[(chip8->opcode & 0x0F00) >> 8] == (chip8->opcode & 0x00FF)) {
				chip8->pc += 4;
			} else {
				chip8->pc += 2;
			}
		break;

		case 0x4000: // 0x4XNN: skip the next instruction if VX != NN
			if (chip8->V[(chip8->opcode & 0x0F00) >> 8] != (chip8->opcode & 0x00FF)) {
				chip8->pc += 4;
			} else {
				chip8->pc += 2;
			}
		break;

		case 0x5000: // 0x5XY0: skip the next instruction if VX == VY
			if (chip8->V[(chip8->opcode & 0x0F00) >> 8] == chip8->V[(chip8->opcode & 0x00F0) >> 4]) {
				chip8->pc += 4;
			} else {
				chip8->pc += 2;
			}
		break;

		case 0x6000: // 0x6XNN: set VX to NN
			chip8->V[(chip8->opcode & 0x0F00) >> 8] = chip8->opcode & 0x00FF;
			chip8->pc += 2;
		break;

		case 0x7000: // 0x7XNN: add NN to VX
			chip8->V[(chip8->opcode & 0x0F00) >> 8] += chip8->opcode & 0x00FF;
			chip8->pc += 2;
		break;

		case 0x8000:
			switch (chip8->opcode & 0x000F) {
				// 0x00Y0 >> 4 == 0x000Y == Y
				// 0x0X00 >> 8 == 0x000X == X
				case 0x0000: // 0x8XY0: set VX to VY's value
					chip8->V[(chip8->opcode & 0x0F00) >> 8] = chip8->V[(chip8->opcode & 0x00F0) >> 4];
					chip8->pc += 2;
				break;

				case 0x0001: // 0x8XY1: set VX to "VX OR VY"
					chip8->V[(chip8->opcode & 0x0F00) >> 8] |= chip8->V[(chip8->opcode & 0x00F0) >> 4];
					chip8->pc += 2;
				break;

				case 0x0002: // 0x8XY1: set VX to "VX AND VY"
					chip8->V[(chip8->opcode & 0x0F00) >> 8] &= chip8->V[(chip8->opcode & 0x00F0) >> 4];
					chip8->pc += 2;
				break;

				case 0x0003: // 0x8XY1: set VX to "VX XOR VY"
					chip8->V[(chip8->opcode & 0x0F00) >> 8] ^= chip8->V[(chip8->opcode & 0x00F0) >> 4];
					chip8->pc += 2;
				break;

				case 0x0004: // 0x8XY4: adds VY to VX
					// if (V[X] > (255 - V[Y]))
					if (chip8->V[chip8->opcode & 0x00F0] >> 4
						> (0xFF - chip8->V[(chip8->opcode & 0x0F00) >> 8])) {
						chip8->V[0xF] = 1; // set VF to 1 (carry flag)
					} else {
						chip8->V[0xF] = 0;
					}

					chip8->V[(chip8->opcode & 0x0F00) >> 8] += chip8->V[(chip8->opcode & 0x00F0) >> 4];
					chip8->pc += 2;
				break;

				case 0x0005: // 0x8XY5: VX -= VY. VF set to 0 when there is a borrow
					if (chip8->V[(chip8->opcode & 0x00F0) >> 4] 
						> chip8->V[(chip8->opcode & 0x0F00) >> 8]) {
						chip8->V[0xF] = 0;
					} else {
						chip8->V[0xF] = 1;
					}

					chip8->V[(chip8->opcode & 0x0F00) >> 8] -= chip8->V[(chip8->opcode & 0x00F0) >> 4];
					chip8->pc += 2;
				break;

				case 0x0006: // 0x8XY6: VX >>= 1. VF set to VX's last bit
					chip8->V[0xF] = chip8->V[(chip8->opcode & 0x0F00) >> 8] & 0x1;
					chip8->V[(chip8->opcode & 0x0F00) >> 8] >>= 1;
					chip8->pc += 2;
				break;

				case 0x0007: // 0x8XY7: VX = VY - VX. VF set to 0 when there's a borrow
					if (chip8->V[(chip8->opcode & 0x00F0) >> 4]
						> chip8->V[(chip8->opcode & 0x0F00) >> 8]) {
						chip8->V[0xF] = 0;
					} else {
						chip8->V[0xF] = 1;
					}

					chip8->V[(chip8->opcode & 0x0F00) >> 8] = chip8->V[(chip8->opcode & 0x00F0) >> 4] - chip8->V[(chip8->opcode & 0x0F00) >> 8];
					chip8->pc += 2;
				break;

				case 0x0008: // 0x8XYE: VX <<= 1. VF set to VX's first bit
					chip8->V[0xF] = chip8->V[(chip8->opcode & 0x0F00) >> 8] >> 7;
					chip8->V[(chip8->opcode & 0x0F00) >> 8] <<= 1;
					chip8->pc += 2;
				break;

				default: 
					fprintf(stderr, "Unknown opcode [0x8000]: 0x%X\n", chip8->opcode);
			}
		break;

		case 0x9000: // 0x9XY0: skip the next instruction if VX != VY
			if (chip8->V[(chip8->opcode & 0x0F00) >> 8] != chip8->V[(chip8->opcode & 0x00F0) >> 4]) {
				chip8->pc += 4;
			} else {
				chip8->pc += 2;
			}
		break;


		case 0xA000: // 0xANNN: sets I to the address NNN
			chip8->I = chip8->opcode & 0x0FFF;
			chip8->pc += 2;
		break;

		case 0xB000: // BNNN: jump to address NNN plus V0
			chip8->pc = (chip8->opcode & 0x0FFF) + chip8->V[0];
		break;

		case 0xC000: // CXNN; set VX to a random number AND NN
			chip8->V[(chip8->opcode & 0x0F00) >> 8] = (rand() % 0xFF) & (chip8->opcode & 0x00FF);
			chip8->pc += 2;
		break;

		/* 	0xDXYN: draws a sprite stored in memory at location I, at 
			coordinates (VX,VY) with a width of 8px, a height of Npx.
			Uses XOR drawing */
		case 0xD000: {
			ul8 x = chip8->V[(chip8->opcode & 0x0F00) >> 8];
			ul8 y = chip8->V[(chip8->opcode & 0x00F0) >> 4];
			ul8 height = chip8->opcode & 0x000F;
			ul8 pixel;

			chip8->V[0xF] = 0;

			for (ul8 yline = 0; yline < height; ++yline) {
				// fetch the pixel value from I onwards
				pixel = chip8->memory[chip8->I + yline];

				// a row is hardcoded to 8 bits wide
				for (ul8 xline = 0; xline < 8; ++xline) {
					// 0x80 >> 8 = 0, 0x80 >> 7 = 1, 0x80 >> 6 = 10 (in binary)
					// so this checks if each of the 8 bits in pixel is equal to 1
					if ((pixel & (0x80 >> xline)) != 0) {
						// check if the pixel on the display is set to 1
						if (chip8->screen[(x + xline + ((y + yline) * WIDTH))] == 1) {
							// register the collision using the VF register
							chip8->V[0xF] = 1;
							// XOR the new pixel value
							chip8->screen[x + xline + ((yline + y) * WIDTH)] ^= 1;
						}
					}
				}
			}

			drawFlag = 1;
			chip8->pc += 2;
		}
		break;

		case 0xE000:
			switch (chip8->opcode & 0x000F) {
				case 0x000E: // 0xEX9E: skip the next instruction if the key in VX is pressed
					if (chip8->kb[chip8->V[(chip8->opcode & 0X0F00) >> 8]] != 0) {
						chip8->pc += 4;
					} else {
						chip8->pc += 2;
					}
				break;

				case 0x0001: // 0xEXA1: skip the next instruction if the key in VX ins't pressed
					if (chip8->kb[chip8->V[(chip8->opcode & 0x0F00) >> 8]] == 0) {
						chip8->pc += 4;
					} else {
						chip8->pc += 2;
					}
				break;

				default:
					fprintf(stderr, "Unknown opcode [0xE000] 0x%X\n", chip8->opcode);
			}
		break;

		case 0xF000:
			switch (chip8->opcode & 0x00FF) {
				case 0x0007: // 0xFX07: set VX to the value of the delay timer
					chip8->V[(chip8->opcode & 0x0F00) >> 8] = chip8->delayTimer;
					chip8->pc += 2;
				break;

				case 0x000A: { // 0xFX0A: keypress is stored in VX
					ul8 keypress = 0;

					for (size_t i = 0; i < 16; ++i) {
						if (chip8->kb[i] != 0) {
							chip8->V[(chip8->opcode & 0x0F00) >> 8] = chip8->kb[i];
							keypress = 1;
						}
					}

					// Stay on this instruction until we receive a keypress
					if (!keypress)
						return;

					chip8->pc += 2;
				}
				break;

				case 0x0015: // 0xFX15: set the delay timer to VX
					chip8->delayTimer = chip8->V[(chip8->opcode & 0x0F00) >> 8];
					chip8->pc += 2;
				break;

				case 0x0018: // 0xFX18: set the sound timer to VX
					chip8->soundTimer = chip8->V[(chip8->opcode & 0x0F00) >> 8];
					chip8->pc += 2;
				break;

				case 0x001E: // 0xFX1E: add VX to I, set VF to 1 if there is carry
					if (chip8->I + chip8->V[(chip8->opcode & 0x0F00) >> 8] > 0xFFF) {
						chip8->V[0xF] = 1;
					} else {
						chip8->V[0xF] = 0;
					}

					chip8->I += chip8->V[(chip8->opcode & 0x0F00) >> 8];
					chip8->pc += 2;
				break;

				case 0x0029: // 0xFX29: set I to the location of the sprite in VX
					chip8->I = chip8->V[(chip8->opcode & 0x0F00) >> 8];
					chip8->pc += 2;
				break;

				/* 	0xFX33: store the binary-coded decimal representation 
					of VX at the addresses I, I + 1, and I + 2 */
				case 0x0033: 
					// hundreds
					chip8->memory[chip8->I] = chip8->V[(chip8->opcode & 0x0F00) >> 8] / 100;
					// tens
					chip8->memory[chip8->I + 1] = (chip8->V[(chip8->opcode & 0x0F00) >> 8] / 10) % 10;
					// digits
					chip8->memory[chip8->I + 2] = (chip8->V[(chip8->opcode & 0x0F00) >> 8] % 100) % 10;
					chip8->pc =+2;
				break;

				case 0x0055: // 0xFX55: store V0 to VX in memory starting at I
					for (size_t i = 0; i <= ((chip8->opcode & 0x0F00) >> 8); ++i) {
						chip8->memory[chip8->I + i] = chip8->V[i];
					}

					chip8->I += ((chip8->opcode & 0x0F00) >> 8) + 1;
					chip8->pc += 2;
				break;

				case 0x0065: // 0xFX65: fill V0 to VX with values from memory starting at I
					for (size_t i = 0; i < ((chip8->opcode & 0x0F00) >> 8); ++i) {
						chip8->V[i] = chip8->memory[chip8->I + i];
					}

					chip8->I += ((chip8->opcode & 0x0F00) >> 8) + 1;
					chip8->pc += 2;
				break;

				default: 
					fprintf(stderr, "Unknown opcode [0xF000]: 0x%X\n", chip8->opcode);
			}
		break;

		default:
			fprintf(stderr, "Unknown opcode 0x%X\n", chip8->opcode);
	}
	
	// Update timers

	if (chip8->delayTimer > 0) {
		--chip8->delayTimer;
	}

	if (chip8->soundTimer > 0) {
		if (chip8->soundTimer == 1) {
			fprintf(stdout, "BEEP\7\n");
		}
		--chip8->soundTimer;
	}
}

int getDrawFlag()
{
	int old = drawFlag;
	drawFlag = drawFlag ? 0 : 1;

	return old;
}

int getPixel(int x, int y)
{
	return (chip8->screen[(y*WIDTH) + x]);
}

unsigned char * getKeyboard()
{
	return chip8->kb;
}

/*void executeOpcode()
{
	chip8->I = opcode & 0x0FFF; // the 4 last bits contain the 
}*/

void printError(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	fprintf(stderr, format, args);
	va_end(args);

	exit(EXIT_FAILURE);
}