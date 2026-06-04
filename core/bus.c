#include "bus.h"
#include "cpu.h"
#include "device.h"

/**
 * High RAM, ranges from 0xFF80 to 0xFFFE.
 */
static uint8_t high_ram[128];

static uint8_t high_ram_read(uint16_t addr)
{
	if (addr >= 0xFF80 && addr <= 0xFFFE) {
		return high_ram[addr - 0xFF80];
	} else {
		return 0xFF;
	}
}

static void high_ram_write(uint16_t addr, uint8_t value)
{
	if (addr < 0xFF80 || addr > 0xFFFE) {
		return;
	}
	high_ram[addr - 0xFF80] = value;
}

static uint8_t io_reg_read(uint16_t addr)
{
	return 0xFF;
}

static void io_reg_write(uint16_t addr, uint8_t value)
{
}

uint8_t bus_read8(uint16_t addr)
{
	return 0xFF;
}

void bus_write8(uint16_t addr, uint8_t val)
{
}

uint16_t bus_read16(uint16_t addr)
{
	return 0xFFFF;
}

void bus_write16(uint16_t addr, uint16_t val)
{
}