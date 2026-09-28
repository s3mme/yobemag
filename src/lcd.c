#include <SDL2/SDL.h>
#include <stdbool.h>
#include <stdlib.h>

#include "lcd.h"
#include "log.h"

/******************************************************
 *** LOCAL VARIABLES                                ***
 ******************************************************/

#define WINDOW_WIDTH  (640)
#define WINDOW_HEIGHT (576)

static SDL_Window *window;
static SDL_Surface *surface;

/******************************************************
 *** EXPOSED METHODS                                ***
 ******************************************************/

void lcd_init(void) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        LOG_ERROR("SDL_Init failed: %s", SDL_GetError());
        exit(EXIT_FAILURE);
    }
    LOG_DEBUG("SDL_Init finished");

    window = SDL_CreateWindow("yobemag GB Emulator", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH,
                              WINDOW_HEIGHT, SDL_WINDOW_INPUT_FOCUS);
    if (window == NULL) {
        LOG_ERROR("SDL_CreateWindow failed: %s", SDL_GetError());
        exit(EXIT_FAILURE);
    }
    LOG_DEBUG("SDL_CreateWindow finished");

    surface = SDL_GetWindowSurface(window);
    SDL_UpdateWindowSurface(window);
}

void lcd_teardown(void) {
    SDL_DestroyWindow(window);
    SDL_Quit();
}

bool lcd_step(void) {
    SDL_Event e;
    const uint8_t *key_states;

    key_states = SDL_GetKeyboardState(NULL);

    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            return true;
        }
    }

    if (key_states[SDL_SCANCODE_Q]) {
        return true;
    }

    if (key_states[SDL_SCANCODE_A]) {
        LOG_INFO("a");
    }

    return false;
}
