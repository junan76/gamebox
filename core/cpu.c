#include <stdint.h>
#include <stddef.h>
#include "bus.h"

struct cpu_struct
{
    /*
     * Register file of CPU, ONLY support little-endian for now.
     */
    struct
    {
        union
        {
            uint16_t af;
            struct
            {
                uint8_t __padding : 4;
                uint8_t c_flag : 1;
                uint8_t h_flag : 1;
                uint8_t n_flag : 1;
                uint8_t z_flag : 1;
                uint8_t a;
            };
        };
        union
        {
            uint16_t bc;
            struct
            {
                uint8_t c;
                uint8_t b;
            };
        };
        union
        {
            uint16_t de;
            struct
            {
                uint8_t e;
                uint8_t d;
            };
        };
        union
        {
            uint16_t hl;
            struct
            {
                uint8_t l;
                uint8_t h;
            };
        };
        uint16_t sp;
        uint16_t pc;
    } regs;

    /*Interrupt master enable flag*/
    uint8_t ime;
    /*
     * Interrupt enable, reg address: 0xFFFF
     * bit 0: vblank
     * bit 1: lcd
     * bit 2: timer
     * bit 3: serial
     * bit 4: joypad
     * bits[7:5]: NONE
     */
    uint8_t ie;

    uint8_t halted;
    uint8_t stopped;
};

static struct cpu_struct sm83_cpu;
struct cpu_struct *cpu;

static uint8_t *cpu_reg8(uint8_t encode)
{
    switch (encode)
    {
    case 0:
        return &cpu->regs.b;
    case 1:
        return &cpu->regs.c;
    case 2:
        return &cpu->regs.d;
    case 3:
        return &cpu->regs.e;
    case 4:
        return &cpu->regs.h;
    case 5:
        return &cpu->regs.l;
    case 7:
        return &cpu->regs.a;
    default:
        return NULL;
    }
}

/*
 * 8-bit load instructions.
 */

static uint8_t ld_r8_r8(uint8_t opcode)
{
    uint8_t *rs, *rd;
    rs = cpu_reg8(opcode & 0x7);
    rd = cpu_reg8((opcode >> 3) & 0x7);

    *rd = *rs;
    cpu->regs.pc++;
    return 4;
}

static uint8_t ld_r8_n8(uint8_t opcode)
{
    uint8_t *rd;
    uint8_t n8;

    rd = cpu_reg8((opcode >> 3) & 0x7);
    n8 = bus_read8(cpu->regs.pc++);

    if (rd != NULL)
    {
        *rd = n8;
        return 8;
    }
    else
    {
        bus_write8(cpu->regs.hl, n8);
        return 12;
    }
}