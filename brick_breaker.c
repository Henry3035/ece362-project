#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// --- Display parameters ---
#define MATRIX_WIDTH 64
#define MATRIX_HEIGHT 32
#define SCALE 10

// --- Game parameters ---
#define PADDLE_WIDTH 8
#define PADDLE_HEIGHT 2
#define BALL_SIZE 2
#define BLOCK_COLS 8
#define BLOCK_ROWS 3
#define BLOCK_WIDTH (MATRIX_WIDTH / BLOCK_COLS)
#define BLOCK_HEIGHT 4

// Global game variables
int paddle_x;
int paddle_y = MATRIX_HEIGHT - PADDLE_HEIGHT;
int ball_x;
int ball_y;
int ball_dx = 1;
int ball_dy = -1;
uint8_t blocks[BLOCK_ROWS][BLOCK_COLS];
int score;

// SDL variables
SDL_Window* window;
SDL_Renderer* renderer;
Mix_Chunk* bounceSound;
Mix_Chunk* breakSound;
Mix_Chunk* loseLifeSound;
Mix_Chunk* levelUpSound;

// Function prototypes
void init_game(void);
void update_paddle(void);
void update_ball(void);
void draw_frame(void);
void play_bounce_sound(void);
void play_break_sound(void);
void play_lose_life_sound(void);
void play_level_up_sound(void);

int main(void) {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        printf("SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }

    // Create window
    window = SDL_CreateWindow("Brick Breaker", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
                             MATRIX_WIDTH * SCALE, MATRIX_HEIGHT * SCALE, 0);
    if (!window) {
        printf("SDL_CreateWindow Error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    // Create renderer
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        printf("SDL_CreateRenderer Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Initialize SDL_mixer
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        printf("Mix_OpenAudio Error: %s\n", Mix_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Load sound effects (assuming WAV files are in the same directory)
    bounceSound = Mix_LoadWAV("bounce.wav");
    breakSound = Mix_LoadWAV("break.wav");
    loseLifeSound = Mix_LoadWAV("lose_life.wav");
    levelUpSound = Mix_LoadWAV("level_up.wav");
    if (!bounceSound || !breakSound || !loseLifeSound || !levelUpSound) {
        printf("Mix_LoadWAV Error: %s\n", Mix_GetError());
        Mix_CloseAudio();
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    init_game();

    int running = 1;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            }
        }

        update_paddle();
        update_ball();
        draw_frame();
        SDL_Delay(30);
    }

    // Clean up
    Mix_FreeChunk(bounceSound);
    Mix_FreeChunk(breakSound);
    Mix_FreeChunk(loseLifeSound);
    Mix_FreeChunk(levelUpSound);
    Mix_CloseAudio();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

void init_game(void) {
    paddle_x = (MATRIX_WIDTH - PADDLE_WIDTH) / 2;
    ball_x = MATRIX_WIDTH / 2;
    ball_y = MATRIX_HEIGHT / 2;
    ball_dx = 1;
    ball_dy = -1;
    score = 0;
    for (int r = 0; r < BLOCK_ROWS; r++)
        for (int c = 0; c < BLOCK_COLS; c++)
            blocks[r][c] = 1;
}

void update_paddle(void) {
    int mouseX, mouseY;
    SDL_GetMouseState(&mouseX, &mouseY);
    paddle_x = mouseX / SCALE;
    if (paddle_x < 0) paddle_x = 0;
    if (paddle_x > MATRIX_WIDTH - PADDLE_WIDTH) 
        paddle_x = MATRIX_WIDTH - PADDLE_WIDTH;
}

void update_ball(void) {
    ball_x += ball_dx;
    ball_y += ball_dy;

    if (ball_x <= 0 || ball_x + BALL_SIZE >= MATRIX_WIDTH) {
        ball_dx = -ball_dx;
        play_bounce_sound();
    }

    if (ball_y <= 0) {
        ball_dy = -ball_dy;
        play_bounce_sound();
    }

    if (ball_y + BALL_SIZE >= paddle_y &&
        ball_x + BALL_SIZE >= paddle_x &&
        ball_x <= paddle_x + PADDLE_WIDTH) {
        ball_dy = -ball_dy;
        play_bounce_sound();
    }

    for (int r = 0; r < BLOCK_ROWS; r++) {
        for (int c = 0; c < BLOCK_COLS; c++) {
            if (blocks[r][c]) {
                int bx = c * BLOCK_WIDTH;
                int by = r * BLOCK_HEIGHT;
                if (ball_x + BALL_SIZE > bx && ball_x < bx + BLOCK_WIDTH &&
                    ball_y + BALL_SIZE > by && ball_y < by + BLOCK_HEIGHT) {
                    blocks[r][c] = 0;
                    ball_dy = -ball_dy;
                    score++;
                    play_break_sound();
                    return;
                }
            }
        }
    }

    int all_blocks_broken = 1;
    for (int r = 0; r < BLOCK_ROWS; r++) {
        for (int c = 0; c < BLOCK_COLS; c++) {
            if (blocks[r][c]) {
                all_blocks_broken = 0;
                break;
            }
        }
    }

    if (all_blocks_broken) {
        play_level_up_sound();
        play_level_up_sound();
        init_game();
    }

    if (ball_y >= MATRIX_HEIGHT) {
        for (int i = 0; i < 2; i++) {
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);
            SDL_RenderPresent(renderer);
            SDL_Delay(200);
            draw_frame();
            SDL_Delay(200);
        }
        play_lose_life_sound();
        init_game();
    }
}

void draw_frame(void) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_Rect paddleRect = {paddle_x * SCALE, paddle_y * SCALE, 
                          PADDLE_WIDTH * SCALE, PADDLE_HEIGHT * SCALE};
    SDL_RenderFillRect(renderer, &paddleRect);

    SDL_Rect ballRect = {ball_x * SCALE, ball_y * SCALE, 
                        BALL_SIZE * SCALE, BALL_SIZE * SCALE};
    SDL_RenderFillRect(renderer, &ballRect);

    for (int r = 0; r < BLOCK_ROWS; r++) {
        for (int c = 0; c < BLOCK_COLS; c++) {
            if (blocks[r][c]) {
                int bx = c * BLOCK_WIDTH;
                int by = r * BLOCK_HEIGHT;
                SDL_Rect blockRect = {bx * SCALE, by * SCALE, 
                                    BLOCK_WIDTH * SCALE, BLOCK_HEIGHT * SCALE};
                SDL_RenderFillRect(renderer, &blockRect);
            }
        }
    }

    SDL_RenderPresent(renderer);
}

void play_bounce_sound(void) {
    Mix_PlayChannel(-1, bounceSound, 0);
}

void play_break_sound(void) {
    Mix_PlayChannel(-1, breakSound, 0);
}

void play_lose_life_sound(void) {
    Mix_PlayChannel(-1, loseLifeSound, 0);
}

void play_level_up_sound(void) {
    Mix_PlayChannel(-1, levelUpSound, 0);
}