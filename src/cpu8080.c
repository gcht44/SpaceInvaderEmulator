#include "../includes/cpu8080.h"
#include "../includes/utils.h"
#include "../includes/memory.h"
#include "../includes/io.h"
#include "../includes/video.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static int cyc = 0;
static int totcyc = 0;
static bool mid_int = true;

int get_cyc()
{
    return totcyc;
}

void init_cpu(CPU *cpu)
{
    memset(cpu->memory, 0, sizeof(cpu->memory));

    cpu->pc = 0;
    cpu->sp = 0x2400;
    
    cpu->a = 0;
    cpu->b = 0;
    cpu->c = 0;
    cpu->d = 0;
    cpu->e = 0;
    cpu->h = 0;
    cpu->l = 0;

    cpu->z = 0;
    cpu->c = 0;
    cpu->ac = 0;
    cpu->p = 0;
    cpu->s = 0;

    cpu->halted = false;
    cpu->interrupt_enable = false;
    cpu->interrupt_pending = false;
    cpu->ei_pending = false;
}

void fill_frame_buffer(CPU *cpu, uint32_t *frameBuffer) 
{
    // Space Invaders utilise 224x256 (WxH)
    // VRAM commence à 0x2400
    for (int i = 0; i < 0x1C00; i++)  // 0x1C00 = taille de la VRAM 
    {
        uint8_t current_byte = read_memory(cpu, 0x2400 + i);
        
        // Calcul des coordonnées après rotation
        int x = i / 32;        // 32 octets par ligne
        int y = 255 - (i % 32 * 8); // 8 pixels par octet
        
        // Pour chaque bit dans l'octet
        for (int bit = 0; bit < 8; bit++) 
        {
            // Si le bit est à 1, pixel blanc, sinon noir
            uint32_t color = (current_byte & (1 << bit)) ? 0xFFFFFFFFu : 0xFF000000u;
            
            // Position dans le framebuffer
            if ((y - bit) >= 0 && x < W) {
                frameBuffer[x + ((y - bit) * W)] = color;
            }
        }
    }
}

void step_emu(CPU *cpu)
{
    int temp_cyc = 0;
    if (cpu->interrupt_enable && cpu->interrupt_pending && (cpu->ei_pending == 0))
    {
        cpu->interrupt_pending = 0;
        cpu->halted = 0;
        
        cpu->interrupt_enable = 0;
        
        temp_cyc = execute(cpu, cpu->interrupt_vector);
        cyc += temp_cyc;
        totcyc += temp_cyc;
    } 
    else if (!cpu->halted)
    {
        temp_cyc = execute(cpu, read_memory(cpu, cpu->pc++));
        cyc += temp_cyc;
        totcyc += temp_cyc;
    }
    if (cyc >= 33333)
    {
        mid_int = true;
        cyc -= 33333;
        if (cpu->interrupt_enable)
        {
            ask_interrupt(cpu, 0xD7);
            // 1) convertir VRAM -> framebuffer
            uint32_t fb[W*H];
            fill_frame_buffer(cpu, fb);
            // 2) dessiner à l’écran
            draw_pixels(fb);
            // exit(0);
        }
    }
    else if ((cyc >= 16667) && mid_int)
    {
        mid_int = false;
        if (cpu->interrupt_enable)
        {
            ask_interrupt(cpu, 0xCF);
        }
    }
}

// demander une interrupt (pour les périphérique)
void ask_interrupt(CPU *cpu, uint8_t opcode) {
  cpu->interrupt_pending = 1;
  cpu->interrupt_vector = opcode;
}

// Gérer le flag ac indépendament dans l'instruction
void check_condition_bits(CPU *cpu, bool s, bool z, bool p, bool c, uint16_t data)
{
    if (z)
        cpu->z = ((data & 0xFF) == 0);
    if (c)
    {
        cpu->cy = ((data & 0xFF00) > 0xFF);
    }
    if (s)
        cpu->s = ((data & 0x80) != 0);
    if (p)
    {
        int nb_bits_set_to_one = 0;
        uint8_t temp_data = (uint8_t)(data & 0xFF);
        for (int i=0 ; i < 8 ; i++)
        {
            nb_bits_set_to_one += temp_data & 0x1;
            temp_data >>= 1;
        }
        cpu->p = ((nb_bits_set_to_one % 2) == 0);
    }
}

uint8_t get_f_flags(CPU *cpu)
{
    return (cpu->s << 7) | (cpu->z << 6) | (0 << 5) | (cpu->ac << 4) | (0 << 3) | (cpu->p << 2) | (1 << 1) | (cpu->cy);
}

