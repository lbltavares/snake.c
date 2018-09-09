#undef _MSC_VER
#include "SDL2/SDL.h"
#include "SDL2/SDL_image.h"
#include "define.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#define SNAKE_MAX_SIZE MAP_WIDTH *MAP_HEIGHT
#define INACTIVE -5

void initSDL();
void initGame();
void run();
void quitSDL();

void randomFreePosition(int *x, int *y);
void spawnApple(int n);
void setDirection(int p, int x, int y);
void handleEvents(const SDL_Event *event);
void getTileSize(int *tileWidth, int *tileHeight);

void resetSnake(int n);
void updateSnake(int n);
void updateScore();
void updateSnakeCPU(int n);
void renderSnake(int n);
void renderApple(int n);
void drawGrid();

SDL_Texture *loadTexture(const char *path);
SDL_bool checkCollision(int n, int x, int y);
void mapPosition(int *x, int *y);

void update();
void render();

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
SDL_bool running;
SDL_bool gameOver;
char windowTitle[25];

SDL_Texture *appleImg = NULL;
SDL_Texture *snakeTextures[NUM_PLAYERS];

SDL_Point direction[NUM_PLAYERS];
SDL_Point apples[NUM_APPLES];
SDL_Point snakes[NUM_PLAYERS][SNAKE_MAX_SIZE];
long int snake_times[NUM_PLAYERS];
int snake_sizes[NUM_PLAYERS];

int scores[NUM_PLAYERS];

void update()
{
    for (int i = 0; i < NUM_PLAYERS; i++)
        updateSnake(i);

    updateScore();
}

void render()
{
    SDL_SetRenderDrawColor(renderer, BACKGROUND_COLOR, 255);
    SDL_RenderClear(renderer);

    drawGrid();

    for (int i = 0; i < NUM_PLAYERS; i++)
        renderSnake(i);

    for (int i = 0; i < NUM_APPLES; i++)
        renderApple(i);

    SDL_RenderPresent(renderer);
}

void randomFreePosition(int *x, int *y)
{
    long int blockedPositionsSize = 0;
    for (int p = 0; p < NUM_PLAYERS; p++)
    {
        blockedPositionsSize += snake_sizes[p];
    }
    blockedPositionsSize += NUM_APPLES;

    SDL_Point blockedPositions[blockedPositionsSize];
    long int blockedIndex = 0;
    for (int p = 0; p < NUM_PLAYERS; p++)
    {
        for (int n = 0; n < snake_sizes[p]; n++)
        {
            blockedPositions[blockedIndex].x = snakes[p][n].x;
            blockedPositions[blockedIndex].y = snakes[p][n].y;
            blockedIndex++;
        }
    }
    for (int a = 0; a < NUM_APPLES; a++)
    {
        blockedPositions[blockedIndex].x = apples[a].x;
        blockedPositions[blockedIndex].y = apples[a].y;
        blockedIndex++;
    }

    SDL_Point freePositions[MAP_WIDTH * MAP_HEIGHT - blockedPositionsSize];
    long int freeIndex = 0;
    for (int my = 0; my < MAP_HEIGHT; my++)
    {
        for (int mx = 0; mx < MAP_WIDTH; mx++)
        {
            int valid = 1;
            for (long int b = 0; b < blockedPositionsSize; b++)
            {
                if (blockedPositions[b].x == mx && blockedPositions[b].y == my)
                {
                    valid = 0;
                    break;
                }
            }
            if (valid)
            {
                freePositions[freeIndex].x = mx;
                freePositions[freeIndex].y = my;
                freeIndex++;
            }
        }
    }
    long int choosen = rand() % freeIndex;
    *x = freePositions[choosen].x;
    *y = freePositions[choosen].y;
}

SDL_bool checkCollision(int n, int x, int y)
{
    if (!INFINITE_BORDER && x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT)
        return SDL_TRUE;

    for (int i = 0; i < SNAKE_MAX_SIZE; i++)
    {
        if (snakes[n][i].x == INACTIVE)
            break;
        else if (x == snakes[n][i].x && y == snakes[n][i].y)
            return SDL_TRUE;
    }
    return SDL_FALSE;
}

void mapPosition(int *x, int *y)
{
    (*x) %= MAP_WIDTH;
    (*y) %= MAP_HEIGHT;
    (*x) = (*x) < 0 ? MAP_WIDTH + (*x) : (*x);
    (*y) = (*y) < 0 ? MAP_HEIGHT + (*y) : (*y);
}

