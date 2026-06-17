#include <stdio.h>
#include <stdint.h>
#include <gamebox/core.h>

int main(int argc, char const *argv[])
{
	mbc_init(argv[1]);
	joypad_reset();
	cpu_reset();
	while (1) {
		uint8_t ticks = cpu_step();
		timer_step(ticks);
	}
	return 0;
}
