#pragma once
#include <stdint.h>

uint8_t mbc_rom_read(uint16_t addr);
void mbc_ioctl(uint16_t addr, uint8_t value);
void mbc_bootmap_write(uint8_t value);

uint8_t mbc_ram_read(uint16_t addr);
void mbc_ram_write(uint16_t addr, uint8_t value);

uint8_t mbc_init(const char *rom_path);
void mbc_exit(void);

/**
 * MBC types
 * 
 * MBC1
 * 
 * 0000–1FFF — RAM Enable (Write Only)
 * 2000–3FFF — ROM Bank Number (Write Only)
 * 4000–5FFF — RAM Bank Number — or — Upper Bits of ROM Bank Number (Write Only)
 * 6000–7FFF — Banking Mode Select (Write Only)
 * 
 * MBC3
 * 
 * 0000-1FFF - RAM and Timer Enable (Write Only)
 * 2000-3FFF - ROM Bank Number (Write Only)
 * 4000-5FFF - RAM Bank Number - or - RTC Register Select (Write Only)
 * 6000-7FFF - Latch Clock Data (Write Only)
 */