#include <stdio.h>
#include <stdint.h>

extern uint8_t platform_init(const char *rom);
extern void platform_exit(uint8_t reason);
extern uint8_t platform_run(void);

int main(int argc, char const *argv[])
{
	uint8_t err = 0;

	if (platform_init(argv[1])) {
		printf("Fail: platform init\n");
		return 1;
	}

	err = platform_run();

	platform_exit(err);
	return 0;
}
