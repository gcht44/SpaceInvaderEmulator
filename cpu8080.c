#include "includes/cpu8080.h"
#include "includes/utils.h"
#include "memory.h"
#include <stdio.h>

void init_cpu(CPU *cpu)
{
    cpu->a = 0;
    cpu->b = 0;
    cpu->c = 0;
    cpu->d = 0;
    cpu->e = 0;
    cpu->h = 0;
    cpu->l = 0;

    cpu->pc = 0;
    cpu->sp = 0xf000;

    cpu->z = 0;
    cpu->c = 0;
    cpu->ac = 0;
    cpu->p = 0;
    cpu->s = 0;
}

// Gérer le flag ac indépendament dans l'instruction
void check_condition_bits(CPU *cpu, bool s, bool z, bool p, bool c, uint16_t data)
{
    if (z)
        cpu->z = ((data & 0xFF) == 0);
    if (c)
        cpu->cy = ((data & 0xFF) > 0xFF);
    if (s)
        cpu->s = ((data & 0x80) >> 7);
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

void print_opcode(CPU *cpu)
{
    uint8_t opcode = cpu->memory[cpu->pc];
    printf("$%04X: %s (A:%02X B:%02X C:%02X D:%02X E:%02X H:%02X L:%02X Flags:%c%c%c%c%c)", 
        cpu->pc, opcode_name(opcode), cpu->a, cpu->b, cpu->c, cpu->d, cpu->e,
        cpu->h, cpu->l, (cpu->s == 0) ? '-' : 'S', (cpu->z == 0) ? '-' : 'Z', (cpu->ac == 0) ? '-' : 'A', 
        (cpu->p == 0) ? '-' : 'P', (cpu->cy == 0) ? '-' : 'c');
}

int execute(CPU *cpu)
{
    print_opcode(cpu);
    uint8_t opcode = read_memory(cpu, cpu->pc++);

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
            new_carry = (cpu->a & 0x80) >> 7;
            cpu->a = (cpu->a << 1) | cpu->cy;
            cpu->cy = new_carry;
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
            break;
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
            // TODO: implémentation
            break;
        case 0x0F: // RRC
            // TODO: implémentation
            break;
        case 0x10: // NOP
            return 4;
        case 0x11: // LXI D, d16
            // TODO: implémentation
            break;
        case 0x12: // STAX D
            // TODO: implémentation
            break;
        case 0x13: // INX D
            // TODO: implémentation
            break;
        case 0x14: // INR D
            // TODO: implémentation
            break;
        case 0x15: // DCR D
            // TODO: implémentation
            break;
        case 0x16: // MVI D, d8
            // TODO: implémentation
            break;
        case 0x17: // RAL
            // TODO: implémentation
            break;
        case 0x18: // NOP
            return 4;
        case 0x19: // DAD D
            // TODO: implémentation
            break;
        case 0x1A: // LDAX D
            // TODO: implémentation
            break;
        case 0x1B: // DCX D
            // TODO: implémentation
            break;
        case 0x1C: // INR E
            // TODO: implémentation
            break;
        case 0x1D: // DCR E
            // TODO: implémentation
            break;
        case 0x1E: // MVI E, d8
            // TODO: implémentation
            break;
        case 0x1F: // RAR
            // TODO: implémentation
            break;
        case 0x20: // NOP
            return 4;
        case 0x21: // LXI H, d16
            // TODO: implémentation
            break;
        case 0x22: // SHLD a16
            // TODO: implémentation
            break;
        case 0x23: // INX H
            // TODO: implémentation
            break;
        case 0x24: // INR H
            // TODO: implémentation
            break;
        case 0x25: // DCR H
            // TODO: implémentation
            break;
        case 0x26: // MVI H, d8
            // TODO: implémentation
            break;
        case 0x27: // DAA
            // TODO: implémentation
            break;
        case 0x28: // NOP
            return 4;
        case 0x29: // DAD H
            // TODO: implémentation
            break;
        case 0x2A: // LHLD a16
            // TODO: implémentation
            break;
        case 0x2B: // DCX H
            // TODO: implémentation
            break;
        case 0x2C: // INR L
            // TODO: implémentation
            break;
        case 0x2D: // DCR L
            // TODO: implémentation
            break;
        case 0x2E: // MVI L, d8
            // TODO: implémentation
            break;
        case 0x2F: // CMA
            // TODO: implémentation
            break;
        case 0x30: // NOP
            return 4;
        case 0x31: // LXI SP, d16
            // TODO: implémentation
            break;
        case 0x32: // STA a16
            // TODO: implémentation
            break;
        case 0x33: // INX SP
            // TODO: implémentation
            break;
        case 0x34: // INR M
            // TODO: implémentation
            break;
        case 0x35: // DCR M
            // TODO: implémentation
            break;
        case 0x36: // MVI M, d8
            // TODO: implémentation
            break;
        case 0x37: // STC
            // TODO: implémentation
            break;
        case 0x38: // NOP
            return 4;
        case 0x39: // DAD SP
            // TODO: implémentation
            break;
        case 0x3A: // LDA a16
            // TODO: implémentation
            break;
        case 0x3B: // DCX SP
            // TODO: implémentation
            break;
        case 0x3C: // INR A
            // TODO: implémentation
            break;
        case 0x3D: // DCR A
            // TODO: implémentation
            break;
        case 0x3E: // MVI A, d8
            // TODO: implémentation
            break;
        case 0x3F: // CMC
            // TODO: implémentation
            break;
        case 0x40: // MOV B, B
            // TODO: implémentation
            break;
        case 0x41: // MOV B, C
            // TODO: implémentation
            break;
        case 0x42: // MOV B, D
            // TODO: implémentation
            break;
        case 0x43: // MOV B, E
            // TODO: implémentation
            break;
        case 0x44: // MOV B, H
            // TODO: implémentation
            break;
        case 0x45: // MOV B, L
            // TODO: implémentation
            break;
        case 0x46: // MOV B, M
            // TODO: implémentation
            break;
        case 0x47: // MOV B, A
            // TODO: implémentation
            break;
        case 0x48: // MOV C, B
            // TODO: implémentation
            break;
        case 0x49: // MOV C, C
            // TODO: implémentation
            break;
        case 0x4A: // MOV C, D
            // TODO: implémentation
            break;
        case 0x4B: // MOV C, E
            // TODO: implémentation
            break;
        case 0x4C: // MOV C, H
            // TODO: implémentation
            break;
        case 0x4D: // MOV C, L
            // TODO: implémentation
            break;
        case 0x4E: // MOV C, M
            // TODO: implémentation
            break;
        case 0x4F: // MOV C, A
            // TODO: implémentation
            break;
        case 0x50: // MOV D, B
            // TODO: implémentation
            break;
        case 0x51: // MOV D, C
            // TODO: implémentation
            break;
        case 0x52: // MOV D, D
            // TODO: implémentation
            break;
        case 0x53: // MOV D, E
            // TODO: implémentation
            break;
        case 0x54: // MOV D, H
            // TODO: implémentation
            break;
        case 0x55: // MOV D, L
            // TODO: implémentation
            break;
        case 0x56: // MOV D, M
            // TODO: implémentation
            break;
        case 0x57: // MOV D, A
            // TODO: implémentation
            break;
        case 0x58: // MOV E, B
            // TODO: implémentation
            break;
        case 0x59: // MOV E, C
            // TODO: implémentation
            break;
        case 0x5A: // MOV E, D
            // TODO: implémentation
            break;
        case 0x5B: // MOV E, E
            // TODO: implémentation
            break;
        case 0x5C: // MOV E, H
            // TODO: implémentation
            break;
        case 0x5D: // MOV E, L
            // TODO: implémentation
            break;
        case 0x5E: // MOV E, M
            // TODO: implémentation
            break;
        case 0x5F: // MOV E, A
            // TODO: implémentation
            break;
        case 0x60: // MOV H, B
            // TODO: implémentation
            break;
        case 0x61: // MOV H, C
            // TODO: implémentation
            break;
        case 0x62: // MOV H, D
            // TODO: implémentation
            break;
        case 0x63: // MOV H, E
            // TODO: implémentation
            break;
        case 0x64: // MOV H, H
            // TODO: implémentation
            break;
        case 0x65: // MOV H, L
            // TODO: implémentation
            break;
        case 0x66: // MOV H, M
            // TODO: implémentation
            break;
        case 0x67: // MOV H, A
            // TODO: implémentation
            break;
        case 0x68: // MOV L, B
            // TODO: implémentation
            break;
        case 0x69: // MOV L, C
            // TODO: implémentation
            break;
        case 0x6A: // MOV L, D
            // TODO: implémentation
            break;
        case 0x6B: // MOV L, E
            // TODO: implémentation
            break;
        case 0x6C: // MOV L, H
            // TODO: implémentation
            break;
        case 0x6D: // MOV L, L
            // TODO: implémentation
            break;
        case 0x6E: // MOV L, M
            // TODO: implémentation
            break;
        case 0x6F: // MOV L, A
            // TODO: implémentation
            break;
        case 0x70: // MOV M, B
            // TODO: implémentation
            break;
        case 0x71: // MOV M, C
            // TODO: implémentation
            break;
        case 0x72: // MOV M, D
            // TODO: implémentation
            break;
        case 0x73: // MOV M, E
            // TODO: implémentation
            break;
        case 0x74: // MOV M, H
            // TODO: implémentation
            break;
        case 0x75: // MOV M, L
            // TODO: implémentation
            break;
        case 0x76: // HLT
            // TODO: implémentation
            break;
        case 0x77: // MOV M, A
            // TODO: implémentation
            break;
        case 0x78: // MOV A, B
            // TODO: implémentation
            break;
        case 0x79: // MOV A, C
            // TODO: implémentation
            break;
        case 0x7A: // MOV A, D
            // TODO: implémentation
            break;
        case 0x7B: // MOV A, E
            // TODO: implémentation
            break;
        case 0x7C: // MOV A, H
            // TODO: implémentation
            break;
        case 0x7D: // MOV A, L
            // TODO: implémentation
            break;
        case 0x7E: // MOV A, M
            // TODO: implémentation
            break;
        case 0x7F: // MOV A, A
            // TODO: implémentation
            break;
        case 0x80: // ADD B
            // TODO: implémentation
            break;
        case 0x81: // ADD C
            // TODO: implémentation
            break;
        case 0x82: // ADD D
            // TODO: implémentation
            break;
        case 0x83: // ADD E
            // TODO: implémentation
            break;
        case 0x84: // ADD H
            // TODO: implémentation
            break;
        case 0x85: // ADD L
            // TODO: implémentation
            break;
        case 0x86: // ADD M
            // TODO: implémentation
            break;
        case 0x87: // ADD A
            // TODO: implémentation
            break;
        case 0x88: // ADC B
            // TODO: implémentation
            break;
        case 0x89: // ADC C
            // TODO: implémentation
            break;
        case 0x8A: // ADC D
            // TODO: implémentation
            break;
        case 0x8B: // ADC E
            // TODO: implémentation
            break;
        case 0x8C: // ADC H
            // TODO: implémentation
            break;
        case 0x8D: // ADC L
            // TODO: implémentation
            break;
        case 0x8E: // ADC M
            // TODO: implémentation
            break;
        case 0x8F: // ADC A
            // TODO: implémentation
            break;
        case 0x90: // SUB B
            // TODO: implémentation
            break;
        case 0x91: // SUB C
            // TODO: implémentation
            break;
        case 0x92: // SUB D
            // TODO: implémentation
            break;
        case 0x93: // SUB E
            // TODO: implémentation
            break;
        case 0x94: // SUB H
            // TODO: implémentation
            break;
        case 0x95: // SUB L
            // TODO: implémentation
            break;
        case 0x96: // SUB M
            // TODO: implémentation
            break;
        case 0x97: // SUB A
            // TODO: implémentation
            break;
        case 0x98: // SBB B
            // TODO: implémentation
            break;
        case 0x99: // SBB C
            // TODO: implémentation
            break;
        case 0x9A: // SBB D
            // TODO: implémentation
            break;
        case 0x9B: // SBB E
            // TODO: implémentation
            break;
        case 0x9C: // SBB H
            // TODO: implémentation
            break;
        case 0x9D: // SBB L
            // TODO: implémentation
            break;
        case 0x9E: // SBB M
            // TODO: implémentation
            break;
        case 0x9F: // SBB A
            // TODO: implémentation
            break;
        case 0xA0: // ANA B
            // TODO: implémentation
            break;
        case 0xA1: // ANA C
            // TODO: implémentation
            break;
        case 0xA2: // ANA D
            // TODO: implémentation
            break;
        case 0xA3: // ANA E
            // TODO: implémentation
            break;
        case 0xA4: // ANA H
            // TODO: implémentation
            break;
        case 0xA5: // ANA L
            // TODO: implémentation
            break;
        case 0xA6: // ANA M
            // TODO: implémentation
            break;
        case 0xA7: // ANA A
            // TODO: implémentation
            break;
        case 0xA8: // XRA B
            // TODO: implémentation
            break;
        case 0xA9: // XRA C
            // TODO: implémentation
            break;
        case 0xAA: // XRA D
            // TODO: implémentation
            break;
        case 0xAB: // XRA E
            // TODO: implémentation
            break;
        case 0xAC: // XRA H
            // TODO: implémentation
            break;
        case 0xAD: // XRA L
            // TODO: implémentation
            break;
        case 0xAE: // XRA M
            // TODO: implémentation
            break;
        case 0xAF: // XRA A
            // TODO: implémentation
            break;
        case 0xB0: // ORA B
            // TODO: implémentation
            break;
        case 0xB1: // ORA C
            // TODO: implémentation
            break;
        case 0xB2: // ORA D
            // TODO: implémentation
            break;
        case 0xB3: // ORA E
            // TODO: implémentation
            break;
        case 0xB4: // ORA H
            // TODO: implémentation
            break;
        case 0xB5: // ORA L
            // TODO: implémentation
            break;
        case 0xB6: // ORA M
            // TODO: implémentation
            break;
        case 0xB7: // ORA A
            // TODO: implémentation
            break;
        case 0xB8: // CMP B
            // TODO: implémentation
            break;
        case 0xB9: // CMP C
            // TODO: implémentation
            break;
        case 0xBA: // CMP D
            // TODO: implémentation
            break;
        case 0xBB: // CMP E
            // TODO: implémentation
            break;
        case 0xBC: // CMP H
            // TODO: implémentation
            break;
        case 0xBD: // CMP L
            // TODO: implémentation
            break;
        case 0xBE: // CMP M
            // TODO: implémentation
            break;
        case 0xBF: // CMP A
            // TODO: implémentation
            break;
        case 0xC0: // RNZ
            // TODO: implémentation
            break;
        case 0xC1: // POP B
            // TODO: implémentation
            break;
        case 0xC2: // JNZ a16
            // TODO: implémentation
            break;
        case 0xC3: // JMP a16
            // TODO: implémentation
            break;
        case 0xC4: // CNZ a16
            // TODO: implémentation
            break;
        case 0xC5: // PUSH B
            // TODO: implémentation
            break;
        case 0xC6: // ADI d8
            // TODO: implémentation
            break;
        case 0xC7: // RST 0
            // TODO: implémentation
            break;
        case 0xC8: // RZ
            // TODO: implémentation
            break;
        case 0xC9: // RET
            // TODO: implémentation
            break;
        case 0xCA: // JZ a16
            // TODO: implémentation
            break;
        case 0xCB: // NOP
            return 4;
        case 0xCC: // CZ a16
            // TODO: implémentation
            break;
        case 0xCD: // CALL a16
            // TODO: implémentation
            break;
        case 0xCE: // ACI d8
            // TODO: implémentation
            break;
        case 0xCF: // RST 1
            // TODO: implémentation
            break;
        case 0xD0: // RNC
            // TODO: implémentation
            break;
        case 0xD1: // POP D
            // TODO: implémentation
            break;
        case 0xD2: // JNC a16
            // TODO: implémentation
            break;
        case 0xD3: // OUT d8
            // TODO: implémentation
            break;
        case 0xD4: // CNC a16
            // TODO: implémentation
            break;
        case 0xD5: // PUSH D
            // TODO: implémentation
            break;
        case 0xD6: // SUI d8
            // TODO: implémentation
            break;
        case 0xD7: // RST 2
            // TODO: implémentation
            break;
        case 0xD8: // RC
            // TODO: implémentation
            break;
        case 0xD9: // NOP
            return 4;
        case 0xDA: // JC a16
            // TODO: implémentation
            break;
        case 0xDB: // IN d8
            // TODO: implémentation
            break;
        case 0xDC: // CC a16
            // TODO: implémentation
            break;
        case 0xDD: // NOP
            return 4;
        case 0xDE: // SBI d8
            // TODO: implémentation
            break;
        case 0xDF: // RST 3
            // TODO: implémentation
            break;
        case 0xE0: // RPO
            // TODO: implémentation
            break;
        case 0xE1: // POP H
            // TODO: implémentation
            break;
        case 0xE2: // JPO a16
            // TODO: implémentation
            break;
        case 0xE3: // XTHL
            // TODO: implémentation
            break;
        case 0xE4: // CPO a16
            // TODO: implémentation
            break;
        case 0xE5: // PUSH H
            // TODO: implémentation
            break;
        case 0xE6: // ANI d8
            // TODO: implémentation
            break;
        case 0xE7: // RST 4
            // TODO: implémentation
            break;
        case 0xE8: // RPE
            // TODO: implémentation
            break;
        case 0xE9: // PCHL
            // TODO: implémentation
            break;
        case 0xEA: // JPE a16
            // TODO: implémentation
            break;
        case 0xEB: // XCHG
            // TODO: implémentation
            break;
        case 0xEC: // CPE a16
            // TODO: implémentation
            break;
        case 0xED: // NOP
            return 4;
        case 0xEE: // XRI d8
            // TODO: implémentation
            break;
        case 0xEF: // RST 5
            // TODO: implémentation
            break;
        case 0xF0: // RP
            // TODO: implémentation
            break;
        case 0xF1: // POP PSW
            // TODO: implémentation
            break;
        case 0xF2: // JP a16
            // TODO: implémentation
            break;
        case 0xF3: // DI
            // TODO: implémentation
            break;
        case 0xF4: // CP a16
            // TODO: implémentation
            break;
        case 0xF5: // PUSH PSW
            // TODO: implémentation
            break;
        case 0xF6: // ORI d8
            // TODO: implémentation
            break;
        case 0xF7: // RST 6
            // TODO: implémentation
            break;
        case 0xF8: // RM
            // TODO: implémentation
            break;
        case 0xF9: // SPHL
            // TODO: implémentation
            break;
        case 0xFA: // JM a16
            // TODO: implémentation
            break;
        case 0xFB: // EI
            // TODO: implémentation
            break;
        case 0xFC: // CM a16
            // TODO: implémentation
            break;
        case 0xFD: // NOP
            return 4;
        case 0xFE: // CPI d8
            // TODO: implémentation
            break;
        case 0xFF: // RST 7
            // TODO: implémentation
            break;
        default:
            printf("Opcode inconnu: 0x%02X\n", opcode);
            return -1;
    }
    return 0;
}

void lxi()
{

}