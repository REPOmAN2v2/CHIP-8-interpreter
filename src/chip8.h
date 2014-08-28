#ifndef CHIP8_H_
#define CHIP8_H_

#define WIDTH 64
#define HEIGHT 32

void loadGame(const char *game);
void cycle();
int getDrawFlag();
int getPixel(int x, int y);
unsigned char * getKeyboard();

#endif