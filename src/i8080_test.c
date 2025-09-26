#include "includes/i8080_test.h"
#include "includes/cpu8080.h"
#include "includes/memory.h"
#include "includes/utils.h"

#include <stdio.h>
#include <string.h>

static bool test_finished = false;
static int nb_instr = 0;

int get_nb_intr()
{
    return nb_instr;
}

uint8_t load_rom_test(CPU *cpu, char *filename)
{
    FILE *rom = fopen(filename, "rb");
    if(!rom)
    {
        perror("Error fopen:");
        return -1;
    }
    fread(&cpu->memory[0x100], sizeof(uint8_t) * (0x4B00), 1, rom); // Mettre la bonne taille
    fclose(rom);
    return 0;
}

void init_cpu_test(CPU *cpu)
{
    cpu->pc = 0x0100;
    cpu->sp = 0x0000;
    memset(cpu->memory, 0, sizeof(cpu->memory));

    cpu->a = 0;
    cpu->b = 0;
    cpu->c = 0;
    cpu->d = 0;
    cpu->e = 0;
    cpu->h = 0;
    cpu->l = 0;

    cpu->z = 0;
    cpu->c = 0;
    cpu->ac = 0;
    cpu->p = 0;
    cpu->s = 0;

    cpu->halted = false;
    cpu->interrupt_enable = false;
}

void finish_exec()
{
    test_finished = true;
}

void print_opcode(CPU *cpu, int cyc)
{
    uint8_t opcode = cpu->memory[cpu->pc];
    printf("PC: %04X, AF: %02X%02X, BC: %02X%02X, DE: %02X%02X, HL: %02X%02X, SP: %04X, CYC: %d (%02X %02X %02X %02X)\n", 
        cpu->pc, cpu->a, get_f_flags(cpu) ,cpu->b, cpu->c, cpu->d, cpu->e,
        cpu->h, cpu->l, cpu->sp, cyc,read_memory(cpu, cpu->pc), read_memory(cpu, cpu->pc+1), read_memory(cpu, cpu->pc+2), 
        read_memory(cpu, cpu->pc+3));
}

int main(void)
{
    CPU cpu;
    init_cpu_test(&cpu);
    // load_rom_test(&cpu, "rom/test_rom/8080PRE.COM");
    load_rom_test(&cpu, "rom/test_rom/8080EXM.COM");
    
    // inject "out 0,a" at 0x0000 (signal to stop the test)
    cpu.memory[0x0000] = 0xD3;
    cpu.memory[0x0001] = 0x00;

    // inject "out 1,a" at 0x0005 (signal to output some characters)
    cpu.memory[0x0005] = 0xD3;
    cpu.memory[0x0006] = 0x01;
    cpu.memory[0x0007] = 0xC9;

    // Test for debug SUB flag F
    /*cpu.memory[0x0100] = 0x3A; // LDA A, a16
    cpu.memory[0x0101] = 0x90;
    cpu.memory[0x0102] = 0x00;
    cpu.memory[0x0103] = 0x06; // MVI B, d8
    cpu.memory[0x0104] = 0x16;
    cpu.memory[0x0105] = 0xB8; // CMP B
    cpu.memory[0x0106] = 0xCD; // CALL (EXIT)
    cpu.memory[0x0107] = 0x00;
    cpu.memory[0x0108] = 0x00; 

    // Test number
    cpu.memory[0x0090] = 0x06; // For A
    cpu.memory[0x0091] = 0x01;*/

    int cyc = 0;
    while (!test_finished)
    {
        if (nb_instr == 2919050698)
            test_finished = 1;
        if (nb_instr > 210538394)
            print_opcode(&cpu, cyc);
        cyc += step_emu(&cpu);
        nb_instr++;
        // printf("%d\n", cyc);
    }
    printf("%d instructions executed on %d cycles\n", nb_instr, cyc);
    return 0;
}