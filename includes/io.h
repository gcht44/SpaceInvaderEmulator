#ifndef IO__H
#define IO__H

#include <stdint.h>

typedef struct CPU CPU;

typedef enum
{
    COIN,
    ONE_P_SHOOT,
    ONE_P_LEFT,
    ONE_P_RIGHT,
    ONE_P_START,
    TWO_P_START,

    TWO_P_SHOOT,
    TWO_P_LEFT,
    TWO_P_RIGHT,
} IO_Def;

void write_io(CPU cpu, uint8_t port, uint8_t value);
uint8_t read_io(uint8_t port);
void keyboard_to_io(IO_Def iod, uint8_t value);

#endif