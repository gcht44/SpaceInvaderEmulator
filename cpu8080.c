#include "includes/cpu8080.h"
#include "includes/utils.h"
#include "includes/memory.h"
#include "includes/io.h"

#include <stdio.h>
#include <stdlib.h>

void init_cpu(CPU *cpu, EMU_RUN_TYPE type)
{
    if (type == CPUDIAGBIN)
    {
        cpu->pc = 0x0100;
        cpu->sp = 0xf000;
    }
    else
    {
        cpu->pc = 0;
        cpu->sp = 0xf000;
    }
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
}

// Gérer le flag ac indépendament dans l'instruction
void check_condition_bits(CPU *cpu, bool s, bool z, bool p, bool c, uint16_t data)
{
    if (z)
        cpu->z = ((data & 0xFF) == 0);
    if (c)
        cpu->cy = ((data & 0xFF) > 0xFF);
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
    return (cpu->s << 7) | (cpu->z << 6) | (cpu->ac << 4) | (cpu->p << 2) | (1 << 1) | (cpu->cy);
}

void print_opcode(CPU *cpu)
{
    uint8_t opcode = cpu->memory[cpu->pc];
    printf("$%04X: %s (A:%02X B:%02X C:%02X D:%02X E:%02X H:%02X L:%02X Flags:%c%c%c%c%c)\n", 
        cpu->pc, opcode_name(opcode), cpu->a, cpu->b, cpu->c, cpu->d, cpu->e,
        cpu->h, cpu->l, (cpu->s == 0) ? '-' : 'S', (cpu->z == 0) ? '-' : 'Z', (cpu->ac == 0) ? '-' : 'A', 
        (cpu->p == 0) ? '-' : 'P', (cpu->cy == 0) ? '-' : 'c');
}