int execute(CPU *cpu, uint8_t opcode)
{
    // print_opcode(cpu);
    if (cpu->ei_pending)
    {
        cpu->ei_pending = 0;
        cpu->interrupt_enable = true;
    }


    uint8_t hi;
    uint8_t lo;
    uint16_t res_16bits;
    uint32_t res_32bits;
    uint16_t addr;
    int16_t res_signed;
    bool new_carry;
    switch (opcode)
    {
        case 0x00: // NOP
            return 4; // return le temps en cycles
        case 0x01: // LXI B, d16
            cpu->c = read_memory(cpu, cpu->pc++);
            cpu->b = read_memory(cpu, cpu->pc++);
            return 10;
        case 0x02: // STAX B
            write_memory(((cpu->b << 8) | cpu->c), cpu->a);
            return 7;
        case 0x03: // INX B
            cpu->c++;
            if (cpu->c == 0)
                cpu->b++;
            return 5;
        case 0x04: // INR B
            res_16bits = cpu->b + 1;
            // for Auxiliary carry
            cpu->ac = (((cpu->b & 0xF) + 1) > 0xF);
            check_condition_bits(cpu, 1, 1, 1, 0, res_16bits);
            cpu->b = (uint8_t)(res_16bits & 0xFF);
            return 5;
        case 0x05: // DCR B
            lo = cpu->b - 1;
            // for Auxiliary carry
            cpu->ac = !((lo & 0xF) == 0xF);
            check_condition_bits(cpu, 1, 1, 1, 0, lo);
            cpu->b = lo;
            return 5;
        case 0x06: // MVI B, d8
            lo = read_memory(cpu, cpu->pc++);
            cpu->b = lo;
            return 7;
        case 0x07: // RLC
            cpu->cy = (cpu->a & 0x80) >> 7;
            cpu->a <<= 1;
            cpu->a |= cpu->cy;
            return 4;
        case 0x08: // NOP
            return 4;
        case 0x09: // DAD B
            res_32bits = ((cpu->b << 8) | cpu->c) + ((cpu->h << 8) | cpu->l);
            cpu->cy = (res_32bits > 0xFFFF); // Carry for 32 bits
            cpu->h = (uint8_t)((res_32bits & 0xFF00) >> 8);
            cpu->l = (uint8_t)(res_32bits & 0x00FF);
            return 10;
        case 0x0A: // LDAX B
            cpu->a = read_memory(cpu, ((cpu->b << 8) | cpu->c));
            return 7;
        case 0x0B: // DCX B
            cpu->c--;
            if (cpu->c == 0xFF)
                cpu->b--;
            return 5;
        case 0x0C: // INR C
            res_16bits = cpu->c + 1;
            // for Auxiliary carry
            cpu->ac = (((cpu->c & 0xF) + 1) > 0xF);
            check_condition_bits(cpu, 1, 1, 1, 0, res_16bits);
            cpu->c = (uint8_t)(res_16bits & 0xFF);
            return 5;
        case 0x0D: // DCR C
            lo = cpu->c - 1;
            // for Auxiliary carry
            cpu->ac = !((lo & 0xF) == 0xF);
            check_condition_bits(cpu, 1, 1, 1, 0, lo);
            cpu->c = lo;
            return 5;
        case 0x0E: // MVI C, d8
            lo = read_memory(cpu, cpu->pc++);
            cpu->c = lo;
            return 7;
        case 0x0F: // RRC
            cpu->cy = (cpu->a & 0x1);
            cpu->a >>= 1;
            cpu->a |= cpu->cy << 7;
            return 4;
        case 0x10: // NOP
            return 4;
        case 0x11: // LXI D, d16
            cpu->e = read_memory(cpu, cpu->pc++);
            cpu->d = read_memory(cpu, cpu->pc++);
            return 10;
        case 0x12: // STAX D
            write_memory(((cpu->d << 8) | cpu->e), cpu->a);
            return 7;
        case 0x13: // INX D
            cpu->e++;
            if (cpu->e == 0)
                cpu->d++;
            return 5;
        case 0x14: // INR D
            res_16bits = cpu->d + 1;
            // for Auxiliary carry
            cpu->ac = (((cpu->d & 0xF) + 1) > 0xF);
            check_condition_bits(cpu, 1, 1, 1, 0, res_16bits);
            cpu->d = (uint8_t)(res_16bits & 0xFF);
            return 5;
        case 0x15: // DCR D
            lo = cpu->d - 1;
            // for Auxiliary carry
            cpu->ac = !((lo & 0xF) == 0xF);
            check_condition_bits(cpu, 1, 1, 1, 0, lo);
            cpu->d = lo;
            return 5;
        case 0x16: // MVI D, d8
            lo = read_memory(cpu, cpu->pc++);
            cpu->d = lo;
            return 7;
        case 0x17: // RAL
            new_carry = ((cpu->a & 0x80) == 0x80);
            cpu->a = ((cpu->a << 1) | cpu->cy);
            cpu->cy = new_carry;
            return 4;
        case 0x18: // NOP
            return 4;
        case 0x19: // DAD D
            res_32bits = ((cpu->d << 8) | cpu->e) + ((cpu->h << 8) | cpu->l);
            cpu->cy = (res_32bits > 0xFFFF); // Carry for 32 bits
            cpu->h = (uint8_t)((res_32bits & 0xFF00) >> 8);
            cpu->l = (uint8_t)(res_32bits & 0x00FF);
            return 10;
        case 0x1A: // LDAX D
            cpu->a = read_memory(cpu, ((cpu->d << 8) | cpu->e));
            return 7;
        case 0x1B: // DCX D
            cpu->e--;
            if (cpu->e == 0xFF)
                cpu->d--;
            return 5;
        case 0x1C: // INR E
            res_16bits = cpu->e + 1;
            // for Auxiliary carry
            cpu->ac = (((cpu->e & 0xF) + 1) > 0xF);
            check_condition_bits(cpu, 1, 1, 1, 0, res_16bits);
            cpu->e = (uint8_t)(res_16bits & 0xFF);
            return 5;
        case 0x1D: // DCR E
            lo = cpu->e - 1;
            // for Auxiliary carry
            cpu->ac = !((lo & 0xF) == 0xF);
            check_condition_bits(cpu, 1, 1, 1, 0, lo);
            cpu->e = lo;
            return 5;
        case 0x1E: // MVI E, d8
            lo = read_memory(cpu, cpu->pc++);
            cpu->e = lo;
            return 7;
        case 0x1F: // RAR
            new_carry = (cpu->a & 0x1);
            cpu->a = ((cpu->a >> 1) | (cpu->cy << 7));
            cpu->cy = new_carry;
            return 4;
        case 0x20: // NOP
            return 4;
        case 0x21: // LXI H, d16
            cpu->l = read_memory(cpu, cpu->pc++);
            cpu->h = read_memory(cpu, cpu->pc++);
            return 10;
        case 0x22: // SHLD a16
            lo = read_memory(cpu, cpu->pc++);
            hi = read_memory(cpu, cpu->pc++);
            addr = ((hi << 8) | lo);
            write_memory(addr, cpu->l);
            write_memory(addr+1, cpu->h);
            return 16;
        case 0x23: // INX H
            cpu->l++;
            if (cpu->l == 0)
                cpu->h++;
            return 5;
        case 0x24: // INR H
            res_16bits = cpu->h + 1;
            // for Auxiliary carry
            cpu->ac = (((cpu->h & 0xF) + 1) > 0xF);
            check_condition_bits(cpu, 1, 1, 1, 0, res_16bits);
            cpu->h = (uint8_t)(res_16bits & 0xFF);
            return 5;
        case 0x25: // DCR H
            lo = cpu->h - 1;
            // for Auxiliary carry
            cpu->ac = !((lo & 0xF) == 0xF);
            check_condition_bits(cpu, 1, 1, 1, 0, lo);
            cpu->h = lo;
            return 5;
        case 0x26: // MVI H, d8
            lo = read_memory(cpu, cpu->pc++);
            cpu->h = lo;
            return 7;
        case 0x27:; // DAA
            uint8_t add6 = (((cpu->a & 0x0F) > 9) || cpu->ac);
            uint8_t add60 = ((cpu->a > 0x99) || cpu->cy);

            if (add6)
            {
                cpu->ac = (((cpu->a & 0x0F) + 0x06) > 0x0F);
                cpu->a += 0x06;
            }
            if (add60)
            {
                cpu->a += 0x60;
                cpu->cy = 1;
            }
            check_condition_bits(cpu, 1, 1, 1, 0, cpu->a);
            return 4;
        case 0x28: // NOP
            return 4;
        case 0x29: // DAD H
            res_32bits = (((cpu->h << 8) | cpu->l) << 1);
            cpu->cy = (res_32bits > 0xFFFF); // Carry for 32 bits
            cpu->h = (uint8_t)((res_32bits & 0xFF00) >> 8);
            cpu->l = (uint8_t)(res_32bits & 0x00FF);
            return 10;
        case 0x2A: // LHLD a16
            lo = read_memory(cpu, cpu->pc++);
            hi = read_memory(cpu, cpu->pc++);
            addr = ((hi << 8) | lo);
            cpu->l = read_memory(cpu, addr);
            cpu->h = read_memory(cpu, addr+1);
            return 16;
        case 0x2B: // DCX H
            cpu->l--;
            if (cpu->l == 0xFF)
                cpu->h--;
            return 5;
        case 0x2C: // INR L
            res_16bits = cpu->l + 1;
            // for Auxiliary carry
            cpu->ac = (((cpu->l & 0xF) + 1) > 0xF);
            check_condition_bits(cpu, 1, 1, 1, 0, res_16bits);
            cpu->l = (uint8_t)(res_16bits & 0xFF);
            return 5;
        case 0x2D: // DCR L
            lo = cpu->l - 1;
            // for Auxiliary carry
            cpu->ac = !((lo & 0xF) == 0xF);
            check_condition_bits(cpu, 1, 1, 1, 0, lo);
            cpu->l = lo;
            return 5;
        case 0x2E: // MVI L, d8
            lo = read_memory(cpu, cpu->pc++);
            cpu->l = lo;
            return 7;
        case 0x2F: // CMA
            cpu->a ^= 0xFF;
            return 4;
        case 0x30: // NOP
            return 4;
        case 0x31: // LXI SP, d16
            lo = read_memory(cpu, cpu->pc++);
            hi = read_memory(cpu, cpu->pc++);
            cpu->sp = (uint16_t)((hi << 8) | lo);
            return 10;
        case 0x32: // STA a16
            lo = read_memory(cpu, cpu->pc++);
            hi = read_memory(cpu, cpu->pc++);
            addr = ((hi << 8) | lo);
            write_memory(addr, cpu->a);
            return 13;
        case 0x33: // INX SP
            cpu->sp++;
            return 5;
        case 0x34: // INR M
            res_16bits = read_memory(cpu, ((cpu->h << 8) | cpu->l)) + 1;
            // for Auxiliary carry
            cpu->ac = (((read_memory(cpu, ((cpu->h << 8) | cpu->l)) & 0xF) + 1) > 0xF);
            check_condition_bits(cpu, 1, 1, 1, 0, res_16bits);
            write_memory(((cpu->h << 8) | cpu->l), (uint8_t)(res_16bits & 0xFF));
            return 10;
        case 0x35: // DCR M
            lo = read_memory(cpu, ((cpu->h << 8) | cpu->l)) - 1;
            // for Auxiliary carry
            cpu->ac = !((lo & 0xF) == 0xF);
            check_condition_bits(cpu, 1, 1, 1, 0, lo);
            write_memory(((cpu->h << 8) | cpu->l), lo);
            return 10;
        case 0x36: // MVI M, d8
            addr = (cpu->h << 8) | cpu->l;
            write_memory(addr, read_memory(cpu, cpu->pc++));
            return 10;
        case 0x37: // STC
            cpu->cy = 1;
            return 4;
        case 0x38: // NOP
            return 4;
        case 0x39: // DAD SP
            res_32bits = (cpu->sp + ((cpu->h << 8) | cpu->l));
            cpu->cy = (res_32bits > 0xFFFF); // Carry for 32 bits
            cpu->h = (uint8_t)((res_32bits & 0xFF00) >> 8);
            cpu->l = (uint8_t)(res_32bits & 0x00FF);
            return 10;
        case 0x3A: // LDA a16
            lo = read_memory(cpu, cpu->pc++);
            hi = read_memory(cpu, cpu->pc++);
            addr = ((hi << 8) | lo);
            cpu->a = read_memory(cpu, addr);
            return 13;
        case 0x3B: // DCX SP
            cpu->sp--;
            return 5;
        case 0x3C: // INR A
            res_16bits = cpu->a + 1;
            // for Auxiliary carry
            cpu->ac = (((cpu->a & 0xF) + 1) > 0xF);
            check_condition_bits(cpu, 1, 1, 1, 0, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 5;
        case 0x3D: // DCR A
            lo = cpu->a - 1;
            // for Auxiliary carry
            cpu->ac = !((lo & 0xF) == 0xF);
            check_condition_bits(cpu, 1, 1, 1, 0, lo);
            cpu->a = lo;
            return 5;
        case 0x3E: // MVI A, d8
            lo = read_memory(cpu, cpu->pc++);
            cpu->a = lo;
            return 7;
        case 0x3F: // CMC
            cpu->cy ^= 1;
            return 4;
        case 0x40: // MOV B, B
            cpu->b = cpu->b;
            return 5;
        case 0x41: // MOV B, C
            cpu->b = cpu->c;
            return 5;
        case 0x42: // MOV B, D
            cpu->b = cpu->d;
            return 5;
        case 0x43: // MOV B, E
            cpu->b = cpu->e;
            return 5;
        case 0x44: // MOV B, H
            cpu->b = cpu->h;
            return 5;
        case 0x45: // MOV B, L
            cpu->b = cpu->l;
            return 5;
        case 0x46: // MOV B, M
            cpu->b = read_memory(cpu, ((cpu->h << 8) | cpu->l));
            return 7;
        case 0x47: // MOV B, A
            cpu->b = cpu->a;
            return 5;
        case 0x48: // MOV C, B
            cpu->c = cpu->b;
            return 5;
        case 0x49: // MOV C, C
            cpu->c = cpu->c;
            return 5;
        case 0x4A: // MOV C, D
            cpu->c = cpu->d;
            return 5;
        case 0x4B: // MOV C, E
            cpu->c = cpu->e;
            return 5;
        case 0x4C: // MOV C, H
             cpu->c = cpu->h;
            return 5;
        case 0x4D: // MOV C, L
            cpu->c = cpu->l;
            return 5;
        case 0x4E: // MOV C, M
            cpu->c = read_memory(cpu, ((cpu->h << 8) | cpu->l));
            return 7;
        case 0x4F: // MOV C, A
            cpu->c = cpu->a;
            return 5;
        case 0x50: // MOV D, B
            cpu->d = cpu->b;
            return 5;
        case 0x51: // MOV D, C
            cpu->d = cpu->c;
            return 5;
        case 0x52: // MOV D, D
            cpu->d = cpu->d;
            return 5;
        case 0x53: // MOV D, E
            cpu->d = cpu->e;
            return 5;
        case 0x54: // MOV D, H
            cpu->d = cpu->h;
            return 5;
        case 0x55: // MOV D, L
            cpu->d = cpu->l;
            return 5;
        case 0x56: // MOV D, M
            cpu->d = read_memory(cpu, ((cpu->h << 8) | cpu->l));
            return 7;
        case 0x57: // MOV D, A
            cpu->d = cpu->a;
            return 5;
        case 0x58: // MOV E, B
            cpu->e = cpu->b;
            return 5;
        case 0x59: // MOV E, C
            cpu->e = cpu->c;
            return 5;
        case 0x5A: // MOV E, D
            cpu->e = cpu->d;
            return 5;
        case 0x5B: // MOV E, E
            cpu->e = cpu->e;
            return 5;
        case 0x5C: // MOV E, H
            cpu->e = cpu->h;
            return 5;
        case 0x5D: // MOV E, L
            cpu->e = cpu->l;
            return 5;
        case 0x5E: // MOV E, M
            cpu->e = read_memory(cpu, ((cpu->h << 8) | cpu->l));
            return 7;
        case 0x5F: // MOV E, A
            cpu->e = cpu->a;
            return 5;
        case 0x60: // MOV H, B
            cpu->h = cpu->b;
            return 5;
        case 0x61: // MOV H, C
            cpu->h = cpu->c;
            return 5;
        case 0x62: // MOV H, D
            cpu->h = cpu->d;
            return 5;
        case 0x63: // MOV H, E
            cpu->h = cpu->e;
            return 5;
        case 0x64: // MOV H, H
            cpu->h = cpu->h;
            return 5;
        case 0x65: // MOV H, L
            cpu->h = cpu->l;
            return 5;
        case 0x66: // MOV H, M
             cpu->h = read_memory(cpu, ((cpu->h << 8) | cpu->l));
            return 7;
        case 0x67: // MOV H, A
            cpu->h = cpu->a;
            return 5;
        case 0x68: // MOV L, B
            cpu->l = cpu->b;
            return 5;
        case 0x69: // MOV L, C
            cpu->l = cpu->c;
            return 5;
        case 0x6A: // MOV L, D
            cpu->l = cpu->d;
            return 5;
        case 0x6B: // MOV L, E
            cpu->l = cpu->e;
            return 5;
        case 0x6C: // MOV L, H
            cpu->l = cpu->h;
            return 5;
        case 0x6D: // MOV L, L
            cpu->l = cpu->l;
            return 5;
        case 0x6E: // MOV L, M
            cpu->l = read_memory(cpu, ((cpu->h << 8) | cpu->l));
            return 7;
        case 0x6F: // MOV L, A
            cpu->l = cpu->a;
            return 5;
        case 0x70: // MOV M, B
            write_memory(((cpu->h << 8) | cpu->l), cpu->b);
            return 7;
        case 0x71: // MOV M, C
            write_memory(((cpu->h << 8) | cpu->l), cpu->c);
            return 7;
        case 0x72: // MOV M, D
            write_memory(((cpu->h << 8) | cpu->l), cpu->d);
            return 7;
        case 0x73: // MOV M, E
            write_memory(((cpu->h << 8) | cpu->l), cpu->e);
            return 7;
        case 0x74: // MOV M, H
            write_memory(((cpu->h << 8) | cpu->l), cpu->h);
            return 7;
        case 0x75: // MOV M, L
            write_memory(((cpu->h << 8) | cpu->l), cpu->l);
            return 7;
        case 0x76: // HLT
            cpu->halted = true;
            return 7;
        case 0x77: // MOV M, A
            write_memory(((cpu->h << 8) | cpu->l), cpu->a);
            return 7;
        case 0x78: // MOV A, B
            cpu->a = cpu->b;
            return 5;
        case 0x79: // MOV A, C
            cpu->a = cpu->c;
            return 5;
        case 0x7A: // MOV A, D
            cpu->a = cpu->d;
            return 5;
        case 0x7B: // MOV A, E
            cpu->a = cpu->e;
            return 5;
        case 0x7C: // MOV A, H
            cpu->a = cpu->h;
            return 5;
        case 0x7D: // MOV A, L
            cpu->a = cpu->l;
            return 5;
        case 0x7E: // MOV A, M
            cpu->a = read_memory(cpu, ((cpu->h << 8) | cpu->l));
            return 7;
        case 0x7F: // MOV A, A
            cpu->a = cpu->a;
            return 5;
        case 0x80: // ADD B
            res_16bits = cpu->a + cpu->b;
            cpu->ac = (((cpu->a & 0x0F) + (cpu->b & 0x0F)) > 0x0F);
            check_condition_bits(cpu, 1, 1, 1, 1, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 4;
        case 0x81: // ADD C
            res_16bits = cpu->a + cpu->c;
            cpu->ac = (((cpu->a & 0x0F) + (cpu->c & 0x0F)) > 0x0F);
            check_condition_bits(cpu, 1, 1, 1, 1, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 4;
        case 0x82: // ADD D
            res_16bits = cpu->a + cpu->d;
            cpu->ac = (((cpu->a & 0x0F) + (cpu->d & 0x0F)) > 0x0F);
            check_condition_bits(cpu, 1, 1, 1, 1, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 4;
        case 0x83: // ADD E
            res_16bits = cpu->a + cpu->e;
            cpu->ac = (((cpu->a & 0x0F) + (cpu->e & 0x0F)) > 0x0F);
            check_condition_bits(cpu, 1, 1, 1, 1, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 4;
        case 0x84: // ADD H
            res_16bits = cpu->a + cpu->h;
            cpu->ac = (((cpu->a & 0x0F) + (cpu->h & 0x0F)) > 0x0F);
            check_condition_bits(cpu, 1, 1, 1, 1, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 4;
        case 0x85: // ADD L
            res_16bits = cpu->a + cpu->l;
            cpu->ac = (((cpu->a & 0x0F) + (cpu->l & 0x0F)) > 0x0F);
            check_condition_bits(cpu, 1, 1, 1, 1, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 4;
        case 0x86: // ADD M
            lo = read_memory(cpu, ((cpu->h << 8) | cpu->l));
            res_16bits = cpu->a + lo;
            cpu->ac = (((cpu->a & 0x0F) + (lo & 0x0F)) > 0x0F);
            check_condition_bits(cpu, 1, 1, 1, 1, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 7;
        case 0x87: // ADD A
            res_16bits = cpu->a + cpu->a;
            cpu->ac = (((cpu->a & 0x0F) + (cpu->a & 0x0F)) > 0x0F);
            check_condition_bits(cpu, 1, 1, 1, 1, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 4;
        case 0x88: // ADC B
            res_16bits = cpu->a + cpu->b + cpu->cy;
            cpu->ac = (((cpu->a & 0x0F) + (cpu->b & 0x0F) + cpu->cy) > 0x0F);
            check_condition_bits(cpu, 1, 1, 1, 1, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 4;
        case 0x89: // ADC C
            res_16bits = cpu->a + cpu->c + cpu->cy;
            cpu->ac = (((cpu->a & 0x0F) + (cpu->c & 0x0F) + cpu->cy) > 0x0F);
            check_condition_bits(cpu, 1, 1, 1, 1, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 4;
        case 0x8A: // ADC D
            res_16bits = cpu->a + cpu->d + cpu->cy;
            cpu->ac = (((cpu->a & 0x0F) + (cpu->d & 0x0F) + cpu->cy) > 0x0F);
            check_condition_bits(cpu, 1, 1, 1, 1, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 4;
        case 0x8B: // ADC E
            res_16bits = cpu->a + cpu->e + cpu->cy;
            cpu->ac = (((cpu->a & 0x0F) + (cpu->e & 0x0F) + cpu->cy) > 0x0F);
            check_condition_bits(cpu, 1, 1, 1, 1, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 4;
        case 0x8C: // ADC H
            res_16bits = cpu->a + cpu->h + cpu->cy;
            cpu->ac = (((cpu->a & 0x0F) + (cpu->h & 0x0F) + cpu->cy) > 0x0F);
            check_condition_bits(cpu, 1, 1, 1, 1, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 4;
        case 0x8D: // ADC L
            res_16bits = cpu->a + cpu->l + cpu->cy;
            cpu->ac = (((cpu->a & 0x0F) + (cpu->l & 0x0F) + cpu->cy) > 0x0F);
            check_condition_bits(cpu, 1, 1, 1, 1, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 4;
        case 0x8E: // ADC M
            lo = read_memory(cpu, ((cpu->h << 8) | cpu->l));
            res_16bits = cpu->a + lo + cpu->cy;
            cpu->ac = (((cpu->a & 0x0F) + (lo & 0x0F) + cpu->cy) > 0x0F);
            check_condition_bits(cpu, 1, 1, 1, 1, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 7;
        case 0x8F: // ADC A
            res_16bits = cpu->a + cpu->a + cpu->cy;
            cpu->ac = (((cpu->a & 0x0F) + (cpu->a & 0x0F) + cpu->cy) > 0x0F);
            check_condition_bits(cpu, 1, 1, 1, 1, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 4;
        case 0x90: // SUB B
            res_16bits = cpu->a - cpu->b;
            cpu->ac = (((cpu->a & 0xF) + ((~cpu->b) & 0xF) + 1) > 0xF);
            check_condition_bits(cpu, 1, 1, 1, 1, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 4;
        case 0x91: // SUB C
            res_16bits = cpu->a - cpu->c;
            cpu->ac = (((cpu->a & 0xF) + ((~cpu->c) & 0xF) + 1) > 0xF);
            check_condition_bits(cpu, 1, 1, 1, 1, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 4;
        case 0x92: // SUB D
            res_16bits = cpu->a - cpu->d;
            cpu->ac = (((cpu->a & 0xF) + ((~cpu->d) & 0xF) + 1) > 0xF);
            check_condition_bits(cpu, 1, 1, 1, 1, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 4;
        case 0x93: // SUB E
            res_16bits = cpu->a - cpu->e;
            cpu->ac = (((cpu->a & 0xF) + ((~cpu->e) & 0xF) + 1) > 0xF);
            check_condition_bits(cpu, 1, 1, 1, 1, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 4;
        case 0x94: // SUB H
            res_16bits = cpu->a - cpu->h;
            cpu->ac = (((cpu->a & 0xF) + ((~cpu->h) & 0xF) + 1) > 0xF);
            check_condition_bits(cpu, 1, 1, 1, 1, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 4;
        case 0x95: // SUB L
            res_16bits = cpu->a - cpu->l;
            cpu->ac = (((cpu->a & 0xF) + ((~cpu->l) & 0xF) + 1) > 0xF);
            check_condition_bits(cpu, 1, 1, 1, 1, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 4;
        case 0x96: // SUB M
            lo = read_memory(cpu, ((cpu->h << 8) | cpu->l));
            res_16bits = cpu->a - lo;
            cpu->ac = (((cpu->a & 0xF) + ((~lo) & 0xF) + 1) > 0xF);
            check_condition_bits(cpu, 1, 1, 1, 1, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 7;
        case 0x97: // SUB A
            res_16bits = cpu->a - cpu->a;
            cpu->ac = (((cpu->a & 0xF) + ((~cpu->a) & 0xF) + 1) > 0xF);
            check_condition_bits(cpu, 1, 1, 1, 1, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 4;
        case 0x98: // SBB B
            res_16bits = cpu->a - (cpu->b + cpu->cy);
            cpu->ac = (((cpu->a & 0xF) + ((~cpu->b) & 0xF) + !cpu->cy) > 0xF);
            check_condition_bits(cpu, 1, 1, 1, 1, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 4;
        case 0x99: // SBB C
            res_16bits = cpu->a - (cpu->c + cpu->cy);
            cpu->ac = (((cpu->a & 0xF) + ((~cpu->c) & 0xF) + !cpu->cy) > 0xF);
            check_condition_bits(cpu, 1, 1, 1, 1, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 4;
        case 0x9A: // SBB D
            res_16bits = cpu->a - (cpu->d + cpu->cy);
            cpu->ac = (((cpu->a & 0xF) + ((~cpu->d) & 0xF) + !cpu->cy) > 0xF);
            check_condition_bits(cpu, 1, 1, 1, 1, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 4;
        case 0x9B: // SBB E
            res_16bits = cpu->a - (cpu->e + cpu->cy);
            cpu->ac = (((cpu->a & 0xF) + ((~cpu->e) & 0xF) + !cpu->cy) > 0xF);
            check_condition_bits(cpu, 1, 1, 1, 1, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 4;
        case 0x9C: // SBB H
            res_16bits = cpu->a - (cpu->h + cpu->cy);
            cpu->ac = (((cpu->a & 0xF) + ((~cpu->h) & 0xF) + !cpu->cy) > 0xF);
            check_condition_bits(cpu, 1, 1, 1, 1, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 4;
        case 0x9D: // SBB L
            res_16bits = cpu->a - (cpu->l + cpu->cy);
            cpu->ac = (((cpu->a & 0xF) + ((~cpu->l) & 0xF) + !cpu->cy) > 0xF);
            check_condition_bits(cpu, 1, 1, 1, 1, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 4;
        case 0x9E: // SBB M
            lo = read_memory(cpu, ((cpu->h << 8) | cpu->l));
            res_16bits = cpu->a - (lo + cpu->cy);
            cpu->ac = (((cpu->a & 0xF) + ((~lo) & 0xF) + !cpu->cy) > 0xF);
            check_condition_bits(cpu, 1, 1, 1, 1, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 7;
        case 0x9F: // SBB A
            res_16bits = cpu->a - (cpu->a + cpu->cy);
            cpu->ac = (((cpu->a & 0xF) + ((~cpu->a) & 0xF) + !cpu->cy) > 0xF);
            check_condition_bits(cpu, 1, 1, 1, 1, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 4;
        case 0xA0: // ANA B
            res_16bits = cpu->a & cpu->b;
            cpu->ac = (((cpu->a | cpu->b) & 0x08) != 0);
            cpu->cy = 0;
            check_condition_bits(cpu, 1, 1, 1, 0, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 4;
        case 0xA1: // ANA C
            res_16bits = cpu->a & cpu->c;
            cpu->ac = (((cpu->a | cpu->c) & 0x08) != 0);
            cpu->cy = 0;
            check_condition_bits(cpu, 1, 1, 1, 0, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 4;
        case 0xA2: // ANA D
            res_16bits = cpu->a & cpu->d;
            cpu->ac = (((cpu->a | cpu->d) & 0x08) != 0);
            cpu->cy = 0;
            check_condition_bits(cpu, 1, 1, 1, 0, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 4;
        case 0xA3: // ANA E
            res_16bits = cpu->a & cpu->e;
            cpu->ac = (((cpu->a | cpu->e) & 0x08) != 0);
            cpu->cy = 0;
            check_condition_bits(cpu, 1, 1, 1, 0, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 4;
        case 0xA4: // ANA H
            res_16bits = cpu->a & cpu->h;
            cpu->ac = (((cpu->a | cpu->h) & 0x08) != 0);
            cpu->cy = 0;
            check_condition_bits(cpu, 1, 1, 1, 0, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 4;
        case 0xA5: // ANA L
            res_16bits = cpu->a & cpu->l;
            cpu->ac = (((cpu->a | cpu->l) & 0x08) != 0);
            cpu->cy = 0;
            check_condition_bits(cpu, 1, 1, 1, 0, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 4;
        case 0xA6: // ANA M
            lo = read_memory(cpu, ((cpu->h << 8) | cpu->l));
            res_16bits = cpu->a & lo;
            cpu->ac = (((cpu->a | lo) & 0x08) != 0);
            cpu->cy = 0;
            check_condition_bits(cpu, 1, 1, 1, 0, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 7;
        case 0xA7: // ANA A
            res_16bits = cpu->a & cpu->a;
            cpu->ac = (((cpu->a | cpu->a) & 0x08) != 0);
            cpu->cy = 0;
            check_condition_bits(cpu, 1, 1, 1, 0, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 4;
        case 0xA8: // XRA B
            res_16bits = cpu->a ^ cpu->b;
            cpu->ac = 0;
            cpu->cy = 0;
            check_condition_bits(cpu, 1, 1, 1, 0, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 4;
        case 0xA9: // XRA C
            res_16bits = cpu->a ^ cpu->c;
            cpu->ac = 0;
            cpu->cy = 0;
            check_condition_bits(cpu, 1, 1, 1, 0, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 4;
        case 0xAA: // XRA D
            res_16bits = cpu->a ^ cpu->d;
            cpu->ac = 0;
            cpu->cy = 0;
            check_condition_bits(cpu, 1, 1, 1, 0, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 4;
        case 0xAB: // XRA E
            res_16bits = cpu->a ^ cpu->e;
            cpu->ac = 0;
            cpu->cy = 0;
            check_condition_bits(cpu, 1, 1, 1, 0, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 4;
        case 0xAC: // XRA H
            res_16bits = cpu->a ^ cpu->h;
            cpu->ac = 0;
            cpu->cy = 0;
            check_condition_bits(cpu, 1, 1, 1, 0, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 4;
        case 0xAD: // XRA L
            res_16bits = cpu->a ^ cpu->l;
            cpu->ac = 0;
            cpu->cy = 0;
            check_condition_bits(cpu, 1, 1, 1, 0, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 4;
        case 0xAE: // XRA M
            lo = read_memory(cpu, ((cpu->h << 8) | cpu->l));
            res_16bits = cpu->a ^ lo;
            cpu->ac = 0;
            cpu->cy = 0;
            check_condition_bits(cpu, 1, 1, 1, 0, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 7;
        case 0xAF: // XRA A
            res_16bits = cpu->a ^ cpu->a;
            cpu->ac = 0;
            cpu->cy = 0;
            check_condition_bits(cpu, 1, 1, 1, 0, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 4;
        case 0xB0: // ORA B
            res_16bits = cpu->a | cpu->b;
            cpu->ac = 0;
            cpu->cy = 0;
            check_condition_bits(cpu, 1, 1, 1, 0, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 4;
        case 0xB1: // ORA C
            res_16bits = cpu->a | cpu->c;
            cpu->ac = 0;
            cpu->cy = 0;
            check_condition_bits(cpu, 1, 1, 1, 0, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 4;
        case 0xB2: // ORA D
            res_16bits = cpu->a | cpu->d;
            cpu->ac = 0;
            cpu->cy = 0;
            check_condition_bits(cpu, 1, 1, 1, 0, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 4;
        case 0xB3: // ORA E
            res_16bits = cpu->a | cpu->e;
            cpu->ac = 0;
            cpu->cy = 0;
            check_condition_bits(cpu, 1, 1, 1, 0, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 4;
        case 0xB4: // ORA H
            res_16bits = cpu->a | cpu->h;
            cpu->ac = 0;
            cpu->cy = 0;
            check_condition_bits(cpu, 1, 1, 1, 0, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 4;
        case 0xB5: // ORA L
            res_16bits = cpu->a | cpu->l;
            cpu->ac = 0;
            cpu->cy = 0;
            check_condition_bits(cpu, 1, 1, 1, 0, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 4;
        case 0xB6: // ORA M
            lo = read_memory(cpu, ((cpu->h << 8) | cpu->l));
            res_16bits = cpu->a | lo;
            cpu->ac = 0;
            cpu->cy = 0;
            check_condition_bits(cpu, 1, 1, 1, 0, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 7;
        case 0xB7: // ORA A
            res_16bits = cpu->a | cpu->a;
            cpu->ac = 0;
            cpu->cy = 0;
            check_condition_bits(cpu, 1, 1, 1, 0, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 4;
        case 0xB8: // CMP B
            res_signed = cpu->a - cpu->b;
            cpu->cy = res_signed >> 8;
            cpu->ac = ~(cpu->a ^ res_signed ^ cpu->b) & 0x10;
            cpu->s = ((res_signed & 0x80) != 0);
            check_condition_bits(cpu, 0, 1, 1, 0, res_signed);
            return 4;
        case 0xB9: // CMP C
            res_signed = cpu->a - cpu->c;
            cpu->cy = res_signed >> 8;
            cpu->ac = ~(cpu->a ^ res_signed ^ cpu->c) & 0x10;
            cpu->s = ((res_signed & 0x80) != 0);
            check_condition_bits(cpu, 0, 1, 1, 0, res_signed);
            return 4;
        case 0xBA: // CMP D
            res_signed = cpu->a - cpu->d;
            cpu->cy = res_signed >> 8;
            cpu->ac = ~(cpu->a ^ res_signed ^ cpu->d) & 0x10;
            cpu->s = ((res_signed & 0x80) != 0);
            check_condition_bits(cpu, 0, 1, 1, 0, res_signed);
            return 4;
        case 0xBB: // CMP E
            res_signed = cpu->a - cpu->e;
            cpu->cy = res_signed >> 8;
            cpu->ac = ~(cpu->a ^ res_signed ^ cpu->e) & 0x10;
            cpu->s = ((res_signed & 0x80) != 0);
            check_condition_bits(cpu, 0, 1, 1, 0, res_signed);
            return 4;
        case 0xBC: // CMP H
            res_signed = cpu->a - cpu->h;
            cpu->cy = res_signed >> 8;
            cpu->ac = ~(cpu->a ^ res_signed ^ cpu->h) & 0x10;
            cpu->s = ((res_signed & 0x80) != 0);
            check_condition_bits(cpu, 0, 1, 1, 0, res_signed);
            return 4;
        case 0xBD: // CMP L
            res_signed = cpu->a - cpu->l;
            cpu->cy = res_signed >> 8;
            cpu->ac = ~(cpu->a ^ res_signed ^ cpu->l) & 0x10;
            cpu->s = ((res_signed & 0x80) != 0);
            check_condition_bits(cpu, 0, 1, 1, 0, res_signed);
            return 4;
        case 0xBE: // CMP M
            lo = read_memory(cpu, ((cpu->h << 8) | cpu->l));
            res_signed = cpu->a - lo;
            cpu->cy = res_signed >> 8;
            cpu->ac = ~(cpu->a ^ res_signed ^ lo) & 0x10;
            cpu->s = ((res_signed & 0x80) != 0);
            check_condition_bits(cpu, 1, 1, 1, 1, res_signed);
            return 7;
        case 0xBF: // CMP A
            res_signed = cpu->a - cpu->a;
            cpu->cy = res_signed >> 8;
            cpu->ac = ~(cpu->a ^ res_signed ^ cpu->a) & 0x10;
            cpu->s = ((res_signed & 0x80) != 0);
            check_condition_bits(cpu, 0, 1, 1, 0, res_signed);
            return 4;
        case 0xC0: // RNZ
            if (!cpu->z)
            {
                ret(cpu);
                return 11;
            }
            return 5;
        case 0xC1: // POP B
            cpu->c = read_memory(cpu, cpu->sp++);
            cpu->b = read_memory(cpu, cpu->sp++);
            return 10;
        case 0xC2: // JNZ a16
            lo = read_memory(cpu, cpu->pc++);
            hi = read_memory(cpu, cpu->pc++);
            if (!cpu->z)
            {
                addr = (lo + (hi << 8));
                cpu->pc = addr;
            }
            return 10;
        case 0xC3: // JMP a16
            lo = read_memory(cpu, cpu->pc++);
            hi = read_memory(cpu, cpu->pc++);
            addr = (lo + (hi << 8));
            cpu->pc = addr;
            return 10;
        case 0xC4: // CNZ
            lo = read_memory(cpu, cpu->pc++);
            hi = read_memory(cpu, cpu->pc++);
            if (!cpu->z)
            {
                addr = (lo + (hi << 8));
                call(cpu, addr);
                return 17;
            }
            return 11;
        case 0xC5: // PUSH B
            write_memory(--cpu->sp, cpu->b);
            write_memory(--cpu->sp, cpu->c);
            return 11;
        case 0xC6: // ADI d8
            lo = read_memory(cpu, cpu->pc++);
            res_16bits = cpu->a + lo;
            cpu->ac = (((cpu->a & 0x0F) + (lo & 0x0F)) > 0x0F);
            check_condition_bits(cpu, 1, 1, 1, 1, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 7;
        case 0xC7: // RST 0
            rst(cpu, 0);
            return 11;
        case 0xC8: // RZ
            if (cpu->z)
            {
                ret(cpu);
                return 11;
            }
            return 5;
        case 0xC9: // RET
            ret(cpu);
            return 10;
        case 0xCA: // JZ a16
            lo = read_memory(cpu, cpu->pc++);
            hi = read_memory(cpu, cpu->pc++);
            if (cpu->z)
            {
                addr = (lo + (hi << 8));
                cpu->pc = addr;
            }
            return 10;
        case 0xCB: // JMP a16
            lo = read_memory(cpu, cpu->pc++);
            hi = read_memory(cpu, cpu->pc++);
            addr = (lo + (hi << 8));
            cpu->pc = addr;
            return 10;
        case 0xCC: // CZ a16
            lo = read_memory(cpu, cpu->pc++);
            hi = read_memory(cpu, cpu->pc++);
            if (cpu->z)
            {
                addr = (lo + (hi << 8));
                call(cpu, addr);
                return 17;
            }
            return 11;
        case 0xCD: // CALL a16
            lo = read_memory(cpu, cpu->pc++);
            hi = read_memory(cpu, cpu->pc++);
            addr = (lo + (hi << 8));
            call(cpu, addr);
            return 17;
        case 0xCE: // ACI d8
            lo = read_memory(cpu, cpu->pc++);
            res_16bits = cpu->a + lo + cpu->cy;
            cpu->ac = (((cpu->a & 0x0F) + (lo & 0x0F) + cpu->cy) > 0x0F);
            check_condition_bits(cpu, 1, 1, 1, 1, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 7;
        case 0xCF: // RST 1
            rst(cpu, 1);
            return 11;
        case 0xD0: // RNC
            if (!cpu->cy)
            {
                ret(cpu);
                return 11;
            }
            return 5;
        case 0xD1: // POP D
            cpu->e = read_memory(cpu, cpu->sp++);
            cpu->d = read_memory(cpu, cpu->sp++);
            return 10;
        case 0xD2: // JNC a16
            lo = read_memory(cpu, cpu->pc++);
            hi = read_memory(cpu, cpu->pc++);
            if (!cpu->cy)
            {
                addr = (lo + (hi << 8));
                cpu->pc = addr;
            }
            return 10;
        case 0xD3: // OUT d8
            lo = read_memory(cpu, cpu->pc++);
            write_io(*cpu, lo, cpu->a);
            return 10;
        case 0xD4: // CNC a16
            lo = read_memory(cpu, cpu->pc++);
            hi = read_memory(cpu, cpu->pc++);
            if (!cpu->cy)
            {
                addr = (lo + (hi << 8));
                call(cpu, addr);
                return 17;
            }
            return 11;
        case 0xD5: // PUSH D
            write_memory(--cpu->sp, cpu->d);
            write_memory(--cpu->sp, cpu->e);
            return 11;
        case 0xD6: // SUI d8
            lo = read_memory(cpu, cpu->pc++);
            res_16bits = cpu->a - lo;
            cpu->ac = (((cpu->a & 0xF) + ((~lo) & 0xF) + 1) > 0xF);
            check_condition_bits(cpu, 1, 1, 1, 1, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 7;
        case 0xD7: // RST 2
            rst(cpu, 2);
            return 11;
        case 0xD8: // RC
            if (cpu->cy)
            {
                ret(cpu);
                return 11;
            }
            return 5;
        case 0xD9: // RET
            ret(cpu);
            return 10;
        case 0xDA: // JC a16
            lo = read_memory(cpu, cpu->pc++);
            hi = read_memory(cpu, cpu->pc++);
            if (cpu->cy)
            {
                addr = (lo + (hi << 8));
                cpu->pc = addr;
            }
            return 10;
        case 0xDB: // IN d8
            lo = read_memory(cpu, cpu->pc++);
            cpu->a = read_io(lo);
            return 10;
        case 0xDC: // CC a16
            lo = read_memory(cpu, cpu->pc++);
            hi = read_memory(cpu, cpu->pc++);
            if (cpu->cy)
            {
                addr = (lo + (hi << 8));
                call(cpu, addr);
                return 17;
            }
            return 11;
        case 0xDD: // CALL a16
            lo = read_memory(cpu, cpu->pc++);
            hi = read_memory(cpu, cpu->pc++);
            addr = (lo + (hi << 8));
            call(cpu, addr);
            return 17;
        case 0xDE: // SBI d8
            lo = read_memory(cpu, cpu->pc++);
            res_16bits = cpu->a - (lo + cpu->cy);
            cpu->ac = (((cpu->a & 0xF) + ((~lo) & 0xF) + !cpu->cy) > 0xF);
            check_condition_bits(cpu, 1, 1, 1, 1, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 7;
        case 0xDF: // RST 3
            rst(cpu, 3);
            return 11;
        case 0xE0: // RPO
            if (!cpu->p)
            {
                ret(cpu);
                return 11;
            }
            return 5;
        case 0xE1: // POP H
            cpu->l = read_memory(cpu, cpu->sp++);
            cpu->h = read_memory(cpu, cpu->sp++);
            return 10;
        case 0xE2: // JPO a16
            lo = read_memory(cpu, cpu->pc++);
            hi = read_memory(cpu, cpu->pc++);
            if (!cpu->p)
            {
                addr = (lo + (hi << 8));
                cpu->pc = addr;
            }
            return 10;
        case 0xE3: // XTHL
            lo = read_memory(cpu, cpu->sp);
            hi = read_memory(cpu, cpu->sp+1);
            write_memory(cpu->sp, cpu->l);
            write_memory(cpu->sp+1, cpu->h);
            cpu->l = lo;
            cpu->h = hi;
            return 18;
        case 0xE4: // CPO a16
            lo = read_memory(cpu, cpu->pc++);
            hi = read_memory(cpu, cpu->pc++);
            if (!cpu->p)
            {
                addr = (lo + (hi << 8));
                call(cpu, addr);
                return 17;
            }
            return 11;
        case 0xE5: // PUSH H
            write_memory(--cpu->sp, cpu->h);
            write_memory(--cpu->sp, cpu->l);
            return 11;
        case 0xE6: // ANI d8
            lo = read_memory(cpu, cpu->pc++);
            res_16bits = cpu->a & lo;
            cpu->ac = (((cpu->a | lo) & 0x08) != 0);
            cpu->cy = 0;
            check_condition_bits(cpu, 1, 1, 1, 0, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 7;
        case 0xE7: // RST 4
            rst(cpu, 4);
            return 11;
        case 0xE8: // RPE
            if (cpu->p)
            {
                ret(cpu);
                return 11;
            }
            return 5;
        case 0xE9: // PCHL
            cpu->pc = ((cpu->h << 8) | cpu->l);
            return 5;
        case 0xEA: // JPE a16
            lo = read_memory(cpu, cpu->pc++);
            hi = read_memory(cpu, cpu->pc++);
            if (cpu->p)
            {
                addr = (lo + (hi << 8));
                cpu->pc = addr;
            }
            return 10;
        case 0xEB: // XCHG
            hi = cpu->h;
            lo = cpu->l;
            cpu->h = cpu->d;
            cpu->l = cpu->e;
            cpu->d = hi;
            cpu->e = lo;
            return 4;
        case 0xEC: // CPE a16
            lo = read_memory(cpu, cpu->pc++);
            hi = read_memory(cpu, cpu->pc++);
            if (cpu->p)
            {
                addr = (lo + (hi << 8));
                call(cpu, addr);
                return 17;
            }
            return 11;
        case 0xED: // CALL a16
            lo = read_memory(cpu, cpu->pc++);
            hi = read_memory(cpu, cpu->pc++);
            addr = (lo + (hi << 8));
            call(cpu, addr);
            return 17;
        case 0xEE: // XRI d8
            lo = read_memory(cpu, cpu->pc++);
            res_16bits = cpu->a ^ lo;
            cpu->ac = 0;
            cpu->cy = 0;
            check_condition_bits(cpu, 1, 1, 1, 0, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 7;
        case 0xEF: // RST 5
            rst(cpu, 5);
            return 11;
        case 0xF0: // RP
            if (!cpu->s)
            {
                ret(cpu);
                return 11;
            }
            return 5;
        case 0xF1: // POP PSW
            lo = read_memory(cpu, cpu->sp++);
            cpu->s = ((lo & 0x80) > 0);
            cpu->z = ((lo & 0x40) > 0);
            cpu->ac = ((lo & 0x10) > 0);
            cpu->p = ((lo & 0x04) > 0);
            cpu->cy = ((lo & 0x01) > 0);
            cpu->a = read_memory(cpu, cpu->sp++);
            return 10;
        case 0xF2: // JP a16
            lo = read_memory(cpu, cpu->pc++);
            hi = read_memory(cpu, cpu->pc++);
            if (!cpu->s)
            {
                addr = (lo + (hi << 8));
                cpu->pc = addr;
            }
            return 10;
        case 0xF3: // DI
            cpu->interrupt_enable = false;
            cpu->interrupt_pending = false;
            return 4;
        case 0xF4: // CP a16
            lo = read_memory(cpu, cpu->pc++);
            hi = read_memory(cpu, cpu->pc++);
            if (!cpu->s)
            {
                addr = (lo + (hi << 8));
                call(cpu, addr);
                return 17;
            }
            return 11;
        case 0xF5: // PUSH PSW
            write_memory(--cpu->sp, cpu->a);
            write_memory(--cpu->sp, get_f_flags(cpu));
            return 11;
        case 0xF6: // ORI d8
            lo = read_memory(cpu, cpu->pc++);
            res_16bits = cpu->a | lo;
            cpu->ac = 0;
            cpu->cy = 0;
            check_condition_bits(cpu, 1, 1, 1, 0, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 7;
        case 0xF7: // RST 6
            rst(cpu, 6);
            return 11;
        case 0xF8: // RM
            if (cpu->s)
            {
                ret(cpu);
                return 11;
            }
            return 5;
        case 0xF9: // SPHL
            cpu->sp = ((cpu->h << 8) | cpu->l);
            return 5;
        case 0xFA: // JM a16
            lo = read_memory(cpu, cpu->pc++);
            hi = read_memory(cpu, cpu->pc++);
            if (cpu->s)
            {
                addr = (lo + (hi << 8));
                cpu->pc = addr;
            }
            return 10;
        case 0xFB: // EI
            cpu->ei_pending = true;
            return 4;
        case 0xFC: // CM a16
            lo = read_memory(cpu, cpu->pc++);
            hi = read_memory(cpu, cpu->pc++);
            if (cpu->s)
            {
                addr = (lo + (hi << 8));
                call(cpu, addr);
                return 17;
            }
            return 11;
        case 0xFD: // CALL a16
            lo = read_memory(cpu, cpu->pc++);
            hi = read_memory(cpu, cpu->pc++);
            addr = (lo + (hi << 8));
            call(cpu, addr);
            return 17;
        case 0xFE: // CPI d8
            lo = read_memory(cpu, cpu->pc++);
            res_16bits = cpu->a - lo;
            cpu->ac = ~(cpu->a ^ res_16bits ^ lo) & 0x10;
            check_condition_bits(cpu, 1, 1, 1, 1, res_16bits);
            return 7;
        case 0xFF: // RST 7
            rst(cpu, 7);
            return 11;
        default:
            printf("Opcode inconnu: 0x%02X\n", opcode);
            return -1;
    }
    return 0;
}

void call(CPU *cpu, uint16_t addr)
{
    write_memory(--cpu->sp, ((cpu->pc >> 8) & 0xFF));
    write_memory(--cpu->sp, (cpu->pc & 0xFF));
    cpu->pc = addr;
}

void rst(CPU *cpu, uint8_t i)
{
    uint16_t addr = 8 * i;
    call(cpu, addr);
}

void ret(CPU *cpu)
{
    uint8_t lo = read_memory(cpu, cpu->sp++);
    uint8_t hi = read_memory(cpu, cpu->sp++);
    uint16_t addr = (lo + (hi << 8));
    cpu->pc = addr;
}