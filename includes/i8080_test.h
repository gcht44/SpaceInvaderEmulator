/*#ifndef i8080__H
#define i8080__H

#include <stdint.h>

typedef struct CPU CPU;

uint8_t  load_rom_test(CPU *cpu, char *filename);
void init_cpu_test(CPU *cpu);
void finish_exec();
int get_nb_intr();

#endif*/