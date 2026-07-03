#include <stdio.h>
#include <stdint.h>
#include <gamebox/core.h>

int main(int argc, char const *argv[])
{
	uint8_t rc = 0;
	rc = gb_init(argv[1]);
	while (1) {
		gb_step();
	}
	return 0;
}
