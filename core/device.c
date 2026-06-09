#include <stdint.h>
#include <stdio.h>

#include "cpu.h"

/**
 * Serial device.
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
 * Timer device.
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