#ifndef _DEFINE_H
#define _DEFINE_H

#define MAP_WIDTH 52
#define MAP_HEIGHT 28
#define INFINITE_BORDER 1

#define WINDOW_WIDTH MAP_WIDTH * 20
#define WINDOW_HEIGHT MAP_HEIGHT * 20
#define RESIZABLE_WINDOW 0
#define FPS 60

#define NUM_PLAYERS 3
#define NUM_APPLES 10

#define BACKGROUND_COLOR 190, 190, 190
#define GRID_COLOR 200, 200, 200
#define APPLE_COLOR 0, 255, 0

#define SNAKE_DELAY 100

const char *SNAKE_NAMES[NUM_PLAYERS] = {
    "Ana",
    "Joao",
    "Maria",
};

const char *SNAKE_IMGS[NUM_PLAYERS] = {
    "res/blue.png",
    "res/red.png",
    "res/pink.png",
};

const int SNAKE_CPU[NUM_PLAYERS] = {
    1,
    1,
    1,
};

#endif