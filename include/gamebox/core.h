#pragma once
#include <stdint.h>

uint8_t mbc_init(const char *rom_path);
uint8_t cpu_step(void);
void cpu_reset(void);
void joypad_reset(void);
void timer_step(uint8_t ticks);