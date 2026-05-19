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

static uint8_t ill_op(uint8_t opcode)
{
    cpu->stopped = 1;
    return 0;
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
    return 4;
}

static uint8_t ld_r8_n8(uint8_t opcode)
{
    uint8_t *rd;
    uint8_t n8;

    rd = cpu_reg8((opcode >> 3) & 0x7);
    n8 = bus_read8(cpu->regs.pc++);

    *rd = n8;
    return 8;
}

static uint8_t ld_r8_hl(uint8_t opcode)
{
    uint8_t *rd = cpu_reg8((opcode >> 3) & 0x7);
    *rd = bus_read8(cpu->regs.hl);
    return 8;
}

static uint8_t ld_hl_r8(uint8_t opcode)
{
    uint8_t *rs = cpu_reg8(opcode & 0x7);
    bus_write8(cpu->regs.hl, *rs);
    return 8;
}

static uint8_t ld_hl_n8(uint8_t opcode)
{
    uint8_t value = bus_read8(cpu->regs.pc++);
    bus_write8(cpu->regs.hl, value);
    return 12;
}

static uint8_t ld_a_r16(uint8_t opcode)
{
    if (opcode == 0x0a)
    {
        cpu->regs.a = bus_read8(cpu->regs.bc);
    }
    else if (opcode == 0x1a)
    {
        cpu->regs.a = bus_read8(cpu->regs.de);
    }
    return 8;
}

static uint8_t ld_r16_a(uint8_t opcode)
{
    if (opcode == 0x02)
    {
        bus_write8(cpu->regs.bc, cpu->regs.a);
    }
    else if (opcode == 0x12)
    {
        bus_write8(cpu->regs.de, cpu->regs.a);
    }
    return 8;
}

static uint8_t ld_a_n16(uint8_t opcode)
{
    uint16_t addr = bus_read16(cpu->regs.pc);
    cpu->regs.a = bus_read8(addr);
    cpu->regs.pc += 2;
    return 16;
}

static uint8_t ld_n16_a(uint8_t opcode)
{
    uint16_t addr = bus_read16(cpu->regs.pc);
    bus_write8(addr, cpu->regs.a);
    cpu->regs.pc += 2;
    return 16;
}

static uint8_t ldh_a_c(uint8_t opcode)
{
    cpu->regs.a = bus_read8(0xFF00 + cpu->regs.c);
    return 8;
}

static uint8_t ldh_c_a(uint8_t opcode)
{
    bus_write8(0xFF00 + cpu->regs.c, cpu->regs.a);
    return 8;
}

static uint8_t ldh_a_n16(uint8_t opcode)
{
    uint8_t low = bus_read8(cpu->regs.pc);
    uint16_t addr = 0xFF00 + low;
    cpu->regs.a = bus_read8(addr);
    cpu->regs.pc++;
    return 12;
}

static uint8_t ldh_n16_a(uint8_t opcode)
{
    uint8_t low = bus_read8(cpu->regs.pc);
    uint16_t addr = 0xFF00 + low;
    bus_write8(addr, cpu->regs.a);
    cpu->regs.pc++;
    return 12;
}

static uint8_t ldi_a_hl(uint8_t opcode)
{
    cpu->regs.a = bus_read8(cpu->regs.hl++);
    return 8;
}

static uint8_t ldi_hl_a(uint8_t opcode)
{
    bus_write8(cpu->regs.hl++, cpu->regs.a);
    return 8;
}

static uint8_t ldd_a_hl(uint8_t opcode)
{
    cpu->regs.a = bus_read8(cpu->regs.hl--);
    return 8;
}

static uint8_t ldd_hl_a(uint8_t opcode)
{
    bus_write8(cpu->regs.hl--, cpu->regs.a);
    return 8;
}

/*
 * DOING:
 * 16-bit load instructions.
 */
static uint8_t ld_r16_n16(uint8_t opcode)
{
    uint16_t value = bus_read16(cpu->regs.pc);

    switch (opcode)
    {
    case 0x01:
        cpu->regs.bc = value;
        break;
    case 0x11:
        cpu->regs.de = value;
        break;
    case 0x21:
        cpu->regs.hl = value;
        break;
    case 0x31:
        cpu->regs.sp = value;
        break;
    }

    cpu->regs.pc += 2;
    return 12;
}

/*
 * TODO:
 * 8-bit arithmetic and logical instructions.
 */

/*
 * TODO:
 * 16-bit arithmetic instructions.
 */

/*
 * TODO:
 * Control flow instructions.
 */

/*
 * TODO:
 * Miscellaneous instructions.
 */

typedef uint8_t (*opcode_handler)(uint8_t);
opcode_handler opcode_handlers[256] = {
    ill_op, /*0x00 */
    ill_op, /*0x01 */
    ill_op, /*0x02 */
    ill_op, /*0x03 */
    ill_op, /*0x04 */
    ill_op, /*0x05 */
    ill_op, /*0x06 */
    ill_op, /*0x07 */
    ill_op, /*0x08 */
    ill_op, /*0x09 */
    ill_op, /*0x0a */
    ill_op, /*0x0b */
    ill_op, /*0x0c */
    ill_op, /*0x0d */
    ill_op, /*0x0e */
    ill_op, /*0x0f */

};