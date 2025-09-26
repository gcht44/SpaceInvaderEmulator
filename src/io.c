#include "../includes/io.h"
#include "../includes/cpu8080.h"
#include "../includes/memory.h"
#include "../includes/video.h"

#include <stdio.h>


/*
INPUTS (readio)
Port 0
 bit 0 DIP4 (Seems to be self-test-request read at power up)
 bit 1 Always 1
 bit 2 Always 1
 bit 3 Always 1
 bit 4 Fire
 bit 5 Left
 bit 6 Right
 bit 7 ? tied to demux port 7 ?

Port 1
 bit 0 = CREDIT (1 if deposit)
 bit 1 = 2P start (1 if pressed)
 bit 2 = 1P start (1 if pressed)
 bit 3 = Always 1
 bit 4 = 1P shot (1 if pressed)
 bit 5 = 1P left (1 if pressed)
 bit 6 = 1P right (1 if pressed)
 bit 7 = Not connected
 
Port 2
 bit 0 = DIP3 00 = 3 ships  10 = 5 ships
 bit 1 = DIP5 01 = 4 ships  11 = 6 ships
 bit 2 = Tilt
 bit 3 = DIP6 0 = extra ship at 1500, 1 = extra ship at 1000
 bit 4 = P2 shot (1 if pressed)
 bit 5 = P2 left (1 if pressed)
 bit 6 = P2 right (1 if pressed)
 bit 7 = DIP7 Coin info displayed in demo screen 0=ON

Port 3
  bit 0-7 Shift register data 
 */


static uint16_t bits_reg;
static uint8_t shift_amount;

// Port 1
static uint8_t is_coin;
static uint8_t is_one_p_shoot;
static uint8_t is_one_p_left;
static uint8_t is_one_p_right;
static uint8_t is_one_p_start;
static uint8_t is_two_p_start;

static uint8_t is_two_p_shoot;
static uint8_t is_two_p_left;
static uint8_t is_two_p_right;
// Port 2


void keyboard_to_io(IO_Def iod, uint8_t value)
{
    switch (iod)
    {
    case COIN: // Keyboard C
        is_coin = value;
        break;
    case ONE_P_SHOOT: // Keyboard  Z
        is_one_p_shoot = value; 
        break;
    case ONE_P_LEFT: // Keyboard Q
        is_one_p_left = value;
        break;
    case ONE_P_RIGHT: // Keyboard D
        is_one_p_right = value;
        break;
    case ONE_P_START: // Keyboard R
        is_one_p_start = value;
        break;
    case TWO_P_START: // Keyboard T
        is_two_p_start = value;
        break;

    case TWO_P_SHOOT: // Keyboard  Space
        is_two_p_shoot = value; 
        break;
    case TWO_P_LEFT: // Keyboard <
        is_two_p_left = value;
        break;
    case TWO_P_RIGHT: // Keyboard >
        is_two_p_right = value;
        break;
    default:
        break;
    }
    // SDL_Log("ONE_P_SHOOT: %d\n", is_one_p_shoot);
}

void write_io(CPU cpu, uint8_t port, uint8_t value)
{
    switch (port)
    {
    /*case 0x00:
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
        }*/

        case 2:
            shift_amount = value & 7;
            return;
        case 4:
            bits_reg = (value << 8) | (bits_reg >> 8);
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
        case 0:
            return 0x0F;
        case 1:
            return (0 << 7) | (is_one_p_right << 6) | (is_one_p_left << 5) | (is_one_p_shoot << 4 ) | (1 << 3) | (is_one_p_start << 2) | (is_two_p_start << 1) | is_coin;
        case 2:
            return (0 << 7) | (is_two_p_right << 6) | (is_two_p_left << 5) | (is_two_p_shoot << 4) | (1 << 3) | (0 << 2) | (0 << 1) | 0;
        case 3:
            return (bits_reg >> shift_amount) & 0xFF;
    default:
        break;
    }
    return 0;
}