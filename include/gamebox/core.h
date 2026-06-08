#pragma once

void mbc_init(const char *rom_path);
uint8_t cpu_step(void);
void cpu_reset(void);