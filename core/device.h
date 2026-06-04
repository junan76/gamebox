#pragma once
#include <stdint.h>

/**
 * Serial buffer read/write
 * Reg address: 0xFF01
 */
uint8_t serial_buffer_read(void);
void serial_buffer_write(uint8_t value);

/**
 * Serial control read/write
 * Reg address: 0xFF02
 */
uint8_t serial_control_read(void);
void serial_control_write(uint8_t value);