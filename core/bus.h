#pragma once
#include <stdint.h>

/* 8-bit bus read/write */
uint8_t bus_read8(uint16_t addr);
void bus_write8(uint16_t addr, uint8_t val);

/* 16-bit bus read/write */
uint16_t bus_read16(uint16_t addr);
void bus_write16(uint16_t addr, uint16_t val);