#pragma once
#include <stdint.h>

#define GB_KEY_RIGHT 1
#define GB_KEY_LEFT 2
#define GB_KEY_UP 4
#define GB_KEY_DOWN 8
#define GB_KEY_A 16
#define GB_KEY_B 32
#define GB_KEY_SELECT 64
#define GB_KEY_START 128

struct platform {
	void *(*open)(const char *rom);
	void (*close)(void *rd);
	uint8_t (*read)(void *rd, uint8_t *buf, uint8_t bid);

	uint8_t (*poll_input)(void);
	void (*submit_line)(uint8_t *line, uint8_t ly);
	void (*submit_audio)(uint8_t *audio);

	void (*delay)(uint8_t ms);
	void (*serial_debug)(uint8_t c);
};

uint8_t gb_register_platform(struct platform *p);
uint8_t gb_init(const char *rom_path);
void gb_run_frame(void);