#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

#include <raylib.h>
#include <gamebox/gamebox.h>

#define SZ_16KB 16384

static Color *pixels = NULL;
static Image image;
static Texture2D frame;

static void *rom_open(const char *rom)
{
	FILE *rd = fopen(rom, "rb");
	if (rd == NULL) {
		perror("fopen");
	}

	return rd;
}

static void rom_close(void *rd)
{
	if (rd == NULL) {
		return;
	}
	fclose((FILE *)rd);
}

static uint8_t rom_read(void *rd, uint8_t *buf, uint8_t bid)
{
	int rc = fseek((FILE *)rd, bid * SZ_16KB, SEEK_SET);
	if (rc) {
		perror("fseek");
		return rc;
	}

	if (fread(buf, 1, SZ_16KB, (FILE *)rd) != SZ_16KB) {
		printf("fread: no enough data\n");
		return 1;
	}

	return 0;
}

static uint8_t input_poll_keys(void)
{
	uint8_t keys = 0;

	if (IsKeyDown(KEY_D)) {
		keys |= GB_KEY_RIGHT;
	}
	if (IsKeyDown(KEY_A)) {
		keys |= GB_KEY_LEFT;
	}
	if (IsKeyDown(KEY_W)) {
		keys |= GB_KEY_UP;
	}
	if (IsKeyDown(KEY_S)) {
		keys |= GB_KEY_DOWN;
	}

	if (IsKeyDown(KEY_O)) {
		keys |= GB_KEY_A;
	}
	if (IsKeyDown(KEY_K)) {
		keys |= GB_KEY_B;
	}
	if (IsKeyDown(KEY_SPACE)) {
		keys |= GB_KEY_SELECT;
	}
	if (IsKeyDown(KEY_ENTER)) {
		keys |= GB_KEY_START;
	}

	return keys;
}

static void submit_line_pixels(uint8_t *line, uint8_t lid)
{
	static const Color pallete[4] = {
		{ 155, 188, 15, 255 },
		{ 139, 172, 15, 255 },
		{ 48, 98, 48, 255 },
		{ 15, 56, 15, 255 },
	};

	if (!line || lid >= 144) {
		return;
	}

	for (int i = 0; i < 160; i++) {
		uint8_t color = line[i] & 0x03;
		pixels[160 * lid + i] = pallete[color];
	}
}

static void serial_debug_print(uint8_t c)
{
	printf("%c", c);
}

static struct platform linux_platform = {
	.open = rom_open,
	.close = rom_close,
	.read = rom_read,

	.poll_input = input_poll_keys,
	.submit_line = submit_line_pixels,

	.serial_debug = serial_debug_print,
};

uint8_t platform_init(const char *rom)
{
	if (gb_register_platform(&linux_platform)) {
		printf("Fail: gb_register_platform: linux\n");
		return 1;
	}

	if (gb_init(rom)) {
		printf("Fail: gb_init: %s\n", rom);
		return 1;
	}

	InitWindow(160 + 10, 144 + 10, "gbox");

	pixels = (Color *)MemAlloc(160 * 144 * sizeof(Color));

	image.data = pixels;
	image.width = 160;
	image.height = 144;
	image.mipmaps = 1;
	image.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;

	frame = LoadTextureFromImage(image);
	return 0;
}

void platform_exit(uint8_t reason)
{
	UnloadImage(image);
	UnloadTexture(frame);
	CloseWindow();
}

uint8_t platform_run(void)
{
	SetTargetFPS(60);

	while (!WindowShouldClose()) {
		gb_run_frame();
		UpdateTexture(frame, pixels);

		BeginDrawing();
		ClearBackground(RAYWHITE);
		DrawTexture(frame, 5, 5, WHITE);
		EndDrawing();
	}

	return 0;
}
