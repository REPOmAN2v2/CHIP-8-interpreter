#ifndef CHIP8_H_
#define CHIP8_H_

#include <stdbool.h>

#define WIDTH 64
#define HEIGHT 32

void loadGame(const char *game);
void execute();
void updateTimers();

// Getters 
int getPixel(int x, int y);
unsigned char * getKeyboard();

#endif