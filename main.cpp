#include <iostream>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <array>
#include <fstream>
#include <chrono>
#include <random>
#include <stdexcept>
#include <SDL2/SDL.h>

const int VIDEO_WIDTH = 64;
const int VIDEO_HEIGHT = 32;

class chip8 {
public:
    uint8_t registers[16]{};
    uint8_t memory[4096]{};
    uint16_t index{};
    uint16_t pc{};
    uint8_t sp{};
    uint8_t delay_timer{};
    uint8_t sound_timer{};
    uint8_t keypad[16]{};
    uint32_t video[64 * 32]{};
    uint16_t opcode{};
    uint16_t stack[16]{};

    chip8();
    void loadROM(const char* filename);
    void cycle();

private:
    std::default_random_engine randGen;
    std::uniform_int_distribution<int> randByte;

    void Table0();
    void Table8();
    void TableE();
    void TableF();
    void OP_NULL();

    void OP_00E0(); void OP_00EE(); void OP_1nnn(); void OP_2nnn();
    void OP_3xkk(); void OP_4xkk(); void OP_5xy0(); void OP_6xkk();
    void OP_7xkk(); void OP_8xy0(); void OP_8xy1(); void OP_8xy2();
    void OP_8xy3(); void OP_8xy4(); void OP_8xy5(); void OP_8xy6();
    void OP_8xy7(); void OP_8xyE(); void OP_9xy0(); void OP_Annn();
    void OP_Bnnn(); void OP_Cxkk(); void OP_Dxyn(); void OP_Ex9E();
    void OP_ExA1(); void OP_Fx07(); void OP_Fx0A(); void OP_Fx15();
    void OP_Fx18(); void OP_Fx1E(); void OP_Fx29(); void OP_Fx33();
    void OP_Fx55(); void OP_Fx65();

    typedef void (chip8::*Chip8Func)();
    Chip8Func table[0xF + 1];
    Chip8Func table0[0xE + 1];
    Chip8Func table8[0xE + 1];
    Chip8Func tableE[0xE + 1];
    Chip8Func tableF[0x65 + 1];
};

const unsigned int START_ADDRESS = 0x200;
const unsigned int FONTSET_SIZE = 80;
const unsigned int FONTSET_START_ADDRESS = 0x50;
uint8_t chip8_fontset[FONTSET_SIZE] = {
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

chip8::chip8()
    : randGen(std::chrono::system_clock::now().time_since_epoch().count()),
      randByte(0, 255) {
    pc = START_ADDRESS;
    for (unsigned int i = 0; i < FONTSET_SIZE; ++i) {
        memory[FONTSET_START_ADDRESS + i] = chip8_fontset[i];
    }

    table[0x0] = &chip8::Table0;
    table[0x1] = &chip8::OP_1nnn;
    table[0x2] = &chip8::OP_2nnn;
    table[0x3] = &chip8::OP_3xkk;
    table[0x4] = &chip8::OP_4xkk;
    table[0x5] = &chip8::OP_5xy0;
    table[0x6] = &chip8::OP_6xkk;
    table[0x7] = &chip8::OP_7xkk;
    table[0x8] = &chip8::Table8;
    table[0x9] = &chip8::OP_9xy0;
    table[0xA] = &chip8::OP_Annn;
    table[0xB] = &chip8::OP_Bnnn;
    table[0xC] = &chip8::OP_Cxkk;
    table[0xD] = &chip8::OP_Dxyn;
    table[0xE] = &chip8::TableE;
    table[0xF] = &chip8::TableF;

    for (size_t i = 0; i <= 0xE; i++) {
        table0[i] = &chip8::OP_NULL;
        table8[i] = &chip8::OP_NULL;
        tableE[i] = &chip8::OP_NULL;
    }

    table0[0x0] = &chip8::OP_00E0;
    table0[0xE] = &chip8::OP_00EE;
    table8[0x0] = &chip8::OP_8xy0;
    table8[0x1] = &chip8::OP_8xy1;
    table8[0x2] = &chip8::OP_8xy2;
    table8[0x3] = &chip8::OP_8xy3;
    table8[0x4] = &chip8::OP_8xy4;
    table8[0x5] = &chip8::OP_8xy5;
    table8[0x6] = &chip8::OP_8xy6;
    table8[0x7] = &chip8::OP_8xy7;
    table8[0xE] = &chip8::OP_8xyE;
    tableE[0x1] = &chip8::OP_ExA1;
    tableE[0xE] = &chip8::OP_Ex9E;

    for (size_t i = 0; i <= 0x65; i++) {
        tableF[i] = &chip8::OP_NULL;
    }
    tableF[0x07] = &chip8::OP_Fx07;
    tableF[0x0A] = &chip8::OP_Fx0A;
    tableF[0x15] = &chip8::OP_Fx15;
    tableF[0x18] = &chip8::OP_Fx18;
    tableF[0x1E] = &chip8::OP_Fx1E;
    tableF[0x29] = &chip8::OP_Fx29;
    tableF[0x33] = &chip8::OP_Fx33;
    tableF[0x55] = &chip8::OP_Fx55;
    tableF[0x65] = &chip8::OP_Fx65;
}

void chip8::Table0() {
    ((*this).*(table0[opcode & 0x000F]))();
}
void chip8::Table8() {
    ((*this).*(table8[opcode & 0x000F]))();
}
void chip8::TableE() {
    ((*this).*(tableE[opcode & 0x000F]))();
}
void chip8::TableF() {
    ((*this).*(tableF[opcode & 0x00FF]))();
}
void chip8::OP_NULL() {
}

void chip8::cycle() {
    opcode = (memory[pc] << 8u) | memory[pc + 1];
    pc += 2;
    ((*this).*(table[(opcode & 0xF000u) >> 12u]))();
    if (delay_timer > 0) {
        --delay_timer;
    }
    if (sound_timer > 0) {
        --sound_timer;
    }
}

void chip8::loadROM(const char* filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open ROM file");
    }
    else {
        std::streampos size = file.tellg();
        char* buffer = new char[size];
        file.seekg(0, std::ios::beg);
        file.read(buffer, size);
        file.close();

        for (long i = 0; i < size; ++i) {
            memory[START_ADDRESS + i] = buffer[i];
        }
        delete[] buffer;
    }
}

