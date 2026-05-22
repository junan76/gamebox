#include <stdint.h>
#include <stddef.h>
#include "bus.h"

struct cpu_struct {
	/**
     * Register file of CPU, ONLY support little-endian for now.
     */
	struct {
		union {
			uint16_t af;
			struct {
				uint8_t _padding : 4;
				uint8_t c_flag : 1;
				uint8_t h_flag : 1;
				uint8_t n_flag : 1;
				uint8_t z_flag : 1;
				uint8_t a;
			};
		};
		union {
			uint16_t bc;
			struct {
				uint8_t c;
				uint8_t b;
			};
		};
		union {
			uint16_t de;
			struct {
				uint8_t e;
				uint8_t d;
			};
		};
		union {
			uint16_t hl;
			struct {
				uint8_t l;
				uint8_t h;
			};
		};
		uint16_t sp;
		uint16_t pc;
	} regs;

	/*Interrupt master enable flag*/
	uint8_t ime;
	/**
     * Interrupt enable, reg address: 0xFFFF
     * bits[7:5]: NONE
     * bit 4: joypad
     * bit 3: serial
     * bit 2: timer
     * bit 1: lcd
	 * bit 0: vblank
     */
	uint8_t ie;
	/**
	 * Interrult requested flag, reg address 0xFF0F
     * bits[7:5]: NONE
     * bit 4: joypad
     * bit 3: serial
     * bit 2: timer
     * bit 1: lcd
	 * bit 0: vblank
	 */
	uint8_t irq;

	uint8_t halted;
	uint8_t halt_bug;
	uint8_t stopped;
};

static struct cpu_struct sm83_cpu;
struct cpu_struct *cpu = &sm83_cpu;

