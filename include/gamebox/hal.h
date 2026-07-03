#pragma once
#include <stdint.h>

/**
 * fs
 */
void *hal_open(const char *rom_path);
void hal_close(void *rd);
uint8_t hal_load(void *rd, uint8_t *buf, uint8_t bank_id);

/**
 * input
 */
uint8_t hal_input_poll(void);

/**
 * graphics
 */
void hal_draw_line(uint8_t *line);

/**
 * audio
 */
void hal_audio_play(uint8_t audio);

/**
 * time
 */
void hal_delay(uint8_t ms);

/**
 * life cycle
 */
uint8_t hal_init(void);
void hal_exit(void);