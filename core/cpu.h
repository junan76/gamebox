#pragma once
#include <stdint.h>

#define IRQ_VBLANK 0
#define IRQ_LCD 1
#define IRQ_TIMER 2
#define IRQ_SERIAL 3
#define IRQ_JOYPAD 4
#define IRQ_MAX 5

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
void cpu_irq_raise(uint8_t irq);