static uint8_t *cpu_reg8(uint8_t encode)
{
	switch (encode) {
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
	if (opcode == 0x0A) {
		cpu->regs.a = bus_read8(cpu->regs.bc);
	} else if (opcode == 0x1A) {
		cpu->regs.a = bus_read8(cpu->regs.de);
	}
	return 8;
}

static uint8_t ld_r16_a(uint8_t opcode)
{
	if (opcode == 0x02) {
		bus_write8(cpu->regs.bc, cpu->regs.a);
	} else if (opcode == 0x12) {
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

static uint8_t ldh_a_n8(uint8_t opcode)
{
	uint8_t low = bus_read8(cpu->regs.pc++);
	cpu->regs.a = bus_read8(0xFF00 + low);
	return 12;
}

static uint8_t ldh_n8_a(uint8_t opcode)
{
	uint8_t low = bus_read8(cpu->regs.pc++);
	bus_write8(0xFF00 + low, cpu->regs.a);
	return 12;
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

/**
 * 16-bit load instructions.
 */
static uint8_t ld_r16_n16(uint8_t opcode)
{
	uint16_t value = bus_read16(cpu->regs.pc);

	switch (opcode) {
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

static uint8_t ld_n16_sp(uint8_t opcode)
{
	uint16_t addr = bus_read16(cpu->regs.pc);
	bus_write16(addr, cpu->regs.sp);
	cpu->regs.pc += 2;
	return 20;
}

static uint8_t ld_sp_hl(uint8_t opcode)
{
	cpu->regs.sp = cpu->regs.hl;
	return 8;
}

static uint8_t push_r16(uint8_t opcode)
{
	cpu->regs.sp -= 2;
	switch (opcode) {
	case 0xC5:
		bus_write16(cpu->regs.sp, cpu->regs.bc);
		break;
	case 0xD5:
		bus_write16(cpu->regs.sp, cpu->regs.de);
		break;
	case 0xE5:
		bus_write16(cpu->regs.sp, cpu->regs.hl);
		break;
	case 0xF5:
		bus_write16(cpu->regs.sp, cpu->regs.af);
		break;
	}
	return 16;
}

static uint8_t pop_r16(uint8_t opcode)
{
	switch (opcode) {
	case 0xC1:
		cpu->regs.bc = bus_read16(cpu->regs.sp);
		break;
	case 0xD1:
		cpu->regs.de = bus_read16(cpu->regs.sp);
		break;
	case 0xE1:
		cpu->regs.hl = bus_read16(cpu->regs.sp);
		break;
	case 0xF1:
		cpu->regs.af = bus_read16(cpu->regs.sp);
		cpu->regs._padding = 0;
		break;
	}
	cpu->regs.sp += 2;
	return 12;
}

static uint8_t ld_hl_sp_e8(uint8_t opcode)
{
	int8_t e8 = bus_read8(cpu->regs.pc++);
	uint16_t result = cpu->regs.sp + e8;
	cpu->regs.hl = result;

	cpu->regs.z_flag = 0;
	cpu->regs.n_flag = 0;
	uint16_t carry_bit = (cpu->regs.sp & e8) |
			     ((cpu->regs.sp ^ e8) & ~result);
	cpu->regs.c_flag = !!(carry_bit & 0x80);
	cpu->regs.h_flag = !!(carry_bit & 0x08);
	return 12;
}

/**
 * TODO:
 * 8-bit arithmetic and logical instructions.
 */

/**
 * TODO:
 * 16-bit arithmetic instructions.
 */

/**
 * Control flow instructions.
 */
static uint8_t jp_n16(uint8_t opcode)
{
	cpu->regs.pc = bus_read16(cpu->regs.pc);
	return 16;
}

static uint8_t jp_hl(uint8_t opcode)
{
	cpu->regs.pc = cpu->regs.hl;
	return 4;
}

static uint8_t jp_cc_n16(uint8_t opcode)
{
	uint16_t jp_addr = bus_read16(cpu->regs.pc);
	cpu->regs.pc += 2;

	switch (opcode) {
	case 0xC2:
		if (!cpu->regs.z_flag) {
			cpu->regs.pc = jp_addr;
			return 16;
		}
		break;
	case 0xCA:
		if (cpu->regs.z_flag) {
			cpu->regs.pc = jp_addr;
			return 16;
		}
		break;
	case 0xD2:
		if (!cpu->regs.c_flag) {
			cpu->regs.pc = jp_addr;
			return 16;
		}
		break;
	case 0xDA:
		if (cpu->regs.c_flag) {
			cpu->regs.pc = jp_addr;
			return 16;
		}
		break;
	}

	return 12;
}

static uint8_t jr_e8(uint8_t opcode)
{
	int8_t e8 = bus_read8(cpu->regs.pc++);
	cpu->regs.pc += e8;
	return 12;
}

static uint8_t jr_cc_e8(uint8_t opcode)
{
	int8_t e8 = bus_read8(cpu->regs.pc++);

	switch (opcode) {
	case 0x20:
		if (!cpu->regs.z_flag) {
			cpu->regs.pc += e8;
			return 12;
		}
		break;
	case 0x28:
		if (cpu->regs.z_flag) {
			cpu->regs.pc += e8;
			return 12;
		}
		break;
	case 0x30:
		if (!cpu->regs.c_flag) {
			cpu->regs.pc += e8;
			return 12;
		}
		break;
	case 0x38:
		if (cpu->regs.c_flag) {
			cpu->regs.pc += e8;
			return 12;
		}
		break;
	}

	return 8;
}

static uint8_t call_n16(uint8_t opcode)
{
	uint16_t call_addr = bus_read16(cpu->regs.pc);
	cpu->regs.pc += 2;

	cpu->regs.sp -= 2;
	bus_write16(cpu->regs.sp, cpu->regs.pc);
	cpu->regs.pc = call_addr;

	return 24;
}

static uint8_t call_cc_n16(uint8_t opcode)
{
	uint16_t call_addr = bus_read16(cpu->regs.pc);
	cpu->regs.pc += 2;

	switch (opcode) {
	case 0xC4:
		if (!cpu->regs.z_flag) {
			cpu->regs.sp -= 2;
			bus_write16(cpu->regs.sp, cpu->regs.pc);
			cpu->regs.pc = call_addr;
			return 24;
		}
		break;
	case 0xCC:
		if (cpu->regs.z_flag) {
			cpu->regs.sp -= 2;
			bus_write16(cpu->regs.sp, cpu->regs.pc);
			cpu->regs.pc = call_addr;
			return 24;
		}
		break;
	case 0xD4:
		if (!cpu->regs.c_flag) {
			cpu->regs.sp -= 2;
			bus_write16(cpu->regs.sp, cpu->regs.pc);
			cpu->regs.pc = call_addr;
			return 24;
		}
		break;
	case 0xDC:
		if (cpu->regs.c_flag) {
			cpu->regs.sp -= 2;
			bus_write16(cpu->regs.sp, cpu->regs.pc);
			cpu->regs.pc = call_addr;
			return 24;
		}
		break;
	}

	return 12;
}

static uint8_t ret(uint8_t opcode)
{
	cpu->regs.pc = bus_read16(cpu->regs.sp);
	cpu->regs.sp += 2;
	return 16;
}

static uint8_t ret_cc(uint8_t opcode)
{
	uint16_t ret_addr = bus_read16(cpu->regs.sp);

	switch (opcode) {
	case 0xC0:
		if (!cpu->regs.z_flag) {
			cpu->regs.sp += 2;
			cpu->regs.pc = ret_addr;
			return 20;
		}
		break;
	case 0xC8:
		if (cpu->regs.z_flag) {
			cpu->regs.sp += 2;
			cpu->regs.pc = ret_addr;
			return 20;
		}
		break;
	case 0xD0:
		if (!cpu->regs.c_flag) {
			cpu->regs.sp += 2;
			cpu->regs.pc = ret_addr;
			return 20;
		}
		break;
	case 0xD8:
		if (cpu->regs.c_flag) {
			cpu->regs.sp += 2;
			cpu->regs.pc = ret_addr;
			return 20;
		}
		break;
	}

	return 8;
}

static uint8_t reti(uint8_t opcode)
{
	cpu->regs.pc = bus_read16(cpu->regs.sp);
	cpu->regs.sp += 2;
	cpu->ime = 1;
	return 16;
}

static uint8_t rst(uint8_t opcode)
{
	cpu->regs.sp -= 2;
	bus_write16(cpu->regs.sp, cpu->regs.pc);
	cpu->regs.pc = opcode & 0x38;
	return 16;
}

/**
 * Miscellaneous instructions.
 */
static uint8_t halt(uint8_t opcode)
{
	if (cpu->ime == 0 && (cpu->ie & cpu->irq & 0x1F)) {
		cpu->halt_bug = 1;
	} else {
		cpu->halted = 1;
	}
	return 4;
}

static uint8_t stop(uint8_t opcode)
{
	cpu->stopped = 1;
	return 4;
}

static uint8_t di(uint8_t opcode)
{
	cpu->ime = 0;
	return 4;
}

static uint8_t ei(uint8_t opcode)
{
	cpu->ime = 1;
	return 4;
}

static uint8_t nop(uint8_t opcode)
{
	return 4;
}

/**
 * TODO:
 * Fill opcode_handlers table
 */
typedef uint8_t (*opcode_handler)(uint8_t);
static const opcode_handler opcode_handlers[256] = {
	/*0x0_*/
	[0x00] = nop,
	[0x01] = ld_r16_n16,
	[0x02] = ld_r16_a,
	[0x06] = ld_r8_n8,
	[0x08] = ld_n16_sp,
	[0x0A] = ld_a_r16,
	[0x0E] = ld_r8_n8,

	/*0x1_*/
	[0x10] = stop,
	[0x11] = ld_r16_n16,
	[0x12] = ld_r16_a,
	[0x16] = ld_r8_n8,
	[0x18] = jr_e8,
	[0x1A] = ld_a_r16,
	[0x1E] = ld_r8_n8,

	/*0x2_*/
	[0x20] = jr_cc_e8,
	[0x21] = ld_r16_n16,
	[0x22] = ldi_hl_a,
	[0x26] = ld_r8_n8,
	[0x28] = jr_cc_e8,
	[0x2A] = ldi_a_hl,
	[0x2E] = ld_r8_n8,

	/*0x3_*/
	[0x30] = jr_cc_e8,
	[0x31] = ld_r16_n16,
	[0x32] = ldd_hl_a,
	[0x36] = ld_hl_n8,
	[0x38] = jr_cc_e8,
	[0x3A] = ldd_a_hl,
	[0x3E] = ld_r8_n8,

	/*0x4_*/
	[0x40 ... 0x45] = ld_r8_r8,
	[0x46] = ld_r8_hl,
	[0x47 ... 0x4D] = ld_r8_r8,
	[0x4E] = ld_r8_hl,
	[0x4F] = ld_r8_r8,

	/*0x5_*/
	[0x50 ... 0x55] = ld_r8_r8,
	[0x56] = ld_r8_hl,
	[0x57 ... 0x5D] = ld_r8_r8,
	[0x5E] = ld_r8_hl,
	[0x5F] = ld_r8_r8,

	/*0x6_*/
	[0x60 ... 0x65] = ld_r8_r8,
	[0x66] = ld_r8_hl,
	[0x67 ... 0x6D] = ld_r8_r8,
	[0x6E] = ld_r8_hl,
	[0x6F] = ld_r8_r8,

	/*0x7_*/
	[0x70 ... 0x75] = ld_hl_r8,
	[0x76] = halt,
	[0x77] = ld_hl_r8,
	[0x78 ... 0x7D] = ld_r8_r8,
	[0x7E] = ld_r8_hl,
	[0x7F] = ld_r8_r8,

	/*0x8_*/

	/*0x9_*/

	/*0xA_*/

	/*0xB_*/

	/*0xC_*/
	[0xC0] = ret_cc,
	[0xC1] = pop_r16,
	[0xC2] = jp_cc_n16,
	[0xC3] = jp_n16,
	[0xC4] = call_cc_n16,
	[0xC5] = push_r16,
	[0xC7] = rst,
	[0xC8] = ret_cc,
	[0xC9] = ret,
	[0xCA] = jp_cc_n16,
	[0xCC] = call_cc_n16,
	[0xCD] = call_n16,
	[0xCF] = rst,

	/*0xD_*/
	[0xD0] = ret_cc,
	[0xD1] = pop_r16,
	[0xD2] = jp_cc_n16,
	[0xD3] = ill_op,
	[0xD4] = call_cc_n16,
	[0xD5] = push_r16,
	[0xD7] = rst,
	[0xD8] = ret_cc,
	[0xD9] = reti,
	[0xDA] = jp_cc_n16,
	[0xDB] = ill_op,
	[0xDC] = call_cc_n16,
	[0xDD] = ill_op,
	[0xDF] = rst,

	/*0xE_*/
	[0xE0] = ldh_n8_a,
	[0xE1] = pop_r16,
	[0xE2] = ldh_c_a,
	[0xE3 ... 0xE4] = ill_op,
	[0xE5] = push_r16,
	[0xE7] = rst,
	[0xE9] = jp_hl,
	[0xEA] = ld_n16_a,
	[0xEB ... 0xED] = ill_op,
	[0xEF] = rst,

	/*0xF_*/
	[0xF0] = ldh_a_n8,
	[0xF1] = pop_r16,
	[0xF2] = ldh_a_c,
	[0xF3] = di,
	[0xF4] = ill_op,
	[0xF5] = push_r16,
	[0xF7] = rst,
	[0xF8] = ld_hl_sp_e8,
	[0xF9] = ld_sp_hl,
	[0xFA] = ld_a_n16,
	[0xFB] = ei,
	[0xFC ... 0xFD] = ill_op,
	[0xFF] = rst,
};