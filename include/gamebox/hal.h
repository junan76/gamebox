#pragma once
#include <stdint.h>

void *hal_open(const char *rom_path);
void hal_close(void *rd);
uint8_t hal_load(void *rd, uint8_t bank_id, uint8_t *bank);