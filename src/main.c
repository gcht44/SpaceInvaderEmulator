#include <stdio.h>
#include <stdlib.h>
#include "../includes/cpu8080.h"
#include "../includes/memory.h"
#include "../includes/video.h"

void print_opcode(CPU *cpu, int cyc)
{
    // uint8_t opcode = cpu->memory[cpu->pc];
    printf("PC: %04X, AF: %02X%02X, BC: %02X%02X, DE: %02X%02X, HL: %02X%02X, SP: %04X, CYC: %d (%02X %02X %02X %02X)\n", 
        cpu->pc, cpu->a, get_f_flags(cpu) ,cpu->b, cpu->c, cpu->d, cpu->e,
        cpu->h, cpu->l, cpu->sp, cyc,read_memory(cpu, cpu->pc), read_memory(cpu, cpu->pc+1), read_memory(cpu, cpu->pc+2), 
        read_memory(cpu, cpu->pc+3));
}

int main(void)
{
    CPU cpu;
    bool play_emu = true;

    init_cpu(&cpu);
    printf("Le CPU a bien été initialisé\n");
    load_rom(&cpu, "rom/invaders.rom");
    printf("La ROM a bien été chargé\n");

    /*int i = 0;
    while (i < MEMORY_SIZE)
    {
        printf("%02X ", cpu.memory[i++]);
        if (i % 16 == 0) printf("\n");
    }*/

    init_sdl();
    while (play_emu)
    {
        SDL_Event e;    
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) exit(0);
            if (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_ESCAPE) exit(0);
        }
        // print_opcode(&cpu, get_cyc());
        step_emu(&cpu);
    }

    return 0;
}