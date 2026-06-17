#include <stdint.h>
#include <stdio.h>

#include "cpu.h"
#include "device.h"

/**
 * Joypad device
 */
static struct {
	/**
	 * bit[5]: select buttons
	 * bit[4]: select d-pads
	 * NOTE: value of 0 means selected
	 */
	uint8_t select;
	/**
	 * bit[7:4]: buttons
	 * bit[3:0]: d-pads
	 */
	uint8_t keys;
} jp;

void joypad_reset(void)
{
	/**
	 * None of keys are selected or pressed
	 */
	jp.select = 0x30;
	jp.keys = 0xFF;
}

uint8_t joypad_read(void)
{
	if (jp.select == 0x20) {
		return 0xE0 | (jp.keys & 0x0F);
	} else if (jp.select == 0x10) {
		return 0xD0 | ((jp.keys >> 4) & 0x0F);
	} else {
		return 0xFF;
	}
}

void joypad_write(uint8_t select)
{
	jp.select = select & 0x30;
}

void joypad_report_keys(uint8_t keys)
{
	/**
	 * More than one keys can be reported at the same time,
	 * NOTE:
	 * 1) activa-low for a pressed key
	 * 2) Generate joypad interrupt only at the time a key is pressed
	 */
	if (jp.keys & keys) {
		cpu_irq_raise(IRQ_JOYPAD);
	}
	jp.keys = ~keys;
}

/**
 * Serial device
 */
static uint8_t sb;
static uint8_t sc;

uint8_t serial_buffer_read(void)
{
	return sb;
}

void serial_buffer_write(uint8_t value)
{
	sb = value;
	/** TODO: printf for debug only, remove it later */
	printf("%c", sb);
}

uint8_t serial_control_read(void)
{
	return sc;
}

void serial_control_write(uint8_t value)
{
	sc = value;
}

/**
 * Timer device
 */
static struct {
	uint16_t div;
	uint8_t tima;
	uint8_t tma;
	uint8_t tac;
} timer;

uint8_t timer_reg_read(uint16_t addr)
{
	switch (addr) {
	case 0xFF04:
		return (timer.div >> 6) & 0xFF;
	case 0xFF05:
		return timer.tima;
	case 0xFF06:
		return timer.tma;
	case 0xFF07:
		return timer.tac;
	default:
		return 0xFF;
	}
}

void timer_reg_write(uint16_t addr, uint8_t value)
{
	switch (addr) {
	case 0xFF04:
		timer.div = 0;
		break;
	case 0xFF05:
		timer.tima = value;
		break;
	case 0xFF06:
		timer.tma = value;
		break;
	case 0xFF07:
		timer.tac = value & 0x07;
		break;
	default:
		break;
	}
}

void timer_step(uint8_t ticks)
{
	static uint16_t m_cycles = 0;
	uint8_t increment = 0;

	timer.div += (ticks / 4);
	if (!(timer.tac & 0x04)) {
		return;
	}
	m_cycles += (ticks / 4);

	switch (timer.tac & 0x03) {
	case 0:
		increment = m_cycles / 256;
		m_cycles %= 256;
		break;
	case 1:
		increment = m_cycles / 4;
		m_cycles %= 4;
		break;
	case 2:
		increment = m_cycles / 16;
		m_cycles %= 16;
		break;
	case 3:
		increment = m_cycles / 64;
		m_cycles %= 64;
		break;
	}

	for (int i = 0; i < increment; i++) {
		timer.tima += 1;
		if (timer.tima == 0) {
			cpu_irq_raise(IRQ_TIMER);
			timer.tima = timer.tma;
		}
	}
}