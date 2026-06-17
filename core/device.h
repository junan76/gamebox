#pragma once
#include <stdint.h>

/**
 * Joypad register read and key report
 * Reg address: 0xFF00
 */
#define GB_KEY_RIGHT 1
#define GB_KEY_LEFT 2
#define GB_KEY_UP 4
#define GB_KEY_DOWN 8
#define GB_KEY_A 16
#define GB_KEY_B 32
#define GB_KEY_SELECT 64
#define GB_KEY_START 128

void joypad_reset(void);
uint8_t joypad_read(void);
void joypad_write(uint8_t select);
void joypad_report_keys(uint8_t key);

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

/**
 * Timer registers read/write, and timer_step
 */
uint8_t timer_reg_read(uint16_t addr);
void timer_reg_write(uint16_t addr, uint8_t value);
void timer_step(uint8_t ticks);
