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