#ifndef MEMORY__H
#define MEMORY__H

#define MEMORY_SIZE 0x2000

#include <stdint.h>

typedef struct CPU CPU;

int load_rom(CPU *cpu, const char path[]);
uint8_t read_memory(CPU *cpu, uint16_t addr);
void write_memory(CPU *cpu, uint16_t addr, uint8_t value);

#endif