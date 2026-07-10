#include "bus.h"
#include "cpu.h"
#include "device.h"
#include "mbc.h"
#include "ppu.h"

#include <stdio.h>
#include <stdlib.h>

/**
 * High RAM, range from 0xFF80 to 0xFFFE.
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

/**
 * Work RAM, 8KB.
 */
static uint8_t work_ram[8192];

static uint8_t work_ram_read(uint16_t addr)
{
	if (addr < 0xC000 || addr > 0xDFFF)
		return 0xFF;
	return work_ram[addr - 0xC000];
}

static void work_ram_write(uint16_t addr, uint8_t value)
{
	if (addr < 0xC000 || addr > 0xDFFF)
		return;
	work_ram[addr - 0xC000] = value;
}

static uint8_t io_reg_read(uint16_t addr)
{
	if (addr == 0xFF00) {
		return joypad_read();
	} else if (addr == 0xFF01) {
		return serial_buffer_read();
	} else if (addr == 0xFF02) {
		return serial_control_read();
	} else if (addr >= 0xFF04 && addr <= 0xFF07) {
		return timer_reg_read(addr);
	} else if (addr == 0xFF0F) {
		return cpu_irq_read();
	} else if (addr >= 0xFF10 && addr <= 0xFF26) {
		/** TODO: audio */
		return 0;
	} else if (addr >= 0xFF30 && addr <= 0xFF3F) {
		/** TODO: wave patterns */
		return 0;
	} else if (addr >= 0xFF40 && addr <= 0xFF4B) {
		return ppu_reg_read(addr);
	} else if (addr == 0xFF50) {
		/** TODO: boot rom mapping control */
	} else {
		/** TODO: others */
	}
	return 0xFF;
}

static void io_reg_write(uint16_t addr, uint8_t value)
{
	if (addr == 0xFF00) {
		joypad_write(value);
	} else if (addr == 0xFF01) {
		serial_buffer_write(value);
	} else if (addr == 0xFF02) {
		serial_control_write(value);
	} else if (addr >= 0xFF04 && addr <= 0xFF07) {
		timer_reg_write(addr, value);
	} else if (addr == 0xFF0F) {
		cpu_irq_write(value);
	} else if (addr >= 0xFF10 && addr <= 0xFF26) {
		/** TODO: audio*/
	} else if (addr >= 0xFF30 && addr <= 0xFF3F) {
		/** TODO: wave patterns */
	} else if (addr >= 0xFF40 && addr <= 0xFF4B) {
		ppu_reg_write(addr, value);
	} else if (addr == 0xFF50) {
		mbc_bootmap_write(value);
	} else {
		/** TODO: others */
	}
}

uint8_t bus_read8(uint16_t addr)
{
	if (addr <= 0x7FFF) {
		/** ROM bank */
		return mbc_rom_read(addr);
	} else if (addr <= 0x9FFF) {
		return ppu_vram_read(addr);
	} else if (addr <= 0xBFFF) {
		/** External RAM */
		return mbc_ram_read(addr);
	} else if (addr <= 0xDFFF) {
		return work_ram_read(addr);
	} else if (addr <= 0xFDFF) {
		/** Echo RAM, use of this area prohibited */
		return 0xFF;
	} else if (addr <= 0xFE9F) {
		return ppu_oam_read(addr);
	} else if (addr <= 0xFEFF) {
		/** Use of this area is prohibited */
		return 0xFF;
	} else if (addr <= 0xFF7F) {
		return io_reg_read(addr);
	} else if (addr <= 0xFFFE) {
		return high_ram_read(addr);
	} else if (addr == 0xFFFF) {
		return cpu_ie_read();
	}

	return 0xFF;
}

void bus_write8(uint16_t addr, uint8_t value)
{
	if (addr <= 0x7FFF) {
		mbc_ioctl(addr, value);
	} else if (addr <= 0x9FFF) {
		ppu_vram_write(addr, value);
	} else if (addr <= 0xBFFF) {
		/** External RAM */
		mbc_ram_write(addr, value);
	} else if (addr <= 0xDFFF) {
		work_ram_write(addr, value);
	} else if (addr <= 0xFDFF) {
		/** Echo RAM, use of this area prohibited */
	} else if (addr <= 0xFE9F) {
		ppu_oam_write(addr, value);
	} else if (addr <= 0xFEFF) {
		/** Use of this area is prohibited */
	} else if (addr <= 0xFF7F) {
		io_reg_write(addr, value);
	} else if (addr <= 0xFFFE) {
		high_ram_write(addr, value);
	} else if (addr == 0xFFFF) {
		cpu_ie_write(value);
	}
}

uint16_t bus_read16(uint16_t addr)
{
	uint8_t low = bus_read8(addr);
	uint8_t high = bus_read8(addr + 1);
	return ((uint16_t)high << 8) | low;
}

void bus_write16(uint16_t addr, uint16_t value)
{
	bus_write8(addr, value & 0xFF);
	bus_write8(addr + 1, (value >> 8) & 0xFF);
}