# CHIP-8 Emulator in C
A functional CHIP-8 interpreter written from scratch in C, using SDL2 for graphics and input handling. This emulator supports standard CHIP-8 ROMs and implements legacy behaviors to ensure compatibility with classic games like Pong and Space Invaders.

## Features 
- Full Opcode Support: Implements all 35 standard CHIP-8 opcodes
- Graphics: 64x32 monochorme display rendering via SDL2
- Input: Hex keypad mapped to standard QWERTY keyboard
- Timers: Functional Delay and sound timers(60Hz)
- Quirk Handling: Implements legacy memory behavior to support older ROMs


## Dependencies
### macOS (Homebrew)
``` brew install sdl2```
### Linux(Debian/Ubuntu)
``` sudo apt-get install libsdl2-dev ```
### Windows
You will need to set up MinGW or Visual Studio with SDL2 libraraies linked

## Building and Running:
- To compile the code, using `clang` or `gcc`
```clang -Wall -Wextra chip8.c -lSDL2 -o chip8```
- To run a ROM, provide the path to a ROM file as an argument
```./chip8 roms/rom_name.ch8```

## Controls
The original CHIP-8 used a 16-key hex keypad. This maps them to the left side of your keyboard:
| CHIP-8 Keypad | Your Keyboard |
| :---: | :---: |
| **1** **2** **3** **C** | **1** **2** **3** **4** |
| **4** **5** **6** **D** | **Q** **W** **E** **R** |
| **7** **8** **9** **E** | **A** **S** **D** **F** |
| **A** **0** **B** **F** | **Z** **X** **C** **V** |
