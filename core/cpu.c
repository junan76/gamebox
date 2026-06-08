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
	uint8_t ime_pending;
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
	 * Interrult requested flag, reg address: 0xFF0F
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

static uint16_t *cpu_reg16(uint8_t encode, uint8_t sp_first)
{
	switch (encode) {
	case 0:
		return &cpu->regs.bc;
	case 1:
		return &cpu->regs.de;
	case 2:
		return &cpu->regs.hl;
	case 3:
		return sp_first ? &cpu->regs.sp : &cpu->regs.af;
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
	cpu->regs.pc += 2;

	uint16_t *r16 = cpu_reg16((opcode >> 4) & 0x03, 1);
	*r16 = value;

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
	uint16_t *r16 = cpu_reg16((opcode >> 4) & 0x03, 0);
	bus_write16(cpu->regs.sp, *r16);
	return 16;
}

static uint8_t pop_r16(uint8_t opcode)
{
	uint16_t *r16 = cpu_reg16((opcode >> 4) & 0x03, 0);
	*r16 = bus_read16(cpu->regs.sp);
	cpu->regs.sp += 2;

	cpu->regs._padding = 0;
	return 12;
}

static uint8_t ld_hl_sp_e8(uint8_t opcode)
{
	int8_t e8 = bus_read8(cpu->regs.pc++);
	uint16_t result = cpu->regs.sp + e8;
	cpu->regs.hl = result;

	cpu->regs.z_flag = 0;
	cpu->regs.n_flag = 0;
	uint16_t carry = (cpu->regs.sp & e8) | ((cpu->regs.sp ^ e8) & ~result);
	cpu->regs.h_flag = !!(carry & 0x08);
	cpu->regs.c_flag = !!(carry & 0x80);
	return 12;
}

/**
 * 8-bit arithmetic and logical instructions.
 */
static uint8_t add_carry(uint8_t a, uint8_t b)
{
	uint8_t s = a + b;
	return (a & b) | ((a ^ b) & ~s);
}

static uint8_t add_carry3(uint8_t a, uint8_t b, uint8_t c)
{
	uint8_t c1 = add_carry(a, b);
	uint8_t c2 = add_carry(a + b, c);
	return c1 | c2;
}

static void add_sync_flags(uint8_t carry)
{
	cpu->regs.z_flag = (cpu->regs.a == 0);
	cpu->regs.n_flag = 0;
	cpu->regs.h_flag = !!(carry & 0x08);
	cpu->regs.c_flag = !!(carry & 0x80);
}

static uint8_t add_a_r8(uint8_t opcode)
{
	uint8_t *rs = cpu_reg8(opcode & 0x07);
	uint8_t carry = add_carry(cpu->regs.a, *rs);

	cpu->regs.a += *rs;
	add_sync_flags(carry);

	return 4;
}

static uint8_t add_a_hl(uint8_t opcode)
{
	uint8_t value = bus_read8(cpu->regs.hl);
	uint8_t carry = add_carry(cpu->regs.a, value);

	cpu->regs.a += value;
	add_sync_flags(carry);

	return 8;
}

static uint8_t add_a_n8(uint8_t opcode)
{
	uint8_t n8 = bus_read8(cpu->regs.pc++);
	uint8_t carry = add_carry(cpu->regs.a, n8);

	cpu->regs.a += n8;
	add_sync_flags(carry);

	return 8;
}

static uint8_t adc_a_r8(uint8_t opcode)
{
	uint8_t *rs = cpu_reg8(opcode & 0x07);
	uint8_t carry = add_carry3(cpu->regs.a, *rs, cpu->regs.c_flag);

	cpu->regs.a = cpu->regs.a + *rs + cpu->regs.c_flag;
	add_sync_flags(carry);

	return 4;
}

static uint8_t adc_a_hl(uint8_t opcode)
{
	uint8_t value = bus_read8(cpu->regs.hl);
	uint8_t carry = add_carry3(cpu->regs.a, value, cpu->regs.c_flag);

	cpu->regs.a = cpu->regs.a + value + cpu->regs.c_flag;
	add_sync_flags(carry);

	return 8;
}

static uint8_t adc_a_n8(uint8_t opcode)
{
	uint8_t n8 = bus_read8(cpu->regs.pc++);
	uint8_t carry = add_carry3(cpu->regs.a, n8, cpu->regs.c_flag);

	cpu->regs.a = cpu->regs.a + n8 + cpu->regs.c_flag;
	add_sync_flags(carry);

	return 8;
}

static uint8_t sub_carry(uint8_t a, uint8_t b)
{
	uint8_t result = 0;
	if (a < b) {
		result |= 0x80;
	}
	if ((a & 0x0F) < (b & 0x0F)) {
		result |= 0x08;
	}
	return result;
}

static uint8_t sub_carry3(uint8_t a, uint8_t b, uint8_t c)
{
	uint8_t c1 = sub_carry(a, b);
	uint8_t c2 = sub_carry(a - b, c);
	return c1 | c2;
}

static void sub_sync_flags(uint8_t carry)
{
	cpu->regs.z_flag = (cpu->regs.a == 0);
	cpu->regs.n_flag = 1;
	cpu->regs.h_flag = !!(carry & 0x08);
	cpu->regs.c_flag = !!(carry & 0x80);
}

static uint8_t sub_a_r8(uint8_t opcode)
{
	uint8_t *r8 = cpu_reg8(opcode & 0x7);
	uint8_t carry = sub_carry(cpu->regs.a, *r8);

	cpu->regs.a -= *r8;
	sub_sync_flags(carry);
	return 4;
}

static uint8_t sub_a_hl(uint8_t opcode)
{
	uint8_t value = bus_read8(cpu->regs.hl);
	uint8_t carry = sub_carry(cpu->regs.a, value);

	cpu->regs.a -= value;
	sub_sync_flags(carry);
	return 8;
}

static uint8_t sub_a_n8(uint8_t opcode)
{
	uint8_t n8 = bus_read8(cpu->regs.pc++);
	uint8_t carry = sub_carry(cpu->regs.a, n8);

	cpu->regs.a -= n8;
	sub_sync_flags(carry);
	return 8;
}

static uint8_t sbc_a_r8(uint8_t opcode)
{
	uint8_t *r8 = cpu_reg8(opcode & 0x07);
	uint8_t carry = sub_carry3(cpu->regs.a, *r8, cpu->regs.c_flag);

	cpu->regs.a = cpu->regs.a - *r8 - cpu->regs.c_flag;
	sub_sync_flags(carry);
	return 4;
}

static uint8_t sbc_a_hl(uint8_t opcode)
{
	uint8_t value = bus_read8(cpu->regs.hl);
	uint8_t carry = sub_carry3(cpu->regs.a, value, cpu->regs.c_flag);

	cpu->regs.a = cpu->regs.a - value - cpu->regs.c_flag;
	sub_sync_flags(carry);
	return 8;
}

static uint8_t sbc_a_n8(uint8_t opcode)
{
	uint8_t n8 = bus_read8(cpu->regs.pc++);
	uint8_t carry = sub_carry3(cpu->regs.a, n8, cpu->regs.c_flag);

	cpu->regs.a = cpu->regs.a - n8 - cpu->regs.c_flag;
	sub_sync_flags(carry);
	return 8;
}

static void cp_sync_flags(uint8_t result, uint8_t carry)
{
	cpu->regs.z_flag = (result == 0);
	cpu->regs.n_flag = 1;
	cpu->regs.h_flag = !!(carry & 0x08);
	cpu->regs.c_flag = !!(carry & 0x80);
}

static uint8_t cp_a_r8(uint8_t opcode)
{
	uint8_t *r8 = cpu_reg8(opcode & 0x07);
	uint8_t carry = sub_carry(cpu->regs.a, *r8);
	cp_sync_flags(cpu->regs.a - *r8, carry);
	return 4;
}

static uint8_t cp_a_hl(uint8_t opcode)
{
	uint8_t value = bus_read8(cpu->regs.hl);
	uint8_t carry = sub_carry(cpu->regs.a, value);
	cp_sync_flags(cpu->regs.a - value, carry);
	return 8;
}

static uint8_t cp_a_n8(uint8_t opcode)
{
	uint8_t n8 = bus_read8(cpu->regs.pc++);
	uint8_t carry = sub_carry(cpu->regs.a, n8);
	cp_sync_flags(cpu->regs.a - n8, carry);
	return 8;
}

static void inc_sync_flags(uint8_t result, uint8_t carry)
{
	cpu->regs.z_flag = (result == 0);
	cpu->regs.n_flag = 0;
	cpu->regs.h_flag = !!(carry & 0x08);
}

static uint8_t inc_r8(uint8_t opcode)
{
	uint8_t *rd = cpu_reg8((opcode >> 3) & 0x07);
	uint8_t carry = add_carry(*rd, 1);
	*rd += 1;
	inc_sync_flags(*rd, carry);
	return 4;
}

static uint8_t inc_at_hl(uint8_t opcode)
{
	uint8_t value = bus_read8(cpu->regs.hl);
	uint8_t carry = add_carry(value, 1);
	value += 1;
	bus_write8(cpu->regs.hl, value);
	inc_sync_flags(value, carry);
	return 12;
}

static void dec_sync_flags(uint8_t result, uint8_t carry)
{
	cpu->regs.z_flag = (result == 0);
	cpu->regs.n_flag = 1;
	cpu->regs.h_flag = !!(carry & 0x08);
}

static uint8_t dec_r8(uint8_t opcode)
{
	uint8_t *rd = cpu_reg8((opcode >> 3) & 0x07);
	uint8_t carry = sub_carry(*rd, 1);
	*rd -= 1;
	dec_sync_flags(*rd, carry);
	return 4;
}

static uint8_t dec_at_hl(uint8_t opcode)
{
	uint8_t value = bus_read8(cpu->regs.hl);
	uint8_t carry = sub_carry(value, 1);
	value -= 1;
	bus_write8(cpu->regs.hl, value);
	dec_sync_flags(value, carry);
	return 12;
}

static void and_sync_flags(void)
{
	cpu->regs.z_flag = (cpu->regs.a == 0);
	cpu->regs.n_flag = 0;
	cpu->regs.h_flag = 1;
	cpu->regs.c_flag = 0;
}

static uint8_t and_a_r8(uint8_t opcode)
{
	uint8_t *rs = cpu_reg8(opcode & 0x07);
	cpu->regs.a &= *rs;
	and_sync_flags();
	return 4;
}

static uint8_t and_a_hl(uint8_t opcode)
{
	uint8_t value = bus_read8(cpu->regs.hl);
	cpu->regs.a &= value;
	and_sync_flags();
	return 8;
}

static uint8_t and_a_n8(uint8_t opcode)
{
	uint8_t n8 = bus_read8(cpu->regs.pc++);
	cpu->regs.a &= n8;
	and_sync_flags();
	return 8;
}

static void or_xor_sync_flags(void)
{
	cpu->regs.z_flag = (cpu->regs.a == 0);
	cpu->regs.n_flag = 0;
	cpu->regs.h_flag = 0;
	cpu->regs.c_flag = 0;
}

static uint8_t or_a_r8(uint8_t opcode)
{
	uint8_t *rs = cpu_reg8(opcode & 0x07);
	cpu->regs.a |= *rs;
	or_xor_sync_flags();
	return 4;
}

static uint8_t or_a_hl(uint8_t opcode)
{
	uint8_t value = bus_read8(cpu->regs.hl);
	cpu->regs.a |= value;
	or_xor_sync_flags();
	return 8;
}

static uint8_t or_a_n8(uint8_t opcode)
{
	uint8_t n8 = bus_read8(cpu->regs.pc++);
	cpu->regs.a |= n8;
	or_xor_sync_flags();
	return 8;
}

static uint8_t xor_a_r8(uint8_t opcode)
{
	uint8_t *rs = cpu_reg8(opcode & 0x07);
	cpu->regs.a ^= *rs;
	or_xor_sync_flags();
	return 4;
}

static uint8_t xor_a_hl(uint8_t opcode)
{
	uint8_t value = bus_read8(cpu->regs.hl);
	cpu->regs.a ^= value;
	or_xor_sync_flags();
	return 8;
}

static uint8_t xor_a_n8(uint8_t opcode)
{
	uint8_t n8 = bus_read8(cpu->regs.pc++);
	cpu->regs.a ^= n8;
	or_xor_sync_flags();
	return 8;
}

static uint8_t ccf(uint8_t opcode)
{
	cpu->regs.n_flag = 0;
	cpu->regs.h_flag = 0;
	cpu->regs.c_flag = 1 - cpu->regs.c_flag;
	return 4;
}

static uint8_t scf(uint8_t opcode)
{
	cpu->regs.n_flag = 0;
	cpu->regs.h_flag = 0;
	cpu->regs.c_flag = 1;
	return 4;
}

static uint8_t daa(uint8_t opcode)
{
	uint8_t adjust = 0;

	if (cpu->regs.n_flag == 0) {
		if (cpu->regs.h_flag || (cpu->regs.a & 0x0F) > 0x09) {
			adjust += 0x06;
		}
		if (cpu->regs.c_flag || cpu->regs.a > 0x99) {
			adjust += 0x60;
			cpu->regs.c_flag = 1;
		}
		cpu->regs.a += adjust;
	} else {
		if (cpu->regs.h_flag) {
			adjust += 0x06;
		}
		if (cpu->regs.c_flag) {
			adjust += 0x60;
		}
		cpu->regs.a -= adjust;
	}

	cpu->regs.z_flag = (cpu->regs.a == 0);
	cpu->regs.h_flag = 0;
	return 4;
}

static uint8_t cpl(uint8_t opcode)
{
	cpu->regs.a = ~cpu->regs.a;
	cpu->regs.n_flag = 1;
	cpu->regs.h_flag = 1;
	return 4;
}

/**
 * 16-bit arithmetic instructions.
 */
static uint8_t inc_r16(uint8_t opcode)
{
	uint16_t *r16 = cpu_reg16((opcode >> 4) & 0x03, 1);
	*r16 += 1;
	return 8;
}

static uint8_t dec_r16(uint8_t opcode)
{
	uint16_t *r16 = cpu_reg16((opcode >> 4) & 0x03, 1);
	*r16 -= 1;
	return 8;
}

static uint8_t add_hl_r16(uint8_t opcode)
{
	uint16_t *r16 = cpu_reg16((opcode >> 4) & 0x03, 1);
	uint16_t result = cpu->regs.hl + *r16;
	uint16_t carry = (cpu->regs.hl & *r16) |
			 ((cpu->regs.hl ^ *r16) & ~result);

	cpu->regs.hl = result;
	cpu->regs.n_flag = 0;
	cpu->regs.h_flag = !!(carry & 0x0800);
	cpu->regs.c_flag = !!(carry & 0x8000);
	return 8;
}

static uint8_t add_sp_e8(uint8_t opcode)
{
	int8_t e8 = bus_read8(cpu->regs.pc++);
	uint16_t result = cpu->regs.sp + e8;
	uint16_t carry = (cpu->regs.sp & e8) | ((cpu->regs.sp ^ e8) & ~result);

	cpu->regs.sp = result;
	cpu->regs.z_flag = 0;
	cpu->regs.n_flag = 0;
	cpu->regs.h_flag = !!(carry & 0x08);
	cpu->regs.c_flag = !!(carry & 0x80);
	return 16;
}

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
	switch (opcode) {
	case 0xC0:
		if (!cpu->regs.z_flag) {
			cpu->regs.pc = bus_read16(cpu->regs.sp);
			cpu->regs.sp += 2;
			return 20;
		}
		break;
	case 0xC8:
		if (cpu->regs.z_flag) {
			cpu->regs.pc = bus_read16(cpu->regs.sp);
			cpu->regs.sp += 2;
			return 20;
		}
		break;
	case 0xD0:
		if (!cpu->regs.c_flag) {
			cpu->regs.pc = bus_read16(cpu->regs.sp);
			cpu->regs.sp += 2;
			return 20;
		}
		break;
	case 0xD8:
		if (cpu->regs.c_flag) {
			cpu->regs.pc = bus_read16(cpu->regs.sp);
			cpu->regs.sp += 2;
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
	if (cpu->ime == 0 && cpu->ime_pending == 0 &&
	    (cpu->ie & cpu->irq & 0x1F)) {
		cpu->halt_bug = 1;
	} else {
		cpu->halted = 1;
	}
	return 4;
}

static uint8_t stop(uint8_t opcode)
{
	bus_read8(cpu->regs.pc++);
	cpu->stopped = 1;
	return 4;
}

static uint8_t di(uint8_t opcode)
{
	cpu->ime = 0;
	cpu->ime_pending = 0;
	return 4;
}

static uint8_t ei(uint8_t opcode)
{
	cpu->ime_pending = 1;
	return 4;
}

static uint8_t nop(uint8_t opcode)
{
	return 4;
}

/**
 * Accumulator bit rotation instructions.
 */
static uint8_t rlca(uint8_t opcode)
{
	uint8_t bit7 = !!(cpu->regs.a & 0x80);
	cpu->regs.a <<= 1;
	cpu->regs.a |= bit7;

	cpu->regs.z_flag = 0;
	cpu->regs.n_flag = 0;
	cpu->regs.h_flag = 0;
	cpu->regs.c_flag = bit7;
	return 4;
}

static uint8_t rrca(uint8_t opcode)
{
	uint8_t bit0 = !!(cpu->regs.a & 0x01);
	cpu->regs.a >>= 1;
	cpu->regs.a |= (bit0 << 7);

	cpu->regs.z_flag = 0;
	cpu->regs.n_flag = 0;
	cpu->regs.h_flag = 0;
	cpu->regs.c_flag = bit0;
	return 4;
}

static uint8_t rla(uint8_t opcode)
{
	uint8_t bit7 = !!(cpu->regs.a & 0x80);
	cpu->regs.a <<= 1;
	cpu->regs.a |= cpu->regs.c_flag;

	cpu->regs.z_flag = 0;
	cpu->regs.n_flag = 0;
	cpu->regs.h_flag = 0;
	cpu->regs.c_flag = bit7;
	return 4;
}

static uint8_t rra(uint8_t opcode)
{
	uint8_t bit0 = !!(cpu->regs.a & 0x01);
	cpu->regs.a >>= 1;
	cpu->regs.a |= (cpu->regs.c_flag << 7);

	cpu->regs.z_flag = 0;
	cpu->regs.n_flag = 0;
	cpu->regs.h_flag = 0;
	cpu->regs.c_flag = bit0;
	return 4;
}

static uint8_t cb_prefix(uint8_t opcode);
typedef uint8_t (*opcode_handler)(uint8_t);

static const opcode_handler opcode_handlers[256] = {
	/*0x0_*/
	[0x00] = nop,
	[0x01] = ld_r16_n16,
	[0x02] = ld_r16_a,
	[0x03] = inc_r16,
	[0x04] = inc_r8,
	[0x05] = dec_r8,
	[0x06] = ld_r8_n8,
	[0x07] = rlca,
	[0x08] = ld_n16_sp,
	[0x09] = add_hl_r16,
	[0x0A] = ld_a_r16,
	[0x0B] = dec_r16,
	[0x0C] = inc_r8,
	[0x0D] = dec_r8,
	[0x0E] = ld_r8_n8,
	[0x0F] = rrca,

	/*0x1_*/
	[0x10] = stop,
	[0x11] = ld_r16_n16,
	[0x12] = ld_r16_a,
	[0x13] = inc_r16,
	[0x14] = inc_r8,
	[0x15] = dec_r8,
	[0x16] = ld_r8_n8,
	[0x17] = rla,
	[0x18] = jr_e8,
	[0x19] = add_hl_r16,
	[0x1A] = ld_a_r16,
	[0x1B] = dec_r16,
	[0x1C] = inc_r8,
	[0x1D] = dec_r8,
	[0x1E] = ld_r8_n8,
	[0x1F] = rra,

	/*0x2_*/
	[0x20] = jr_cc_e8,
	[0x21] = ld_r16_n16,
	[0x22] = ldi_hl_a,
	[0x23] = inc_r16,
	[0x24] = inc_r8,
	[0x25] = dec_r8,
	[0x26] = ld_r8_n8,
	[0x27] = daa,
	[0x28] = jr_cc_e8,
	[0x29] = add_hl_r16,
	[0x2A] = ldi_a_hl,
	[0x2B] = dec_r16,
	[0x2C] = inc_r8,
	[0x2D] = dec_r8,
	[0x2E] = ld_r8_n8,
	[0x2F] = cpl,

	/*0x3_*/
	[0x30] = jr_cc_e8,
	[0x31] = ld_r16_n16,
	[0x32] = ldd_hl_a,
	[0x33] = inc_r16,
	[0x34] = inc_at_hl,
	[0x35] = dec_at_hl,
	[0x36] = ld_hl_n8,
	[0x37] = scf,
	[0x38] = jr_cc_e8,
	[0x39] = add_hl_r16,
	[0x3A] = ldd_a_hl,
	[0x3B] = dec_r16,
	[0x3C] = inc_r8,
	[0x3D] = dec_r8,
	[0x3E] = ld_r8_n8,
	[0x3F] = ccf,

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
	[0x80 ... 0x85] = add_a_r8,
	[0x86] = add_a_hl,
	[0x87] = add_a_r8,
	[0x88 ... 0x8D] = adc_a_r8,
	[0x8E] = adc_a_hl,
	[0x8F] = adc_a_r8,

	/*0x9_*/
	[0x90 ... 0x95] = sub_a_r8,
	[0x96] = sub_a_hl,
	[0x97] = sub_a_r8,
	[0x98 ... 0x9D] = sbc_a_r8,
	[0x9E] = sbc_a_hl,
	[0x9F] = sbc_a_r8,

	/*0xA_*/
	[0xA0 ... 0xA5] = and_a_r8,
	[0xA6] = and_a_hl,
	[0xA7] = and_a_r8,
	[0xA8 ... 0xAD] = xor_a_r8,
	[0xAE] = xor_a_hl,
	[0xAF] = xor_a_r8,

	/*0xB_*/
	[0xB0 ... 0xB5] = or_a_r8,
	[0xB6] = or_a_hl,
	[0xB7] = or_a_r8,
	[0xB8 ... 0xBD] = cp_a_r8,
	[0xBE] = cp_a_hl,
	[0xBF] = cp_a_r8,

	/*0xC_*/
	[0xC0] = ret_cc,
	[0xC1] = pop_r16,
	[0xC2] = jp_cc_n16,
	[0xC3] = jp_n16,
	[0xC4] = call_cc_n16,
	[0xC5] = push_r16,
	[0xC6] = add_a_n8,
	[0xC7] = rst,
	[0xC8] = ret_cc,
	[0xC9] = ret,
	[0xCA] = jp_cc_n16,
	[0xCB] = cb_prefix,
	[0xCC] = call_cc_n16,
	[0xCD] = call_n16,
	[0xCE] = adc_a_n8,
	[0xCF] = rst,

	/*0xD_*/
	[0xD0] = ret_cc,
	[0xD1] = pop_r16,
	[0xD2] = jp_cc_n16,
	[0xD3] = ill_op,
	[0xD4] = call_cc_n16,
	[0xD5] = push_r16,
	[0xD6] = sub_a_n8,
	[0xD7] = rst,
	[0xD8] = ret_cc,
	[0xD9] = reti,
	[0xDA] = jp_cc_n16,
	[0xDB] = ill_op,
	[0xDC] = call_cc_n16,
	[0xDD] = ill_op,
	[0xDE] = sbc_a_n8,
	[0xDF] = rst,

	/*0xE_*/
	[0xE0] = ldh_n8_a,
	[0xE1] = pop_r16,
	[0xE2] = ldh_c_a,
	[0xE3 ... 0xE4] = ill_op,
	[0xE5] = push_r16,
	[0xE6] = and_a_n8,
	[0xE7] = rst,
	[0xE8] = add_sp_e8,
	[0xE9] = jp_hl,
	[0xEA] = ld_n16_a,
	[0xEB ... 0xED] = ill_op,
	[0xEE] = xor_a_n8,
	[0xEF] = rst,

	/*0xF_*/
	[0xF0] = ldh_a_n8,
	[0xF1] = pop_r16,
	[0xF2] = ldh_a_c,
	[0xF3] = di,
	[0xF4] = ill_op,
	[0xF5] = push_r16,
	[0xF6] = or_a_n8,
	[0xF7] = rst,
	[0xF8] = ld_hl_sp_e8,
	[0xF9] = ld_sp_hl,
	[0xFA] = ld_a_n16,
	[0xFB] = ei,
	[0xFC ... 0xFD] = ill_op,
	[0xFE] = cp_a_n8,
	[0xFF] = rst,
};

/**
 * Rotate, shift and bit operation instructions.
 */
static uint8_t rlc_r8(uint8_t opcode)
{
	uint8_t *r8 = cpu_reg8(opcode & 0x07);
	uint8_t bit7 = !!(*r8 & 0x80);
	*r8 <<= 1;
	*r8 |= bit7;

	cpu->regs.z_flag = (*r8 == 0);
	cpu->regs.n_flag = 0;
	cpu->regs.h_flag = 0;
	cpu->regs.c_flag = bit7;

	return 8;
}

static uint8_t rlc_hl(uint8_t opcode)
{
	uint8_t value = bus_read8(cpu->regs.hl);
	uint8_t bit7 = !!(value & 0x80);
	value <<= 1;
	value |= bit7;
	bus_write8(cpu->regs.hl, value);

	cpu->regs.z_flag = (value == 0);
	cpu->regs.n_flag = 0;
	cpu->regs.h_flag = 0;
	cpu->regs.c_flag = bit7;

	return 16;
}

static uint8_t rrc_r8(uint8_t opcode)
{
	uint8_t *r8 = cpu_reg8(opcode & 0x07);
	uint8_t bit0 = !!(*r8 & 0x01);
	*r8 >>= 1;
	*r8 |= (bit0 << 7);

	cpu->regs.z_flag = (*r8 == 0);
	cpu->regs.n_flag = 0;
	cpu->regs.h_flag = 0;
	cpu->regs.c_flag = bit0;

	return 8;
}

static uint8_t rrc_hl(uint8_t opcode)
{
	uint8_t value = bus_read8(cpu->regs.hl);
	uint8_t bit0 = !!(value & 0x01);
	value >>= 1;
	value |= (bit0 << 7);
	bus_write8(cpu->regs.hl, value);

	cpu->regs.z_flag = (value == 0);
	cpu->regs.n_flag = 0;
	cpu->regs.h_flag = 0;
	cpu->regs.c_flag = bit0;

	return 16;
}

static uint8_t rl_r8(uint8_t opcode)
{
	uint8_t *r8 = cpu_reg8(opcode & 0x07);
	uint8_t bit7 = !!(*r8 & 0x80);
	*r8 <<= 1;
	*r8 |= cpu->regs.c_flag;

	cpu->regs.z_flag = (*r8 == 0);
	cpu->regs.n_flag = 0;
	cpu->regs.h_flag = 0;
	cpu->regs.c_flag = bit7;

	return 8;
}

static uint8_t rl_hl(uint8_t opcode)
{
	uint8_t value = bus_read8(cpu->regs.hl);
	uint8_t bit7 = !!(value & 0x80);
	value <<= 1;
	value |= cpu->regs.c_flag;
	bus_write8(cpu->regs.hl, value);

	cpu->regs.z_flag = (value == 0);
	cpu->regs.n_flag = 0;
	cpu->regs.h_flag = 0;
	cpu->regs.c_flag = bit7;

	return 16;
}

static uint8_t rr_r8(uint8_t opcode)
{
	uint8_t *r8 = cpu_reg8(opcode & 0x07);
	uint8_t bit0 = !!(*r8 & 0x01);
	*r8 >>= 1;
	*r8 |= (cpu->regs.c_flag << 7);

	cpu->regs.z_flag = (*r8 == 0);
	cpu->regs.n_flag = 0;
	cpu->regs.h_flag = 0;
	cpu->regs.c_flag = bit0;

	return 8;
}

static uint8_t rr_hl(uint8_t opcode)
{
	uint8_t value = bus_read8(cpu->regs.hl);
	uint8_t bit0 = !!(value & 0x01);
	value >>= 1;
	value |= (cpu->regs.c_flag << 7);
	bus_write8(cpu->regs.hl, value);

	cpu->regs.z_flag = (value == 0);
	cpu->regs.n_flag = 0;
	cpu->regs.h_flag = 0;
	cpu->regs.c_flag = bit0;

	return 16;
}

static uint8_t sla_r8(uint8_t opcode)
{
	uint8_t *r8 = cpu_reg8(opcode & 0x07);
	uint8_t bit7 = !!(*r8 & 0x80);
	*r8 <<= 1;

	cpu->regs.z_flag = (*r8 == 0);
	cpu->regs.n_flag = 0;
	cpu->regs.h_flag = 0;
	cpu->regs.c_flag = bit7;

	return 8;
}

static uint8_t sla_hl(uint8_t opcode)
{
	uint8_t value = bus_read8(cpu->regs.hl);
	uint8_t bit7 = !!(value & 0x80);
	value <<= 1;
	bus_write8(cpu->regs.hl, value);

	cpu->regs.z_flag = (value == 0);
	cpu->regs.n_flag = 0;
	cpu->regs.h_flag = 0;
	cpu->regs.c_flag = bit7;

	return 16;
}

static uint8_t sra_r8(uint8_t opcode)
{
	uint8_t *r8 = cpu_reg8(opcode & 0x07);
	uint8_t bit0 = !!(*r8 & 0x01);
	uint8_t bit7 = !!(*r8 & 0x80);
	*r8 >>= 1;
	*r8 |= (bit7 << 7);

	cpu->regs.z_flag = (*r8 == 0);
	cpu->regs.n_flag = 0;
	cpu->regs.h_flag = 0;
	cpu->regs.c_flag = bit0;

	return 8;
}

static uint8_t sra_hl(uint8_t opcode)
{
	uint8_t value = bus_read8(cpu->regs.hl);
	uint8_t bit0 = !!(value & 0x01);
	uint8_t bit7 = !!(value & 0x80);
	value >>= 1;
	value |= (bit7 << 7);
	bus_write8(cpu->regs.hl, value);

	cpu->regs.z_flag = (value == 0);
	cpu->regs.n_flag = 0;
	cpu->regs.h_flag = 0;
	cpu->regs.c_flag = bit0;

	return 16;
}

static uint8_t swap_r8(uint8_t opcode)
{
	uint8_t *r8 = cpu_reg8(opcode & 0x07);
	*r8 = (*r8 << 4) | (*r8 >> 4);

	cpu->regs.z_flag = (*r8 == 0);
	cpu->regs.n_flag = 0;
	cpu->regs.h_flag = 0;
	cpu->regs.c_flag = 0;

	return 8;
}

static uint8_t swap_hl(uint8_t opcode)
{
	uint8_t value = bus_read8(cpu->regs.hl);
	value = (value << 4) | (value >> 4);
	bus_write8(cpu->regs.hl, value);

	cpu->regs.z_flag = (value == 0);
	cpu->regs.n_flag = 0;
	cpu->regs.h_flag = 0;
	cpu->regs.c_flag = 0;

	return 16;
}

static uint8_t srl_r8(uint8_t opcode)
{
	uint8_t *r8 = cpu_reg8(opcode & 0x07);
	uint8_t bit0 = !!(*r8 & 0x01);
	*r8 >>= 1;

	cpu->regs.z_flag = (*r8 == 0);
	cpu->regs.n_flag = 0;
	cpu->regs.h_flag = 0;
	cpu->regs.c_flag = bit0;

	return 8;
}

static uint8_t srl_hl(uint8_t opcode)
{
	uint8_t value = bus_read8(cpu->regs.hl);
	uint8_t bit0 = !!(value & 0x01);
	value >>= 1;
	bus_write8(cpu->regs.hl, value);

	cpu->regs.z_flag = (value == 0);
	cpu->regs.n_flag = 0;
	cpu->regs.h_flag = 0;
	cpu->regs.c_flag = bit0;

	return 16;
}

static uint8_t bit_r8(uint8_t opcode)
{
	uint8_t *r8 = cpu_reg8(opcode & 0x07);
	uint8_t bit = (opcode >> 3) & 0x07;
	bit = !!(*r8 & (1 << bit));

	cpu->regs.z_flag = (bit == 0);
	cpu->regs.n_flag = 0;
	cpu->regs.h_flag = 1;

	return 8;
}

static uint8_t bit_hl(uint8_t opcode)
{
	uint8_t value = bus_read8(cpu->regs.hl);
	uint8_t bit = (opcode >> 3) & 0x07;
	bit = !!(value & (1 << bit));

	cpu->regs.z_flag = (bit == 0);
	cpu->regs.n_flag = 0;
	cpu->regs.h_flag = 1;

	return 12;
}

static uint8_t res_r8(uint8_t opcode)
{
	uint8_t *r8 = cpu_reg8(opcode & 0x07);
	uint8_t bit = (opcode >> 3) & 0x07;
	*r8 &= ~(1 << bit);

	return 8;
}

static uint8_t res_hl(uint8_t opcode)
{
	uint8_t value = bus_read8(cpu->regs.hl);
	uint8_t bit = (opcode >> 3) & 0x07;
	value &= ~(1 << bit);
	bus_write8(cpu->regs.hl, value);

	return 16;
}

static uint8_t set_r8(uint8_t opcode)
{
	uint8_t *r8 = cpu_reg8(opcode & 0x07);
	uint8_t bit = (opcode >> 3) & 0x07;
	*r8 |= (1 << bit);

	return 8;
}

static uint8_t set_hl(uint8_t opcode)
{
	uint8_t value = bus_read8(cpu->regs.hl);
	uint8_t bit = (opcode >> 3) & 0x07;
	value |= (1 << bit);
	bus_write8(cpu->regs.hl, value);

	return 16;
}

static const opcode_handler cb_opcode_handlers[256] = {
	/*0x0_*/
	[0x00 ... 0x05] = rlc_r8,
	[0x06] = rlc_hl,
	[0x07] = rlc_r8,
	[0x08 ... 0x0D] = rrc_r8,
	[0x0E] = rrc_hl,
	[0x0F] = rrc_r8,
	/*0x1_*/
	[0x10 ... 0x15] = rl_r8,
	[0x16] = rl_hl,
	[0x17] = rl_r8,
	[0x18 ... 0x1D] = rr_r8,
	[0x1E] = rr_hl,
	[0x1F] = rr_r8,
	/*0x2_*/
	[0x20 ... 0x25] = sla_r8,
	[0x26] = sla_hl,
	[0x27] = sla_r8,
	[0x28 ... 0x2D] = sra_r8,
	[0x2E] = sra_hl,
	[0x2F] = sra_r8,
	/*0x3_*/
	[0x30 ... 0x35] = swap_r8,
	[0x36] = swap_hl,
	[0x37] = swap_r8,
	[0x38 ... 0x3D] = srl_r8,
	[0x3E] = srl_hl,
	[0x3F] = srl_r8,
	/*0x4_*/
	[0x40 ... 0x45] = bit_r8,
	[0x46] = bit_hl,
	[0x47 ... 0x4D] = bit_r8,
	[0x4E] = bit_hl,
	[0x4F] = bit_r8,
	/*0x5_*/
	[0x50 ... 0x55] = bit_r8,
	[0x56] = bit_hl,
	[0x57 ... 0x5D] = bit_r8,
	[0x5E] = bit_hl,
	[0x5F] = bit_r8,
	/*0x6_*/
	[0x60 ... 0x65] = bit_r8,
	[0x66] = bit_hl,
	[0x67 ... 0x6D] = bit_r8,
	[0x6E] = bit_hl,
	[0x6F] = bit_r8,
	/*0x7_*/
	[0x70 ... 0x75] = bit_r8,
	[0x76] = bit_hl,
	[0x77 ... 0x7D] = bit_r8,
	[0x7E] = bit_hl,
	[0x7F] = bit_r8,
	/*0x8_*/
	[0x80 ... 0x85] = res_r8,
	[0x86] = res_hl,
	[0x87 ... 0x8D] = res_r8,
	[0x8E] = res_hl,
	[0x8F] = res_r8,
	/*0x9_*/
	[0x90 ... 0x95] = res_r8,
	[0x96] = res_hl,
	[0x97 ... 0x9D] = res_r8,
	[0x9E] = res_hl,
	[0x9F] = res_r8,
	/*0xA_*/
	[0xA0 ... 0xA5] = res_r8,
	[0xA6] = res_hl,
	[0xA7 ... 0xAD] = res_r8,
	[0xAE] = res_hl,
	[0xAF] = res_r8,
	/*0xB_*/
	[0xB0 ... 0xB5] = res_r8,
	[0xB6] = res_hl,
	[0xB7 ... 0xBD] = res_r8,
	[0xBE] = res_hl,
	[0xBF] = res_r8,
	/*0xC_*/
	[0xC0 ... 0xC5] = set_r8,
	[0xC6] = set_hl,
	[0xC7 ... 0xCD] = set_r8,
	[0xCE] = set_hl,
	[0xCF] = set_r8,
	/*0xD_*/
	[0xD0 ... 0xD5] = set_r8,
	[0xD6] = set_hl,
	[0xD7 ... 0xDD] = set_r8,
	[0xDE] = set_hl,
	[0xDF] = set_r8,
	/*0xE_*/
	[0xE0 ... 0xE5] = set_r8,
	[0xE6] = set_hl,
	[0xE7 ... 0xED] = set_r8,
	[0xEE] = set_hl,
	[0xEF] = set_r8,
	/*0xF_*/
	[0xF0 ... 0xF5] = set_r8,
	[0xF6] = set_hl,
	[0xF7 ... 0xFD] = set_r8,
	[0xFE] = set_hl,
	[0xFF] = set_r8,
};

static uint8_t cb_prefix(uint8_t opcode)
{
	opcode = bus_read8(cpu->regs.pc++);
	return cb_opcode_handlers[opcode](opcode);
}

uint8_t cpu_step(void)
{
	if (cpu->stopped) {
		return 0;
	}

	static uint8_t irq_vectors[] = { 0x40, 0x48, 0x50, 0x58, 0x60 };
	uint8_t irq_pending = cpu->ie & cpu->irq & 0x1F;
	uint8_t irq = 0;

	if (irq_pending) {
		if (cpu->halted) {
			cpu->halted = 0;
		}

		if (cpu->ime) {
			while (irq < 5) {
				if (irq_pending & (1 << irq)) {
					cpu->irq &= ~(1 << irq);
					cpu->ime = 0;
					break;
				}
				irq++;
			}

			cpu->regs.sp -= 2;
			bus_write16(cpu->regs.sp, cpu->regs.pc);
			cpu->regs.pc = irq_vectors[irq];

			return 20;
		}
	}

	if (cpu->halted) {
		return 4;
	}

	uint8_t opcode = bus_read8(cpu->regs.pc);
	if (cpu->halt_bug) {
		cpu->halt_bug = 0;
	} else {
		cpu->regs.pc++;
	}

	opcode_handler handler = opcode_handlers[opcode];
	uint8_t result = handler(opcode);

	if (cpu->ime_pending && opcode != 0xFB) {
		cpu->ime_pending = 0;
		cpu->ime = 1;
	}

	return result;
}

void cpu_reset(void)
{
	cpu->regs.a = 0x01;
	cpu->regs.z_flag = 1;
	cpu->regs.n_flag = 0;
	cpu->regs.h_flag = 0;
	cpu->regs.c_flag = 0;

	cpu->regs.b = 0x00;
	cpu->regs.c = 0x13;

	cpu->regs.d = 0x00;
	cpu->regs.e = 0xD8;

	cpu->regs.h = 0x01;
	cpu->regs.l = 0x4D;

	cpu->regs.pc = 0x0100;
	cpu->regs.sp = 0xFFFE;
}

uint8_t cpu_ie_read()
{
	return cpu->ie;
}

void cpu_ie_write(uint8_t value)
{
	cpu->ie = value;
}

uint8_t cpu_irq_read()
{
	return cpu->irq;
}

void cpu_irq_write(uint8_t value)
{
	cpu->irq = (value & 0x1F);
}