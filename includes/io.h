#ifndef IO__H
#define IO__H

#include <stdint.h>

// typedef struct CPU CPU;

void write_io(uint8_t port, uint8_t value);
uint8_t read_io(uint8_t port);

#endif