#include <stdint.h>

static uint8_t video_ram[8192];

uint8_t ppu_vram_read(uint16_t addr)
{
	if (addr < 0x8000 || addr > 0x9FFF) {
		return 0xFF;
	}
	return video_ram[addr - 0x8000];
}

void ppu_vram_write(uint16_t addr, uint8_t value)
{
	if (addr < 0x8000 || addr > 0x9FFF) {
		return;
	}
	video_ram[addr - 0x8000] = value;
}