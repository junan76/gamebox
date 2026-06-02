#pragma once
#include <stdint.h>

uint8_t cpu_step(void);

/**
 * IE register read/write.
 * Reg address: 0xFFFF
 */
uint8_t cpu_ie_read();
void cpu_ie_write(uint8_t value);

/**
 * IF register read/write.
 * Reg address: 0xFF0F
 */
uint8_t cpu_irq_read();
void cpu_irq_write(uint8_t value);