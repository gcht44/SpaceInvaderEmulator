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

    uint8_t memory[0xFFFF];

    bool halted;
    bool interrupt_enable; // Interrupt OK si true et interrupt_pending true
    bool interrupt_pending; // Un périphérique demande une interrupt
    bool ei_pending; // Dans ei pour signifier qu'après on pourra executer une interrupt
    uint8_t interrupt_vector; // Variable contenant l'opcode que l'on veut executer pendant L'interrupt souvetn RST 
} CPU;

int get_cyc();

void init_cpu(CPU *cpu);
void check_condition_bits(CPU *cpu, bool z, bool c, bool p, bool s, uint16_t data);
uint8_t get_f_flags(CPU *cpu);

int execute(CPU *cpu, uint8_t opcode);
void step_emu(CPU *cpu);

void ask_interrupt(CPU *cpu, uint8_t opcode);

void call(CPU *cpu, uint16_t addr);
void rst(CPU *cpu, uint8_t i);
void ret(CPU *cpu);

#endif