#include "../includes/io.h"
#include "../includes/cpu8080.h"
#include "../includes/memory.h"

#include <stdio.h>

void write_io(CPU cpu, uint8_t port, uint8_t value)
{
    switch (port)
    {
    case 0x00:
        // finish_exec();
        return;
    case 0x01:
        if (cpu.c == 2)
        {
            printf("%c", cpu.e);
        }
        if (cpu.c == 9)
        {
            uint16_t addr = (cpu.d << 8) | cpu.e;
            uint16_t i = 0;
            uint8_t buff[256] = {0};
            while (read_memory(&cpu, addr+i) != '$')
            {
                buff[i] = read_memory(&cpu, addr+i);
                i++;
            }
            buff[i] = '\0';
            i = 0;
            // printf("ligne: %d lettre:\"", get_nb_intr());
            while (buff[i])
            {
                printf("%c", buff[i++]);
                printf("%c", value);
            }
            // printf("\"");
        }
        return;
    default:
        break;
    }
    return;
}

uint8_t read_io(uint8_t port)
{
    switch (port)
    {
    case 0x00:
        break;
    default:
        break;
    }
    return 0;
}