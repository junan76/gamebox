#include <stdint.h>

#include <gamebox/hal.h>
#include "cpu.h"
#include "ppu.h"
#include "device.h"
#include "mbc.h"

uint8_t gb_init(const char *rom_path)
{
	uint8_t rc = mbc_init(rom_path);
	if (rc) {
		return rc;
	}

	joypad_reset();
	cpu_reset();

	return 0;
}

uint8_t gb_step(void)
{
	// uint8_t keys = hal_input_poll();
	joypad_report_keys(0);

	uint8_t ticks = cpu_step();
	timer_step(ticks);
	ppu_step(ticks);

	return ticks;
}