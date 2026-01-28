#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <stdio.h>
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
    bool draw_flag;
} chip8;

void init_chip8(chip8 *chip) {
    chip->pc = 0x200; // RESET PROGRAM COUNTER TO 0x200

    chip->I = 0; // RESET INDEX REGISTER
    chip->sp = 0; // RESET STACK POINTER

    chip->delay_timer = 0;
    chip->sound_timer = 0;
    chip->draw_flag = false;
    
    // CLEAR DISPLAY, STACK, REGISTERS AND MEMORY
    memset(chip->gfx, 0, sizeof(chip->gfx));
    memset(chip->stack, 0, sizeof(chip->stack));
    memset(chip->V, 0, sizeof(chip->V));
    memset(chip->memory, 0, sizeof(chip->memory));
    memset(chip->keypad, 0, sizeof(chip->keypad));


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

    srand(time(NULL));
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

void emulate(chip8 *chip) {
    // COMBINE TWO BYES TO MAKE ONE 16 BIT OPCODE
    uint16_t opcode = (chip->memory[chip->pc] << 8) | chip->memory[chip->pc + 1];

    // DECODE AND EXECUTE
    switch (opcode & 0xF000) {
        case 0x0000:
            switch (opcode & 0x00FF) {
                case 0xE0:
                    memset(chip->gfx, 0, sizeof(chip->gfx));
                    chip->draw_flag = true;
                    chip->pc += 2;
                    break;
                case 0xEE:
                    chip->sp--;
                    chip->pc = chip->stack[chip->sp];
                    chip->pc += 2;
                    break;
                default:
                    printf("Unknown opcode [0x000]: 0x%X\n", opcode);
                    chip->pc += 2;
                    break;
            }
            break;
        case 0x1000:
            chip->pc = (opcode & 0x0FFF);
            break;
        case 0x2000:
            chip->stack[chip->sp] = chip->pc;
            chip->sp++;
            chip->pc = (opcode & 0x0FFF);
            break;
        case 0x3000: {
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t kk = (opcode & 0x00FF);
            if (chip->V[x] == kk) {
                chip->pc += 4;
            }
            else {
                chip->pc += 2;
            }
        }
            break;
        case 0x4000: {
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t kk = (opcode & 0x00FF);
            if (chip->V[x] != kk) {
                chip->pc += 4;
            }
            else {
                chip->pc +=2;
            }
        }
            break;
        case 0x5000: {
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t y = (opcode & 0x00F0) >> 4;
            if (chip->V[x] == chip->V[y]) {
                chip->pc += 4;
            }
            else {
                chip->pc += 2;
            }
        }
            break;
        case 0x6000: {
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t kk = (opcode & 0x00FF);
            chip->V[x] = kk;
            chip->pc += 2;
        }
            break;
        case 0x7000: {
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t kk = (opcode & 0x00FF);
            chip->V[x] = chip->V[x] + kk;
            chip->pc += 2;
        }
            break;
        case 0x8000: 
            switch (opcode & 0x000F) {
                case 0x0: {
                    uint8_t x = (opcode & 0x0F00) >> 8;
                    uint8_t y = (opcode & 0x00F0) >> 4;
                    chip->V[x] = chip->V[y];
                    chip->pc += 2;
                }
                    break;
                case 0x1: {
                    uint8_t x = (opcode & 0x0F00) >> 8;
                    uint8_t y = (opcode & 0x00F0) >> 4;
                    chip->V[x] = chip->V[x] | chip->V[y];
                    chip->pc += 2;
                }
                    break;
                case 0x2: {
                    uint8_t x = (opcode & 0x0F00) >> 8;
                    uint8_t y = (opcode & 0x00F0) >> 4;
                    chip->V[x] = chip->V[x] & chip->V[y];
                    chip->pc += 2;
                }
                    break;
                case 0x3: {
                    uint8_t x = (opcode & 0x0F00) >> 8;
                    uint8_t y = (opcode & 0x00F0) >> 4;
                    chip->V[x] = chip->V[x] ^ chip->V[y];
                    chip->pc += 2;
                }
                    break;
                case 0x4: {
                    uint8_t x = (opcode & 0x0F00) >> 8;
                    uint8_t y = (opcode & 0x00F0) >> 4;
                    uint16_t sum = chip->V[x] + chip->V[y];
                    if (sum > 255) {
                        chip->V[0xF] = 1;
                    }
                    else {
                        chip->V[0xF] = 0;
                    }
                    chip->V[x] = sum & 0xFF;
                    chip->pc += 2;
                }
                    break;
                case 0x5: {
                    uint8_t x = (opcode & 0x0F00) >> 8;
                    uint8_t y = (opcode & 0x00F0) >> 4;
                    if (chip->V[x] > chip->V[y]) {
                        chip->V[0xF] = 1;
                    }
                    else {
                        chip->V[0xF] = 0;
                    }
                    chip->V[x] = chip->V[x] - chip->V[y];
                    chip->pc += 2;
                }
                    break;
                case 0x6: {
                    uint8_t x = (opcode & 0x0F00) >> 8;
                    chip->V[0xF] = (chip->V[x] & 0x1);
                    chip->V[x] >>=1;
                    chip->pc += 2;
                }
                    break;
                case 0x7: {
                    uint8_t x = (opcode & 0x0F00) >> 8;
                    uint8_t y = (opcode & 0x00F0) >> 4;
                    if (chip->V[y] > chip->V[x]) {
                        chip->V[0xF] = 1;
                    }
                    else {
                        chip->V[0xF] = 0;
                    }
                    chip->V[x] = chip->V[y] - chip->V[x];
                    chip->pc += 2;
                }
                    break;
                case 0xE: {
                    uint8_t x = (opcode & 0x0F00) >> 8;
                    chip->V[0xF] = (chip->V[x] & 0x80) >> 7;
                    chip->V[x] <<= 1;
                    chip->pc += 2;
                }
                    break;
                default:
                    printf("Unknown opcode[0x8000]: 0x%X\n", opcode);
                    chip->pc += 2;
                    break;
            }   
            break;
        case 0x9000: {
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t y = (opcode & 0x00F0) >> 4;
            if (chip->V[x] != chip->V[y]) {
                chip->pc += 4;
            }
            else {
                chip->pc += 2;
            }
        }
            break;
        case 0xA000: {
            chip->I = (opcode & 0x0FFF);
            chip->pc += 2;
        }
            break;
        case 0xB000: {
            chip->pc = (opcode & 0x0FFF) + chip->V[0];
        }
            break;
        case 0xC000: {
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t kk = (opcode & 0x00FF);
            uint8_t random = rand() % 256;
            chip->V[x] = random & kk;
            chip->pc += 2;
        }
            break;
        case 0xD000: {
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t y = (opcode & 0x00F0) >> 4;
            x = chip->V[x];
            y = chip->V[y];
            uint8_t height = (opcode & 0x000F);
            chip->V[0xF] = 0;
            for (int row = 0; row < height; row++) {
                uint8_t pixel_row = chip->memory[chip->I + row];
                for (int col = 0; col < 8; col++) {
                    if ((pixel_row & (0x80 >> col)) != 0) {
                        int scrX = (x + col) % 64;
                        int scrY = (y + row) % 32;
                        int index = scrX + (scrY * 64);
                        if (chip->gfx[index] == 1) {
                            chip->V[0xF] = 1;
                        }
                        chip->gfx[index] ^= 1;
                    }
                }
            }
            chip->draw_flag = true;
            chip->pc += 2;
        }
            break;
        case 0xE000: 
           switch (opcode & 0x00FF) {
            case 0x9E: {
                uint8_t x = (opcode & 0x0F00) >> 8;
                uint8_t key = chip->V[x];
                if (chip->keypad[key] != 0) {
                    chip->pc += 4;
                }
                else {
                    chip->pc += 2;
                }
            }
                break;
            case 0xA1: {
                uint8_t x = (opcode & 0x0F00) >> 8;
                uint8_t key = chip->V[x];
                if (chip->keypad[key] == 0) {
                    chip->pc += 4;
                }
                else {
                    chip->pc += 2;
                }
            }
                break;
            default:
                printf("Unknown Opcode [0xE000]: 0x%X\n", opcode);
                chip->pc += 2;
                break;
           }
           break;
        case 0xF000:
           switch (opcode & 0x00FF) {
                case 0x07: {
                    uint8_t x = (opcode & 0x0F00) >> 8;
                    chip->V[x] = chip->delay_timer;
                    chip->pc += 2;
                }
                    break;
                case 0x0A: {
                    uint8_t x = (opcode & 0x0F00) >> 8;
                    bool key_pressed = false;
                    for (int i = 0; i < 16; i++) {
                        if (chip->keypad[i] != 0) {
                            chip->V[x] = i;
                            key_pressed = true;
                            break;
                        }
                    }
                    if (key_pressed) {
                        chip->pc += 2;
                    }
                }
                    break;
                case 0x15: {
                    uint8_t x = (opcode & 0x0F00) >> 8;
                    chip->delay_timer = chip->V[x];
                    chip->pc += 2;
                }
                    break;
                case 0x18: {
                    uint8_t x = (opcode & 0x0F00) >> 8;
                    chip->sound_timer = chip->V[x];
                    chip->pc += 2;
                }
                    break;
                case 0x1E: {
                    uint8_t x = (opcode & 0x0F00) >> 8;
                    chip->I += chip->V[x];
                    chip->pc += 2;
                }
                    break;
                case 0x29: {
                    uint8_t x = (opcode & 0x0F00) >> 8;
                    uint8_t digit = chip->V[x];
                    chip->I = 0x50 + (digit * 5); 
                    chip->pc += 2;
                }
                    break;
                case 0x33: {
                    uint8_t x = (opcode & 0x0F00) >> 8;
                    uint8_t value = chip->V[x];
                    chip->memory[chip->I] = value / 100;
                    chip->memory[chip->I + 1] = (value / 10) % 10;
                    chip->memory[chip->I + 2] = value % 10;
                    chip->pc += 2;
                }
                    break;
                case 0x55: {
                    uint8_t x = (opcode & 0x0F00) >> 8;
                    for (int i = 0; i <= x; i++) {
                        chip->memory[chip->I + i] = chip->V[i];
                    }
                    chip->I += x + 1;
                    chip->pc += 2;
                }
                    break;
                case 0x65: {
                    uint8_t x = (opcode & 0x0F00) >> 8;
                    for (int i = 0; i <= x; i++) {
                    chip->V[i] = chip->memory[chip->I + i];
                    }
                    chip->I += x + 1;
                    chip->pc += 2;
                }
                    break;
                default:
                    printf("Unknown opcode [0xF000]: 0x%X\n", opcode);
                    chip->pc += 2;
                    break;
                }
                break;
        default:
            printf("Unknown opcode: 0x%X\n", opcode);
            chip->pc += 2;
            break;
    }   

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
        SDL_Quit();
        return 1;
    }

    // CREATE A RENDERER
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    if (!renderer) {
        printf("Renderer could not be created. SDL_Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    SDL_Texture *texture = SDL_CreateTexture(renderer,
                                                SDL_PIXELFORMAT_RGBA8888,
                                                SDL_TEXTUREACCESS_STREAMING,
                                                w, h);
 
    if (!texture) {
        printf("Texture could not be created. SDL_Error: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // MAIN LOOP
    bool quit = false;
    SDL_Event event;
    uint32_t pixels[64 * 32]; // TEMPORARY BUFFER FOR SDL COLORS

    while (!quit) {
        while (SDL_PollEvent(&event)) { // EVENT HANDLING FOR KEYBOARD, WINDOW, CLOSE
            if (event.type == SDL_QUIT) {
                quit = true;
            }

            // MAP KEYS FROM COMPUTER TO THE CHIP-8
            if (event.type == SDL_KEYDOWN) {
                    switch (event.key.keysym.sym) {
                        case SDLK_x: chip.keypad[0] = 1; break;
                        case SDLK_1: chip.keypad[1] = 1; break;
                        case SDLK_2: chip.keypad[2] = 1; break;
                        case SDLK_3: chip.keypad[3] = 1; break;
                        case SDLK_q: chip.keypad[4] = 1; break;
                        case SDLK_w: chip.keypad[5] = 1; break;
                        case SDLK_e: chip.keypad[6] = 1; break;
                        case SDLK_a: chip.keypad[7] = 1; break;
                        case SDLK_s: chip.keypad[8] = 1; break;
                        case SDLK_d: chip.keypad[9] = 1; break;
                        case SDLK_z: chip.keypad[0xA] = 1; break;
                        case SDLK_c: chip.keypad[0xB] = 1; break;
                        case SDLK_4: chip.keypad[0xC] = 1; break;
                        case SDLK_r: chip.keypad[0xD] = 1; break;
                        case SDLK_f: chip.keypad[0xE] = 1; break;
                        case SDLK_v: chip.keypad[0xF] = 1; break;
                    }
                }
            if (event.type == SDL_KEYUP) {
                    switch (event.key.keysym.sym) {
                        case SDLK_x: chip.keypad[0] = 0; break;
                        case SDLK_1: chip.keypad[1] = 0; break;
                        case SDLK_2: chip.keypad[2] = 0; break;
                        case SDLK_3: chip.keypad[3] = 0; break;
                        case SDLK_q: chip.keypad[4] = 0; break;
                        case SDLK_w: chip.keypad[5] = 0; break;
                        case SDLK_e: chip.keypad[6] = 0; break;
                        case SDLK_a: chip.keypad[7] = 0; break;
                        case SDLK_s: chip.keypad[8] = 0; break;
                        case SDLK_d: chip.keypad[9] = 0; break;
                        case SDLK_z: chip.keypad[0xA] = 0; break;
                        case SDLK_c: chip.keypad[0xB] = 0; break;
                        case SDLK_4: chip.keypad[0xC] = 0; break;
                        case SDLK_r: chip.keypad[0xD] = 0; break;
                        case SDLK_f: chip.keypad[0xE] = 0; break;
                        case SDLK_v: chip.keypad[0xF] = 0; break;
                    }
                }
            }
            // EMULATION CYCLE
            for (int i = 0; i < 10; i++) {
                emulate(&chip);
            }

            if (chip.delay_timer > 0) chip.delay_timer--;
            if (chip.sound_timer > 0) chip.sound_timer--;

            if (chip.draw_flag) {
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
            
            chip.draw_flag = false;
            // SMALL DELAY TO PREVENT RUNNING AT 100% CPU
        }
        SDL_Delay(16);
    }

    // CLEANUP
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
