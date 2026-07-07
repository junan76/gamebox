#include <stdio.h>
#include <stdint.h>
#include <gamebox/core.h>
#include <raylib.h>

extern uint8_t line_buf[160];
extern uint8_t line_id;

int main(int argc, char const *argv[])
{
	line_id = 0xFF;
#define SCREEN_WIDTH 256
#define SCREEN_HEIGHT 256

	SetTraceLogLevel(LOG_WARNING);
	InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "gbx");
	gb_init(argv[1]);

	Color *pixels = (Color *)MemAlloc(160 * 144 * sizeof(Color));
	Image im = {
		.data = pixels,
		.width = 160,
		.height = 144,
		.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,
		.mipmaps = 1,
	};

	Texture2D checked = LoadTextureFromImage(im);

	uint32_t ticks = 0;
	SetTargetFPS(60);
	while (!WindowShouldClose()) {
		while (ticks < 70224) {
			ticks += gb_step();
			if (line_id != 0xFF) {
				for (int i = 0; i < 160; i++) {
					uint8_t color = line_buf[i] & 0x03;
					switch (color) {
					case 0:
						pixels[160 * line_id + i].r =
							255;
						pixels[160 * line_id + i].g =
							255;
						pixels[160 * line_id + i].b =
							255;
						pixels[160 * line_id + i].a =
							255;
						break;
					case 1:
						pixels[160 * line_id + i].r =
							170;
						pixels[160 * line_id + i].g =
							170;
						pixels[160 * line_id + i].b =
							170;
						pixels[160 * line_id + i].a =
							255;
						break;
					case 2:
						pixels[160 * line_id + i].r =
							85;
						pixels[160 * line_id + i].g =
							85;
						pixels[160 * line_id + i].b =
							85;
						pixels[160 * line_id + i].a =
							255;
						break;
					case 3:
						pixels[160 * line_id + i].r = 0;
						pixels[160 * line_id + i].g = 0;
						pixels[160 * line_id + i].b = 0;
						pixels[160 * line_id + i].a =
							255;
						break;
					}
				}
				line_id = 0xFF;
			}
		}

		ticks = 0;
		UpdateTexture(checked, pixels);

		ClearBackground(RAYWHITE);
		BeginDrawing();
		DrawTexture(checked, 48, 56, WHITE);
		EndDrawing();
	}

	UnloadImage(im);
	CloseWindow();
	return 0;
}