void chip8::OP_00E0() {
    memset(video, 0, sizeof(video));
}

void chip8::OP_00EE() {
    --sp;
    pc = stack[sp];
}

void chip8::OP_1nnn() {
    uint16_t address = opcode & 0x0FFF;
    pc = address;
}

void chip8::OP_2nnn() {
    uint16_t address = opcode & 0x0FFF;
    stack[sp] = pc;
    ++sp;
    pc = address;
}

void chip8::OP_3xkk() {
    uint8_t Vx = (opcode & 0x0F00) >> 8;
    uint8_t byte = opcode & 0x00FF;
    if (registers[Vx] == byte) {
        pc += 2;
    }
}

void chip8::OP_4xkk() {
    uint8_t Vx = (opcode & 0x0F00) >> 8;
    uint8_t byte = opcode & 0x00FF;
    if (registers[Vx] != byte) {
        pc += 2;
    }
}

void chip8::OP_5xy0() {
    uint8_t Vx = (opcode & 0x0F00) >> 8;
    uint8_t Vy = (opcode & 0x00F0) >> 4;
    if (registers[Vx] == registers[Vy]) {
        pc += 2;
    }
}

void chip8::OP_6xkk() {
    uint8_t Vx = (opcode & 0x0F00) >> 8;
    uint8_t byte = opcode & 0x00FF;
    registers[Vx] = byte;
}

void chip8::OP_7xkk() {
    uint8_t Vx = (opcode & 0x0F00) >> 8;
    uint8_t byte = opcode & 0x00FF;
    registers[Vx] += byte;
}

void chip8::OP_8xy0() {
    uint8_t Vx = (opcode & 0x0F00) >> 8;
    uint8_t Vy = (opcode & 0x00F0) >> 4;
    registers[Vx] = registers[Vy];
}

void chip8::OP_8xy1() {
    uint8_t Vx = (opcode & 0x0F00) >> 8;
    uint8_t Vy = (opcode & 0x00F0) >> 4;
    registers[Vx] |= registers[Vy];
}

void chip8::OP_8xy2() {
    uint8_t Vx = (opcode & 0x0F00) >> 8;
    uint8_t Vy = (opcode & 0x00F0) >> 4;
    registers[Vx] &= registers[Vy];
}

void chip8::OP_8xy3() {
    uint8_t Vx = (opcode & 0x0F00) >> 8;
    uint8_t Vy = (opcode & 0x00F0) >> 4;
    registers[Vx] ^= registers[Vy];
}

