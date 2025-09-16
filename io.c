#include "includes/io.h"

#include <stdio.h>

void write_io(uint8_t port, uint8_t value)
{
    switch (port)
    {
    case 0x00:
        printf("%c", value);
        return;
    case 0x01:
        printf("%c", value);
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