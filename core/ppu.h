#pragma once
#include <stdint.h>

uint8_t ppu_reg_read(uint16_t addr);
void ppu_reg_write(uint16_t addr, uint8_t value);

uint8_t ppu_oam_read(uint16_t addr);
void ppu_oam_write(uint16_t addr, uint8_t value);

uint8_t ppu_vram_read(uint16_t addr);
void ppu_vram_write(uint16_t addr, uint8_t value);

void ppu_step(uint8_t ticks);
