#include <stdint.h>
#include <stddef.h>

#include <gamebox/gamebox.h>
#include "cpu.h"
#include "ppu.h"
#include "device.h"
#include "mbc.h"

static uint8_t gb_step(void)
{
	uint8_t ticks = cpu_step();
	timer_step(ticks);
	ppu_step(ticks);

	return ticks;
}

struct platform *platform;

uint8_t gb_register_platform(struct platform *p)
{
	if (p == NULL || platform != NULL) {
		return 1;
	}
	platform = p;
	return 0;
}

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

void gb_run_frame(void)
{
	static uint32_t ticks = 0;

	uint8_t keys = platform->poll_input();
	joypad_report_keys(keys);

	while (ticks < 70224) {
		ticks += gb_step();
	}

	ticks %= 70224;
}