void chip8::OP_8xy4() {
    uint8_t Vx = (opcode & 0x0F00) >> 8;
    uint8_t Vy = (opcode & 0x00F0) >> 4;
    uint16_t sum = registers[Vx] + registers[Vy];
    registers[Vx] = sum & 0xFF;
    registers[0xF] = (sum > 255) ? 1 : 0;
}

void chip8::OP_8xy5() {
    uint8_t Vx = (opcode & 0x0F00) >> 8;
    uint8_t Vy = (opcode & 0x00F0) >> 4;
    uint8_t flag = (registers[Vx] > registers[Vy]) ? 1 : 0;
    registers[Vx] -= registers[Vy];
    registers[0xF] = flag;
}

void chip8::OP_8xy6() {
    uint8_t Vx = (opcode & 0x0F00) >> 8;
    uint8_t flag = registers[Vx] & 0x1;
    registers[Vx] >>= 1;
    registers[0xF] = flag;
}

void chip8::OP_8xy7() {
    uint8_t Vx = (opcode & 0x0F00) >> 8;
    uint8_t Vy = (opcode & 0x00F0) >> 4;
    uint8_t flag = (registers[Vy] > registers[Vx]) ? 1 : 0;
    registers[Vx] = registers[Vy] - registers[Vx];
    registers[0xF] = flag;
}

void chip8::OP_8xyE() {
    uint8_t Vx = (opcode & 0x0F00) >> 8;
    uint8_t flag = (registers[Vx] & 0x80) >> 7;
    registers[Vx] <<= 1;
    registers[0xF] = flag;
}

void chip8::OP_9xy0() {
    uint8_t Vx = (opcode & 0x0F00) >> 8;
    uint8_t Vy = (opcode & 0x00F0) >> 4;
    if (registers[Vx] != registers[Vy]) {
        pc += 2;
    }
}

void chip8::OP_Annn() {
    uint16_t address = opcode & 0x0FFF;
    index = address;
}

void chip8::OP_Bnnn() {
    uint16_t address = opcode & 0x0FFF;
    pc = registers[0] + address;
}

void chip8::OP_Cxkk() {
    uint8_t Vx = (opcode & 0x0F00) >> 8;
    uint8_t byte = opcode & 0x00FF;
    registers[Vx] = randByte(randGen) & byte;
}

void chip8::OP_Dxyn() {
    uint8_t Vx = (opcode & 0x0F00) >> 8;
    uint8_t Vy = (opcode & 0x00F0) >> 4;
    uint8_t height = opcode & 0x000F;

    registers[0xF] = 0;

    for (unsigned int row = 0; row < height; ++row) {
        uint8_t spriteByte = memory[index + row];
        for (unsigned int col = 0; col < 8; ++col) {
            if ((spriteByte & (0x80 >> col)) != 0) {
                uint16_t pixelIndex = ((registers[Vy] + row) % 32) * 64 + ((registers[Vx] + col) % 64);
                if (video[pixelIndex] == 0xFFFFFFFF) {
                    registers[0xF] = 1;
                }
                video[pixelIndex] ^= 0xFFFFFFFF;
            }
        }
    }
}

void chip8::OP_Ex9E() {
    uint8_t Vx = (opcode & 0x0F00) >> 8;
    if (keypad[registers[Vx]] != 0) {
        pc += 2;
    }
}

void chip8::OP_ExA1() {
    uint8_t Vx = (opcode & 0x0F00) >> 8;
    if (keypad[registers[Vx]] == 0) {
        pc += 2;
    }
}

void chip8::OP_Fx07() {
    uint8_t Vx = (opcode & 0x0F00) >> 8;
    registers[Vx] = delay_timer;
}

void chip8::OP_Fx0A() {
    uint8_t Vx = (opcode & 0x0F00) >> 8;
    for (uint8_t i = 0; i < 16; ++i) {
        if (keypad[i] != 0) {
            registers[Vx] = i;
            return;
        }
    }
    pc -= 2;
}

void chip8::OP_Fx15() {
    uint8_t Vx = (opcode & 0x0F00) >> 8;
    delay_timer = registers[Vx];
}

void chip8::OP_Fx18() {
    uint8_t Vx = (opcode & 0x0F00) >> 8;
    sound_timer = registers[Vx];
}

