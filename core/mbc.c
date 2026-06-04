#include <stdint.h>
#define SZ_16KB 16384
#define SZ_8KB 8192
#define SZ_4KB 4096

/**
 * Two 16KB ROM banks, bank1 is switchable maybe.
 * ROM bank0: [0x0000, 0x3FFFF]
 * ROM bank1: [0x4000, 0x7FFFF]
 */
static uint8_t rom_bank0[SZ_16KB];
static uint8_t rom_bank1[SZ_16KB];
/**
 * One 8KB external RAM from cartridge, switchable maybe.
 * Ranges from 0xA000 to 0xBFFF
 */
static uint8_t ram_external[SZ_8KB];

uint8_t rom_read(uint16_t addr)
{
	if (addr <= 0x3FFF) {
		return rom_bank0[addr];
	} else if (addr <= 0x7FFF) {
		return rom_bank1[addr - 0x4000];
	} else {
		return 0xFF;
	}
}

uint8_t ram_external_read(uint16_t addr)
{
	if (addr >= 0xA000 && addr <= 0xBFFF) {
		return ram_external[addr - 0xA000];
	} else {
		return 0xFF;
	}
}

void ram_external_write(uint16_t addr, uint8_t value)
{
	if (addr < 0xA000 || addr > 0xBFFF) {
		return;
	}
	ram_external[addr - 0xA000] = value;
}