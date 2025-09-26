#include "../includes/memory.h"
#include "../includes/cpu8080.h"
#include "../includes/i8080_test.h"

#include <stdio.h>
#include <stdlib.h>

static uint8_t ram[0x400] = {0};
static uint8_t vram[0x1C00] = {0};

int load_rom(CPU *cpu, const char path[])
{
    FILE *rom = fopen(path, "rb");
    if(!rom)
    {
        perror("Error fopen:");
        return -1;
    }
    fread(cpu->memory, sizeof(uint8_t) * (MEMORY_SIZE), 1, rom);
    fclose(rom);
    return 0;
}

uint8_t read_memory(CPU *cpu, uint16_t addr)
{
    if (addr >= 0x4000)
        addr = 0x2000 + ((addr - 0x2000) & 0x1FFF);

    if (addr < 0x2000)
        return cpu->memory[addr];
    else if (addr < 0x2400)
        return ram[addr - 0x2000];
    else if (addr < 0x4000)
        return vram[addr - 0x2400];
    else
    {
        printf("Out of range read (addr: %04X)", addr);
        exit(0);
    }
    return 0;
}

void write_memory(uint16_t addr, uint8_t value)
{
    if (addr >= 0x4000)
        addr = 0x2000 + ((addr - 0x2000) & 0x1FFF);
    
    if (addr < 0x2400)
        ram[addr - 0x2000] = value;
    else if (addr < 0x4000)
        vram[addr - 0x2400] = value;
    else
    {
        printf("Out of range write (addr: %04X)", addr);
        exit(0);
    }
}