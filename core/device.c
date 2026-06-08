#include <stdint.h>
#include <stdio.h>

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