#pragma once
#include <stdint.h>

uint8_t ppu_vram_read(uint16_t addr);
void ppu_vram_write(uint16_t addr, uint8_t value);