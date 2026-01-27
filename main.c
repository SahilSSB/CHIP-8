#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <SDL2/SDL.h>

typedef struct {
    uint8_t memory[4096]; //4KB RAM
    uint8_t V[16]; // 16 REGISTERS
    uint16_t I; // INDEX REGISTER (TO STORE MEMORY ADDRESSES)
    uint16_t pc; // PROGRAM COUNTER

    uint8_t gfx[64 * 32]; // GRAPHICS BUFFER
    
    uint8_t delay_timer;
    uint8_t sound_timer;
    
    uint16_t stack[16];
    uint16_t sp; // STACK POINTER

    uint8_t keypad[16]; // KEYPAD TO STORE STATE OF KEYS
} chip8;

void init_chip8(chip8 *chip) {
    chip->pc = 0x200; // RESET PROGRAM COUNTER TO 0x200

    chip->I = 0; // RESET INDEX REGISTER
    chip->sp = 0; // RESET STACK POINTER

    // CLEAR DISPLAY, STACK, REGISTERS AND MEMORY
    memset(chip->gfx, 0, sizeof(chip->gfx));
    memset(chip->stack, 0, sizeof(chip->stack));
    memset(chip->V, 0, sizeof(chip->V));
    memset(chip->memory, 0, sizeof(chip->memory));

    uint8_t fontset[80] = {
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

    // LOAD FONTSET INTO MEMORY FROM 0x50
    for (int i = 0; i < 80; i++) {
        chip->memory[0x50 + i] = fontset[i];
    }
}

bool load_rom(chip8 *chip, const char *filename) {
    FILE *file = fopen(filename, "rb"); // OPEN FILE IN READ BINARY MODE
    if (!file) {
        printf("Failed to open ROM: %s\n", filename);
        return false;
    }

    // DETERMINE FILE SIZE
    fseek(file, 0, SEEK_END); // GO TO END 
    long size = ftell(file); // GET POSITION
    rewind(file); // GO BACK TO START

    printf("ROM SIZE: %ld BYES\n", size);

    // READ FILE INTO MEMORY STARTING FROM 0x200
    if (size > (4096-512)) { // MAX SIZE IS 4096 - 512 
        printf("ROM TOO LARGE\n");
        return false;
    }

    // fread(destination, element_size, count, stream)
    fread(&chip->memory[0x200], 1, size, file);

    fclose(file);
    return true;
}

int main(int argc, char **argv) {
    if (argc != 2) { //ARGUMENT CHECK
        printf("USAGE: ./chip8 <rom_file>\n");
        return 1;
    }

    // INITIALIZE CHIP-8
    chip8 chip;
    init_chip8(&chip);
    if (!load_rom(&chip, argv[1])) return 1;

    // INITIALIZE SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize. SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    int w = 64;
    int h = 32;
    int scale = 15; // FOR SCALING THE WINDOW

    SDL_Window *window = SDL_CreateWindow("CHIP-8 EMULATOR",
                                        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                        w * scale, h * scale,
                                        SDL_WINDOW_SHOWN);
    
    if (!window) {
        printf("Window could not be created. SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    // CREATE A RENDERER
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    if (!renderer) {
        printf("Renderer could not be created. SDL_Error: %s\n", SDL_GetError());
        return -1;
    }

    SDL_Texture *texture = SDL_CreateTexture(renderer,
                                                SDL_PIXELFORMAT_RGBA8888,
                                                SDL_TEXTUREACCESS_STREAMING,
                                                w, h);
    // MAIN LOOP
    bool quit = false;
    SDL_Event event;
    uint32_t pixels[64 * 32]; // TEMPORARY BUFFER FOR SDL COLORS

    while (!quit) {
        while (SDL_PollEvent(&event)) { // EVENT HANDLING FOR KEYBOARD, WINDOW, CLOSE
            if (event.type == SDL_QUIT) {
                quit = true;
            }
        }

        // CONVERT CHIP - 8 ONE BIT PIXELS TO SDL 32 BIT COLORS
        for (int i = 0; i < 64 * 32; i++) {
            uint8_t pixel = chip.gfx[i];
            
            // IF PIXEL IS 1 THEN WHITE, IF PIXEL IS 0 THEN BLACK
            pixels[i] = (pixel ? 0xFFFFFFFF : 0x000000FF);
        }

        // UPDATE TEXTURE AND RENDER
        SDL_UpdateTexture(texture, NULL, pixels, 64 * sizeof(uint32_t));
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);

        // SMALL DELAY TO PREVENT RUNNING AT 100% CPU
        SDL_Delay(1);
    }

    // CLEANUP
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
