#ifndef CPU_8080__H
#define CPU_8080__H

#include <stdint.h>
#include <stdbool.h>

typedef struct CPU
{
    uint8_t a;
    uint8_t b;
    uint8_t c;
    uint8_t d;
    uint8_t e;
    uint8_t h;
    uint8_t l;

    uint16_t pc;
    uint16_t sp;

    bool z;
    bool cy;
    bool ac;
    bool p;
    bool s;

    char memory[0x2000];

    bool halted;
    bool interrupt_enable;
} CPU;

void init_cpu(CPU *cpu);
void check_condition_bits(CPU *cpu, bool z, bool c, bool p, bool s, uint16_t data);
void print_opcode(CPU *cpu);
int execute(CPU *cpu);

void call(CPU *cpu, uint16_t addr);
void rst(CPU *cpu, uint8_t i);
void ret(CPU *cpu);

#endif