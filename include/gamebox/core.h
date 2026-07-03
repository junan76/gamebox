#pragma once
#include <stdint.h>

uint8_t gb_init(const char *rom_path);
uint8_t gb_step(void);