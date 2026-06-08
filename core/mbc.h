#pragma once
#include <stdint.h>

uint8_t mbc_init(const char *rom_path);
void mbc_mapping_control_write(uint8_t value);
uint8_t mbc_rom_read(uint16_t addr);
uint8_t mbc_ram_external_read(uint16_t addr);
void mbc_ram_external_write(uint16_t addr, uint8_t value);