int execute(CPU *cpu)
{
    print_opcode(cpu);
    uint8_t opcode = read_memory(cpu, cpu->pc++);
    uint8_t hi;
    uint8_t lo;
    uint16_t res_16bits;
    uint32_t res_32bits;
    uint16_t addr;
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
            write_memory(cpu, ((cpu->b << 8) | cpu->c), cpu->a);
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
            res_16bits = cpu->b - 1;
            // for Auxiliary carry
            cpu->ac = (((cpu->b & 0xF) - 1) > 0xF);
            check_condition_bits(cpu, 1, 1, 1, 0, res_16bits);
            cpu->b = (uint8_t)(res_16bits & 0xFF);
            return 5;
        case 0x06: // MVI B, d8
            addr = read_memory(cpu, cpu->pc++);
            cpu->b = read_memory(cpu, addr);
            return 7;
        case 0x07: // RLC
            cpu->cy = (cpu->a & 0x80) >> 7;
            cpu->a <<= 1;
            return 4;
        case 0x08: // NOP
            return 4;
        case 0x09: // DAD B
            res_32bits = ((cpu->b << 8) | cpu->c) + ((cpu->h << 8) | cpu->l);
            cpu->cy = ((res_32bits & 0xFFFF) > 0xFFFF); // Carry for 32 bits
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
            res_16bits = cpu->c - 1;
            // for Auxiliary carry
            cpu->ac = (((cpu->c & 0xF) - 1) > 0xF);
            check_condition_bits(cpu, 1, 1, 1, 0, res_16bits);
            cpu->c = (uint8_t)(res_16bits & 0xFF);
            return 5;
        case 0x0E: // MVI C, d8
            addr = read_memory(cpu, cpu->pc++);
            cpu->c = read_memory(cpu, addr);
            return 7;
        case 0x0F: // RRC
            cpu->cy = (cpu->a & 0x1);
            cpu->a >>= 1;
            return 4;
        case 0x10: // NOP
            return 4;
        case 0x11: // LXI D, d16
            cpu->e = read_memory(cpu, cpu->pc++);
            cpu->d = read_memory(cpu, cpu->pc++);
            return 10;
        case 0x12: // STAX D
            write_memory(cpu, ((cpu->d << 8) | cpu->e), cpu->a);
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
            res_16bits = cpu->d - 1;
            // for Auxiliary carry
            cpu->ac = (((cpu->d & 0xF) - 1) > 0xF);
            check_condition_bits(cpu, 1, 1, 1, 0, res_16bits);
            cpu->d = (uint8_t)(res_16bits & 0xFF);
            return 5;
        case 0x16: // MVI D, d8
            addr = read_memory(cpu, cpu->pc++);
            cpu->d = read_memory(cpu, addr);
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
            cpu->cy = ((res_32bits & 0xFFFF) > 0xFFFF); // Carry for 32 bits
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
            res_16bits = cpu->e - 1;
            // for Auxiliary carry
            cpu->ac = (((cpu->e & 0xF) - 1) > 0xF);
            check_condition_bits(cpu, 1, 1, 1, 0, res_16bits);
            cpu->e = (uint8_t)(res_16bits & 0xFF);
            return 5;
        case 0x1E: // MVI E, d8
            addr = read_memory(cpu, cpu->pc++);
            cpu->e = read_memory(cpu, addr);
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
            write_memory(cpu, addr, cpu->l);
            write_memory(cpu, addr+1, cpu->h);
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
            res_16bits = cpu->h - 1;
            // for Auxiliary carry
            cpu->ac = (((cpu->h & 0xF) - 1) > 0xF);
            check_condition_bits(cpu, 1, 1, 1, 0, res_16bits);
            cpu->h = (uint8_t)(res_16bits & 0xFF);
            return 5;
        case 0x26: // MVI H, d8
            addr = read_memory(cpu, cpu->pc++);
            cpu->h = read_memory(cpu, addr);
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
            cpu->cy = ((res_32bits & 0xFFFF) > 0xFFFF); // Carry for 32 bits
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
            res_16bits = cpu->l - 1;
            // for Auxiliary carry
            cpu->ac = (((cpu->l & 0xF) - 1) > 0xF);
            check_condition_bits(cpu, 1, 1, 1, 0, res_16bits);
            cpu->l = (uint8_t)(res_16bits & 0xFF);
            return 5;
        case 0x2E: // MVI L, d8
            addr = read_memory(cpu, cpu->pc++);
            cpu->l = read_memory(cpu, addr);
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
            write_memory(cpu, addr, cpu->a);
            return 13;
        case 0x33: // INX SP
            cpu->sp++;
            return 5;
        case 0x34: // INR M
            res_16bits = read_memory(cpu, ((cpu->h << 8) | cpu->l)) + 1;
            // for Auxiliary carry
            cpu->ac = (((read_memory(cpu, ((cpu->h << 8) | cpu->l)) & 0xF) + 1) > 0xF);
            check_condition_bits(cpu, 1, 1, 1, 0, res_16bits);
            write_memory(cpu, ((cpu->h << 8) | cpu->l), (uint8_t)(res_16bits & 0xFF));
            return 10;
        case 0x35: // DCR M
            res_16bits = read_memory(cpu, ((cpu->h << 8) | cpu->l)) - 1;
            // for Auxiliary carry
            cpu->ac = (((read_memory(cpu, ((cpu->h << 8) | cpu->l)) & 0xF) - 1) > 0xF);
            check_condition_bits(cpu, 1, 1, 1, 0, res_16bits);
            write_memory(cpu, ((cpu->h << 8) | cpu->l), (uint8_t)(res_16bits & 0xFF));
            return 10;
        case 0x36: // MVI M, d8
            addr = (cpu->h << 8) | cpu->l;
            write_memory(cpu, addr, read_memory(cpu, cpu->pc++));
            return 10;
        case 0x37: // STC
            cpu->cy = 1;
            return 4;
        case 0x38: // NOP
            return 4;
        case 0x39: // DAD SP
            res_32bits = (cpu->sp + ((cpu->h << 8) | cpu->l));
            cpu->cy = ((res_32bits & 0xFFFF) > 0xFFFF); // Carry for 32 bits
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
            res_16bits = cpu->a - 1;
            // for Auxiliary carry
            cpu->ac = (((cpu->a & 0xF) - 1) > 0xF);
            check_condition_bits(cpu, 1, 1, 1, 0, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 5;
        case 0x3E: // MVI A, d8
            addr = read_memory(cpu, cpu->pc++);
            cpu->a = read_memory(cpu, addr);
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
            write_memory(cpu, ((cpu->h << 8) | cpu->l), cpu->b);
            return 7;
        case 0x71: // MOV M, C
            write_memory(cpu, ((cpu->h << 8) | cpu->l), cpu->c);
            return 7;
        case 0x72: // MOV M, D
            write_memory(cpu, ((cpu->h << 8) | cpu->l), cpu->d);
            return 7;
        case 0x73: // MOV M, E
            write_memory(cpu, ((cpu->h << 8) | cpu->l), cpu->e);
            return 7;
        case 0x74: // MOV M, H
            write_memory(cpu, ((cpu->h << 8) | cpu->l), cpu->h);
            return 7;
        case 0x75: // MOV M, L
            write_memory(cpu, ((cpu->h << 8) | cpu->l), cpu->l);
            return 7;
        case 0x76: // HLT
            cpu->halted = true;
            return 7;
        case 0x77: // MOV M, A
            write_memory(cpu, ((cpu->h << 8) | cpu->l), cpu->a);
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
            cpu->ac = (((cpu->a & 0x0F) + (cpu->l & 0x0F)) > 0x0F);
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
            cpu->ac = (((cpu->a & 0x0F) - (cpu->b & 0x0F)) > 0x0F);
            check_condition_bits(cpu, 1, 1, 1, 1, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 4;
        case 0x91: // SUB C
            res_16bits = cpu->a - cpu->c;
            cpu->ac = (((cpu->a & 0x0F) - (cpu->c & 0x0F)) > 0x0F);
            check_condition_bits(cpu, 1, 1, 1, 1, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 4;
        case 0x92: // SUB D
            res_16bits = cpu->a - cpu->d;
            cpu->ac = (((cpu->a & 0x0F) - (cpu->d & 0x0F)) > 0x0F);
            check_condition_bits(cpu, 1, 1, 1, 1, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 4;
        case 0x93: // SUB E
            res_16bits = cpu->a - cpu->e;
            cpu->ac = (((cpu->a & 0x0F) - (cpu->e & 0x0F)) > 0x0F);
            check_condition_bits(cpu, 1, 1, 1, 1, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 4;
        case 0x94: // SUB H
            res_16bits = cpu->a - cpu->h;
            cpu->ac = (((cpu->a & 0x0F) - (cpu->h & 0x0F)) > 0x0F);
            check_condition_bits(cpu, 1, 1, 1, 1, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 4;
        case 0x95: // SUB L
            res_16bits = cpu->a - cpu->l;
            cpu->ac = (((cpu->a & 0x0F) - (cpu->l & 0x0F)) > 0x0F);
            check_condition_bits(cpu, 1, 1, 1, 1, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 4;
        case 0x96: // SUB M
            lo = read_memory(cpu, ((cpu->h << 8) | cpu->l));
            res_16bits = cpu->a - lo;
            cpu->ac = (((cpu->a & 0x0F) - (lo & 0x0F)) > 0x0F);
            check_condition_bits(cpu, 1, 1, 1, 1, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 7;
        case 0x97: // SUB A
            res_16bits = cpu->a - cpu->a;
            cpu->ac = (((cpu->a & 0x0F) - (cpu->a & 0x0F)) > 0x0F);
            check_condition_bits(cpu, 1, 1, 1, 1, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 4;
        case 0x98: // SBB B
            res_16bits = cpu->a - (cpu->b + cpu->cy);
            cpu->ac = (((cpu->a & 0x0F) - ((cpu->b & 0x0F) + cpu->cy)) > 0x0F);
            check_condition_bits(cpu, 1, 1, 1, 1, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 4;
        case 0x99: // SBB C
            res_16bits = cpu->a - (cpu->c + cpu->cy);
            cpu->ac = (((cpu->a & 0x0F) - ((cpu->c & 0x0F) + cpu->cy)) > 0x0F);
            check_condition_bits(cpu, 1, 1, 1, 1, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 4;
        case 0x9A: // SBB D
            res_16bits = cpu->a - (cpu->d + cpu->cy);
            cpu->ac = (((cpu->a & 0x0F) - ((cpu->d & 0x0F) + cpu->cy)) > 0x0F);
            check_condition_bits(cpu, 1, 1, 1, 1, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 4;
        case 0x9B: // SBB E
            res_16bits = cpu->a - (cpu->e + cpu->cy);
            cpu->ac = (((cpu->a & 0x0F) - ((cpu->e & 0x0F) + cpu->cy)) > 0x0F);
            check_condition_bits(cpu, 1, 1, 1, 1, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 4;
        case 0x9C: // SBB H
            res_16bits = cpu->a - (cpu->h + cpu->cy);
            cpu->ac = (((cpu->a & 0x0F) - ((cpu->h & 0x0F) + cpu->cy)) > 0x0F);
            check_condition_bits(cpu, 1, 1, 1, 1, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 4;
        case 0x9D: // SBB L
            res_16bits = cpu->a - (cpu->l + cpu->cy);
            cpu->ac = (((cpu->a & 0x0F) - ((cpu->l & 0x0F) + cpu->cy)) > 0x0F);
            check_condition_bits(cpu, 1, 1, 1, 1, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 4;
        case 0x9E: // SBB M
            lo = read_memory(cpu, ((cpu->h << 8) | cpu->l));
            res_16bits = cpu->a - (lo + cpu->cy);
            cpu->ac = (((cpu->a & 0x0F) - ((lo & 0x0F) + cpu->cy)) > 0x0F);
            check_condition_bits(cpu, 1, 1, 1, 1, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 7;
        case 0x9F: // SBB A
            res_16bits = cpu->a - (cpu->a + cpu->cy);
            cpu->ac = (((cpu->a & 0x0F) - ((cpu->a & 0x0F) + cpu->cy)) > 0x0F);
            check_condition_bits(cpu, 1, 1, 1, 1, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 4;
        case 0xA0: // ANA B
            res_16bits = cpu->a & cpu->b;
            cpu->ac = 1;
            check_condition_bits(cpu, 1, 1, 1, 1, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 4;
        case 0xA1: // ANA C
            res_16bits = cpu->a & cpu->c;
            cpu->ac = 1;
            check_condition_bits(cpu, 1, 1, 1, 1, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 4;
        case 0xA2: // ANA D
            res_16bits = cpu->a & cpu->d;
            cpu->ac = 1;
            check_condition_bits(cpu, 1, 1, 1, 1, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 4;
        case 0xA3: // ANA E
            res_16bits = cpu->a & cpu->e;
            cpu->ac = 1;
            check_condition_bits(cpu, 1, 1, 1, 1, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 4;
        case 0xA4: // ANA H
            res_16bits = cpu->a & cpu->h;
            cpu->ac = 1;
            check_condition_bits(cpu, 1, 1, 1, 1, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 4;
        case 0xA5: // ANA L
            res_16bits = cpu->a & cpu->l;
            cpu->ac = 1;
            check_condition_bits(cpu, 1, 1, 1, 1, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 4;
        case 0xA6: // ANA M
            lo = read_memory(cpu, ((cpu->h << 8) | cpu->l));
            res_16bits = cpu->a & lo;
            cpu->ac = 1;
            check_condition_bits(cpu, 1, 1, 1, 1, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 7;
        case 0xA7: // ANA A
            res_16bits = cpu->a & cpu->a;
            cpu->ac = 1;
            check_condition_bits(cpu, 1, 1, 1, 1, res_16bits);
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
            res_16bits = cpu->a - cpu->b;
            cpu->ac = (((cpu->a & 0x0F) - (cpu->b & 0x0F)) > 0x0F);
            check_condition_bits(cpu, 1, 1, 1, 1, res_16bits);
            return 4;
        case 0xB9: // CMP C
            res_16bits = cpu->a - cpu->c;
            cpu->ac = (((cpu->a & 0x0F) - (cpu->c & 0x0F)) > 0x0F);
            check_condition_bits(cpu, 1, 1, 1, 1, res_16bits);
            return 4;
        case 0xBA: // CMP D
            res_16bits = cpu->a - cpu->d;
            cpu->ac = (((cpu->a & 0x0F) - (cpu->d & 0x0F)) > 0x0F);
            check_condition_bits(cpu, 1, 1, 1, 1, res_16bits);
            return 4;
        case 0xBB: // CMP E
            res_16bits = cpu->a - cpu->e;
            cpu->ac = (((cpu->a & 0x0F) - (cpu->e & 0x0F)) > 0x0F);
            check_condition_bits(cpu, 1, 1, 1, 1, res_16bits);
            return 4;
        case 0xBC: // CMP H
            res_16bits = cpu->a - cpu->h;
            cpu->ac = (((cpu->a & 0x0F) - (cpu->h & 0x0F)) > 0x0F);
            check_condition_bits(cpu, 1, 1, 1, 1, res_16bits);
            return 4;
        case 0xBD: // CMP L
            res_16bits = cpu->a - cpu->l;
            cpu->ac = (((cpu->a & 0x0F) - (cpu->l & 0x0F)) > 0x0F);
            check_condition_bits(cpu, 1, 1, 1, 1, res_16bits);
            return 4;
        case 0xBE: // CMP M
            lo = read_memory(cpu, ((cpu->h << 8) | cpu->l));
            res_16bits = cpu->a - lo;
            cpu->ac = (((cpu->a & 0x0F) - (lo & 0x0F)) > 0x0F);
            check_condition_bits(cpu, 1, 1, 1, 1, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 7;
        case 0xBF: // CMP A
            res_16bits = cpu->a - cpu->l;
            cpu->ac = (((cpu->a & 0x0F) - (cpu->l & 0x0F)) > 0x0F);
            check_condition_bits(cpu, 1, 1, 1, 1, res_16bits);
            return 4;
        case 0xC0: // RNZ
            if (!cpu->z)
            {
                ret(cpu);
                return 11;
            }
            return 5;
        case 0xC1: // POP B
            cpu->b = read_memory(cpu, cpu->sp++);
            return 10;
        case 0xC2: // JNZ a16
            if (!cpu->z)
            {
                lo = read_memory(cpu, cpu->pc++);
                hi = read_memory(cpu, cpu->pc++);
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
            if (!cpu->z)
            {
                lo = read_memory(cpu, cpu->pc++);
                hi = read_memory(cpu, cpu->pc++);
                addr = (lo + (hi << 8));
                call(cpu, addr);
                return 17;
            }
            return 11;
        case 0xC5: // PUSH B
            write_memory(cpu, --cpu->sp, cpu->b);
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
            if (cpu->z)
            {
                lo = read_memory(cpu, cpu->pc++);
                hi = read_memory(cpu, cpu->pc++);
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
            if (cpu->z)
            {
                lo = read_memory(cpu, cpu->pc++);
                hi = read_memory(cpu, cpu->pc++);
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
            if (!cpu->c)
            {
                ret(cpu);
                return 11;
            }
            return 5;
        case 0xD1: // POP D
            cpu->d = read_memory(cpu, cpu->sp++);
            return 10;
        case 0xD2: // JNC a16
            if (!cpu->c)
            {
                lo = read_memory(cpu, cpu->pc++);
                hi = read_memory(cpu, cpu->pc++);
                addr = (lo + (hi << 8));
                cpu->pc = addr;
            }
            return 10;
        case 0xD3: // OUT d8
            lo = read_memory(cpu, cpu->pc++);
            write_io(lo, cpu->a);
            return 10;
        case 0xD4: // CNC a16
            if (!cpu->c)
            {
                lo = read_memory(cpu, cpu->pc++);
                hi = read_memory(cpu, cpu->pc++);
                addr = (lo + (hi << 8));
                call(cpu, addr);
                return 17;
            }
            return 11;
        case 0xD5: // PUSH D
            write_memory(cpu, --cpu->sp, cpu->d);
            return 11;
        case 0xD6: // SUI d8
            lo = read_memory(cpu, cpu->pc++);
            res_16bits = cpu->a - lo;
            cpu->ac = (((cpu->a & 0x0F) - (lo & 0x0F)) > 0x0F);
            check_condition_bits(cpu, 1, 1, 1, 1, res_16bits);
            cpu->a = (uint8_t)(res_16bits & 0xFF);
            return 7;
        case 0xD7: // RST 2
            rst(cpu, 2);
            return 11;
        case 0xD8: // RC
            if (cpu->c)
            {
                ret(cpu);
                return 11;
            }
            return 5;
        case 0xD9: // RET
            ret(cpu);
            return 10;
        case 0xDA: // JC a16
            if (cpu->c)
            {
                lo = read_memory(cpu, cpu->pc++);
                hi = read_memory(cpu, cpu->pc++);
                addr = (lo + (hi << 8));
                cpu->pc = addr;
            }
            return 10;
        case 0xDB: // IN d8
            lo = read_memory(cpu, cpu->pc++);
            cpu->a = read_io(lo);
            return 10;
        case 0xDC: // CC a16
            if (cpu->c)
            {
                lo = read_memory(cpu, cpu->pc++);
                hi = read_memory(cpu, cpu->pc++);
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
            cpu->ac = (((cpu->a & 0x0F) - ((lo & 0x0F) + cpu->cy)) > 0x0F);
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
            cpu->h = read_memory(cpu, cpu->sp++);
            return 10;
        case 0xE2: // JPO a16
            if (!cpu->p)
            {
                lo = read_memory(cpu, cpu->pc++);
                hi = read_memory(cpu, cpu->pc++);
                addr = (lo + (hi << 8));
                cpu->pc = addr;
            }
            return 10;
        case 0xE3: // XTHL
            lo = read_memory(cpu, cpu->sp);
            hi = read_memory(cpu, cpu->sp+1);
            write_memory(cpu, cpu->sp, cpu->l);
            write_memory(cpu, cpu->sp+1, cpu->h);
            cpu->l = lo;
            cpu->h = hi;
            return 18;
        case 0xE4: // CPO a16
            if (!cpu->p)
            {
                lo = read_memory(cpu, cpu->pc++);
                hi = read_memory(cpu, cpu->pc++);
                addr = (lo + (hi << 8));
                call(cpu, addr);
                return 17;
            }
            return 11;
        case 0xE5: // PUSH H
            write_memory(cpu, --cpu->sp, cpu->h);
            return 11;
        case 0xE6: // ANI d8
            lo = read_memory(cpu, cpu->pc++);
            res_16bits = cpu->a & lo;
            cpu->ac = 1;
            check_condition_bits(cpu, 1, 1, 1, 1, res_16bits);
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
            if (cpu->p)
            {
                lo = read_memory(cpu, cpu->pc++);
                hi = read_memory(cpu, cpu->pc++);
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
            return 5;
        case 0xEC: // CPE a16
            if (cpu->p)
            {
                lo = read_memory(cpu, cpu->pc++);
                hi = read_memory(cpu, cpu->pc++);
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
            cpu->cy = ((lo & 0x02) > 0);
            cpu->a = read_memory(cpu, cpu->sp++);
            return 10;
        case 0xF2: // JP a16
            if (!cpu->s)
            {
                lo = read_memory(cpu, cpu->pc++);
                hi = read_memory(cpu, cpu->pc++);
                addr = (lo + (hi << 8));
                cpu->pc = addr;
            }
            return 10;
        case 0xF3: // DI
            cpu->interrupt_enable = false;
            return 4;
        case 0xF4: // CP a16
            if (!cpu->s)
            {
                lo = read_memory(cpu, cpu->pc++);
                hi = read_memory(cpu, cpu->pc++);
                addr = (lo + (hi << 8));
                call(cpu, addr);
                return 17;
            }
            return 11;
        case 0xF5: // PUSH PSW
            write_memory(cpu, --cpu->sp, get_f_flags(cpu));
            write_memory(cpu, --cpu->sp, cpu->a);
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
            if (cpu->s)
            {
                lo = read_memory(cpu, cpu->pc++);
                hi = read_memory(cpu, cpu->pc++);
                addr = (lo + (hi << 8));
                cpu->pc = addr;
            }
            return 10;
        case 0xFB: // EI
            cpu->interrupt_enable = true;
            return 4;
        case 0xFC: // CM a16
            if (cpu->s)
            {
                lo = read_memory(cpu, cpu->pc++);
                hi = read_memory(cpu, cpu->pc++);
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
            cpu->ac = (((cpu->a & 0x0F) - (lo & 0x0F)) > 0x0F);
            check_condition_bits(cpu, 1, 1, 1, 1, res_16bits);
            return 4;
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
    write_memory(cpu, --cpu->sp, ((cpu->pc >> 8) & 0xFF));
    write_memory(cpu, --cpu->sp, (cpu->pc & 0xFF));

    // For cpu.diag  a supr
    if (addr == 0x0005)
    {
        if (cpu->c == 2)
            printf("%c", cpu->e);
        if (cpu->c == 9)
        {
            uint16_t addr_pointer = ((cpu->d << 8) | cpu->e);
            uint16_t i = 0;
            while (read_memory(cpu, addr_pointer+i) != '$')
            {
                printf("%c", read_memory(cpu, addr_pointer+i));
                i++;
            }
        }
        ret(cpu);
    }
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