void updateSnakeCPU(int n)
{
    int snake_size = snake_sizes[n];

    float leastDist = 9999999.0f;
    int nearestApple = 0;
    for (int a = 0; a < NUM_APPLES; a++)
    {
        float distX = abs(apples[a].x - snakes[n][snake_size - 1].x);
        float distY = abs(apples[a].y - snakes[n][snake_size - 1].y);
        float dist = sqrt(distX * distX + distY * distY);
        if (dist < leastDist)
        {
            leastDist = dist;
            nearestApple = a;
        }
    }

    int distX = apples[nearestApple].x - snakes[n][snake_size - 1].x;
    int distY = apples[nearestApple].y - snakes[n][snake_size - 1].y;

    int targetX = distX == 0 ? 0 : distX / abs(distX);
    int targetY = targetX == 0 ? distY == 0 ? 0 : distY / abs(distY) : 0;

    setDirection(n, targetX, targetY);

    int newX = (snakes[n][snake_size - 1].x + direction[n].x);
    int newY = (snakes[n][snake_size - 1].y + direction[n].y);

    if (INFINITE_BORDER)
        mapPosition(&newX, &newY);

    for (int i = 0; i < NUM_PLAYERS; i++)
    {
        if (i == n)
            continue;
        if (checkCollision(i, newX, newY))
        {
            resetSnake(n);
            return;
        }
    }

    for (int i = 0; i < NUM_APPLES; i++)
    {
        if (newX == apples[i].x && newY == apples[i].y)
        {
            snake_sizes[n]++;
            snakes[n][snake_sizes[n] - 1].x = newX;
            snakes[n][snake_sizes[n] - 1].y = newY;
            spawnApple(i);
            return;
        }
    }

    if (snake_size > 1)
    {
        for (int i = 0; i < snake_size - 1; i++)
        {
            snakes[n][i].x = snakes[n][i + 1].x;
            snakes[n][i].y = snakes[n][i + 1].y;
        }
    }
    snakes[n][snake_size - 1].x = newX;
    snakes[n][snake_size - 1].y = newY;
}

void updateSnake(int n)
{
    if (snake_times[n] + SNAKE_DELAY > SDL_GetTicks())
        return;
    snake_times[n] = SDL_GetTicks();
    scores[n] = snake_sizes[n];
    if (SNAKE_CPU[n])
    {
        updateSnakeCPU(n);
        return;
    }

    int snake_size = snake_sizes[n];

    int newX = (snakes[n][snake_size - 1].x + direction[n].x);
    int newY = (snakes[n][snake_size - 1].y + direction[n].y);

    if (INFINITE_BORDER)
        mapPosition(&newX, &newY);

    for (int i = 0; i < NUM_PLAYERS; i++)
    {
        if (i == n)
            continue;
        if (checkCollision(i, newX, newY))
        {
            resetSnake(n);
            return;
        }
    }

    for (int i = 0; i < NUM_APPLES; i++)
    {
        if (newX == apples[i].x && newY == apples[i].y)
        {
            snake_sizes[n]++;
            snakes[n][snake_sizes[n] - 1].x = newX;
            snakes[n][snake_sizes[n] - 1].y = newY;
            spawnApple(i);
            return;
        }
    }

    if (snake_size > 1)
    {
        for (int i = 0; i < snake_size - 1; i++)
        {
            snakes[n][i].x = snakes[n][i + 1].x;
            snakes[n][i].y = snakes[n][i + 1].y;
        }
    }
    snakes[n][snake_size - 1].x = newX;
    snakes[n][snake_size - 1].y = newY;
}

void renderSnake(int n)
{
    int tileWidth, tileHeight;
    getTileSize(&tileWidth, &tileHeight);
    int snake_size = snake_sizes[n];

    for (int i = 0; i < snake_size; i++)
    {
        if (snakes[n][i].x == INACTIVE)
            break;
        SDL_Rect rect;
        rect.x = tileWidth * snakes[n][i].x;
        rect.y = tileHeight * snakes[n][i].y;
        rect.w = tileWidth;
        rect.h = tileHeight;

        SDL_Rect windowRect;
        windowRect.x = windowRect.y = 0;
        windowRect.w = WINDOW_WIDTH;
        windowRect.h = WINDOW_HEIGHT;
        SDL_RenderCopy(renderer, snakeTextures[n], &windowRect, &rect);
        //SDL_RenderFillRect(renderer, &rect);
    }
}

SDL_Texture *loadTexture(const char *path)
{
    SDL_Surface *surf = IMG_Load(path);
    if (surf == NULL)
    {
        printf("Unable to load image %s! SDL_image Error: %s\n", path, IMG_GetError());
    }
    SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
    SDL_FreeSurface(surf);
    return tex;
}

void renderApple(int n)
{
    int tileWidth, tileHeight;
    getTileSize(&tileWidth, &tileHeight);

    SDL_SetRenderDrawColor(renderer, APPLE_COLOR, 255);
    SDL_Rect rect;
    rect.x = tileWidth * apples[n].x;
    rect.y = tileHeight * apples[n].y;
    rect.w = tileWidth;
    rect.h = tileHeight;

    SDL_Rect windowRect;
    windowRect.x = windowRect.y = 0;
    windowRect.w = WINDOW_WIDTH;
    windowRect.h = WINDOW_HEIGHT;

    SDL_RenderCopy(renderer, appleImg, &windowRect, &rect);
    //SDL_RenderFillRect(renderer, &rect);
}

void spawnApple(int n)
{
    randomFreePosition(&apples[n].x, &apples[n].y);
}

