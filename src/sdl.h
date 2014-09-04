#ifndef SDL_H_
#define SDL_H_

#include <SDL2/SDL.h>
#include <stdbool.h>

void initialiseSDL();
void drawGraphics();
void setKeys(int key, bool flag);

#endif