void chip8::OP_Fx1E() {
    uint8_t Vx = (opcode & 0x0F00) >> 8;
    index += registers[Vx];
}

void chip8::OP_Fx29() {
    uint8_t Vx = (opcode & 0x0F00) >> 8;
    index = FONTSET_START_ADDRESS + (registers[Vx] * 5);
}

void chip8::OP_Fx33() {
    uint8_t Vx = (opcode & 0x0F00) >> 8;
    uint8_t value = registers[Vx];
    memory[index] = value / 100;
    memory[index + 1] = (value / 10) % 10;
    memory[index + 2] = value % 10;
}

void chip8::OP_Fx55() {
    uint8_t Vx = (opcode & 0x0F00) >> 8;
    for (uint8_t i = 0; i <= Vx; ++i) {
        memory[index + i] = registers[i];
    }
}

void chip8::OP_Fx65() {
    uint8_t Vx = (opcode & 0x0F00) >> 8;
    for (uint8_t i = 0; i <= Vx; ++i) {
        registers[i] = memory[index + i];
    }
}

class Platform {
public:
    Platform(const char* title, int windowWidth, int windowHeight, int textureWidth, int textureHeight) {
        SDL_Init(SDL_INIT_VIDEO);
        window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, windowWidth, windowHeight, SDL_WINDOW_SHOWN);
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, textureWidth, textureHeight);
    }

    ~Platform() {
        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
    }

    void update(const void* buffer, int pitch) {
        SDL_UpdateTexture(texture, nullptr, buffer, pitch);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, nullptr, nullptr);
        SDL_RenderPresent(renderer);
    }

    // returns true when the user wants to quit
    bool processInput(uint8_t* keypad) {
        bool quit = false;
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                quit = true;
            }
            else if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
                uint8_t isKeyDown = (event.type == SDL_KEYDOWN) ? 1 : 0;
                switch (event.key.keysym.sym) {
                    case SDLK_ESCAPE: quit = true; break;
                    case SDLK_x: keypad[0x0] = isKeyDown; break;
                    case SDLK_1: keypad[0x1] = isKeyDown; break;
                    case SDLK_2: keypad[0x2] = isKeyDown; break;
                    case SDLK_3: keypad[0x3] = isKeyDown; break;
                    case SDLK_q: keypad[0x4] = isKeyDown; break;
                    case SDLK_w: keypad[0x5] = isKeyDown; break;
                    case SDLK_e: keypad[0x6] = isKeyDown; break;
                    case SDLK_a: keypad[0x7] = isKeyDown; break;
                    case SDLK_s: keypad[0x8] = isKeyDown; break;
                    case SDLK_d: keypad[0x9] = isKeyDown; break;
                    case SDLK_z: keypad[0xA] = isKeyDown; break;
                    case SDLK_c: keypad[0xB] = isKeyDown; break;
                    case SDLK_4: keypad[0xC] = isKeyDown; break;
                    case SDLK_r: keypad[0xD] = isKeyDown; break;
                    case SDLK_f: keypad[0xE] = isKeyDown; break;
                    case SDLK_v: keypad[0xF] = isKeyDown; break;
                }
            }
        }
        return quit;
    }

private:
    SDL_Window* window{};
    SDL_Renderer* renderer{};
    SDL_Texture* texture{};
};

int main(int argc, char** argv) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <Scale> <Delay> <ROM>" << std::endl;
        std::exit(EXIT_FAILURE);
    }
    int videoScale = std::stoi(argv[1]);
    int cycleDelay = std::stoi(argv[2]);
    char const* romFilename = argv[3];

    Platform platform("CHIP-8 Emulator", VIDEO_WIDTH * videoScale, VIDEO_HEIGHT * videoScale, VIDEO_WIDTH, VIDEO_HEIGHT);
    chip8 emu;
    emu.loadROM(romFilename);
    int videoPitch = sizeof(emu.video[0]) * VIDEO_WIDTH;
    auto lastCycleTime = std::chrono::high_resolution_clock::now();
    bool quit = false;

    while (!quit) {
        quit = platform.processInput(emu.keypad);
        auto currentTime = std::chrono::high_resolution_clock::now();
        auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastCycleTime).count();
        if (elapsedTime >= cycleDelay) {
            lastCycleTime = currentTime;
            emu.cycle();
            platform.update(emu.video, videoPitch);
        }
    }
    return 0;
}