void getTileSize(int *tileWidth, int *tileHeight)
{
    int w, h;
    SDL_GetWindowSize(window, &w, &h);
    *tileWidth = w / MAP_WIDTH * 1.0;
    *tileHeight = h / MAP_HEIGHT * 1.0;
}

void setDirection(int p, int x, int y)
{
    if (snake_sizes[p] > 1)
    {
        int t_x = (snakes[p][snake_sizes[p] - 1].x + x);
        int t_y = (snakes[p][snake_sizes[p] - 1].y + y);
        if (INFINITE_BORDER)
            mapPosition(&t_x, &t_y);
        if (snakes[p][snake_sizes[p] - 2].x == t_x && snakes[p][snake_sizes[p] - 2].y == t_y)
            return;
    }
    direction[p].x = x;
    direction[p].y = y;
}

void handleEvents(const SDL_Event *event)
{
    if (event->type == SDL_KEYDOWN)
    {
        SDL_Keycode code = event->key.keysym.sym;
        if (code == SDLK_w)
        {
            setDirection(0, 0, -1);
        }
        else if (code == SDLK_s)
        {
            setDirection(0, 0, 1);
        }
        else if (code == SDLK_d)
        {
            setDirection(0, 1, 0);
        }
        else if (code == SDLK_a)
        {
            setDirection(0, -1, 0);
        }

        if (code == SDLK_UP)
        {
            setDirection(1, 0, -1);
        }
        else if (code == SDLK_DOWN)
        {
            setDirection(1, 0, 1);
        }
        else if (code == SDLK_RIGHT)
        {
            setDirection(1, 1, 0);
        }
        else if (code == SDLK_LEFT)
        {
            setDirection(1, -1, 0);
        }
    }
}

void drawGrid()
{
    int tileWidth, tileHeight;
    getTileSize(&tileWidth, &tileHeight);
    SDL_SetRenderDrawColor(renderer, GRID_COLOR, 255);
    for (int w = 0; w < WINDOW_WIDTH; w++)
    {
        SDL_RenderDrawLine(renderer, w * tileWidth, 0, w * tileWidth, WINDOW_HEIGHT);
    }

    for (int h = 0; h < WINDOW_HEIGHT; h++)
    {
        SDL_RenderDrawLine(renderer, 0, h * tileHeight, WINDOW_WIDTH, h * tileHeight);
    }
}

void initSDL()
{
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
    {
        printf("SDL_Init error: %s", SDL_GetError());
        exit(-1);
    }

    SDL_CreateWindowAndRenderer(WINDOW_WIDTH, WINDOW_HEIGHT,
                                RESIZABLE_WINDOW ? SDL_WINDOW_RESIZABLE : SDL_WINDOW_SHOWN,
                                &window, &renderer);

    if (!window || !renderer)
    {
        printf("SDL_CreateWindowAndRenderer error: %s", SDL_GetError());
        exit(-1);
    }

    int imgFlags = IMG_INIT_PNG;
    if (!(IMG_Init(imgFlags) & imgFlags))
    {
        printf("IMG_Init error: %s\n", IMG_GetError());
        SDL_Quit();
        exit(-1);
    }

    appleImg = loadTexture("res/apple.png");
    for (int i = 0; i < NUM_PLAYERS; i++)
        snakeTextures[i] = loadTexture(SNAKE_IMGS[i]);
}

void updateScore()
{
    int best = 0;
    for (int i = 0; i < NUM_PLAYERS; i++)
    {
        if (scores[i] > best)
        {
            best = scores[i];
            sprintf(windowTitle, "Top: %s - %d", SNAKE_NAMES[i], best);
        }
    }
    SDL_SetWindowTitle(window, windowTitle);
}

void resetSnake(int n)
{
    for (int i = 0; i < SNAKE_MAX_SIZE; i++)
        snakes[n][i].x = snakes[n][i].y = INACTIVE;
    randomFreePosition(&snakes[n][0].x, &snakes[n][0].y);
    snake_sizes[n] = 1;
    snake_times[n] = 0;
    scores[n] = 0;
}

void initGame()
{
    running = SDL_TRUE;
    gameOver = SDL_FALSE;

    direction[0].x = 1;
    direction[0].y = 0;

    direction[1].x = 1;
    direction[1].y = 0;

    for (int p = 0; p < NUM_PLAYERS; p++)
        resetSnake(p);

    srand(time(NULL));
    for (int a = 0; a < NUM_APPLES; a++)
        spawnApple(a);
}

void run()
{
    long int time = 0;
    running = SDL_TRUE;
    SDL_Event event;
    while (running)
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
                running = SDL_FALSE;
            else
            {
                handleEvents(&event);
            }
        }

        if (time + 1000.0 / FPS > SDL_GetTicks())
            continue;
        time = SDL_GetTicks();

        update();
        render();
    }
}

void quitSDL()
{
    IMG_Quit();
    SDL_DestroyTexture(appleImg);
    for (int i = 0; i < NUM_PLAYERS; i++)
        SDL_DestroyTexture(snakeTextures[i]);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

int main(int argi, char **args)
{
    initSDL();
    initGame();
    run();
    quitSDL();
    return 0;
}