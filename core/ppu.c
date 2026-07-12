#include <stdint.h>
#include <gamebox/gamebox.h>
#include "bus.h"
#include "cpu.h"

extern struct platform *platform;

/**
 * PPU registers read/write
 */
static struct {
#define LCDC_ENABLE 128
#define LCDC_WIN_MAP 64
#define LCDC_WIN_ENABLE 32
#define LCDC_TILE_DATA 16
#define LCDC_BG_MAP 8
#define LCDC_OBJ_SIZE 4
#define LCDC_OBJ_ENABLE 2
#define LCDC_BG_WIN_DISPLAY 1
	/** Reg addr: 0xFF40 */
	uint8_t lcdc;

#define STAT_LYC_INT 64
#define STAT_MODE2_INT 32
#define STAT_MODE1_INT 16
#define STAT_MODE0_INT 8
#define STAT_LYC_EQS_LY 4
#define STAT_PPU_MODE 3
	/** Reg addr: 0xFF41 */
	uint8_t stat;

	/** Reg addr: 0xFF42 */
	uint8_t scy;
	/** Reg addr: 0xFF43 */
	uint8_t scx;
	/** Reg addr: 0xFF44 */
	uint8_t ly;
	/** Reg addr: 0xFF45 */
	uint8_t lyc;
	/** Reg addr: 0xFF46 */
	uint8_t oam_dma;
	/** Reg addr: 0xFF47 */
	uint8_t bgp;
	/** Reg addr: 0xFF48 */
	uint8_t obp0;
	/** Reg addr: 0xFF49 */
	uint8_t obp1;
	/** Reg addr: 0xFF4A */
	uint8_t wy;
	/** Reg addr: 0xFF4B */
	uint8_t wx;

	uint16_t tcycles;
} ppu;

static inline uint8_t ppu_enabled(void)
{
	return !!(ppu.lcdc & LCDC_ENABLE);
}

static inline uint8_t ppu_obj_enabled(void)
{
	return !!(ppu.lcdc & LCDC_OBJ_ENABLE);
}

static inline uint8_t ppu_get_mode(void)
{
	return ppu.stat & STAT_PPU_MODE;
}

static void check_lyc_equals_ly(void)
{
	if (ppu.lyc == ppu.ly) {
		ppu.stat |= STAT_LYC_EQS_LY;
		if (ppu.stat & STAT_LYC_INT) {
			cpu_irq_raise(IRQ_LCD);
		}
	} else {
		ppu.stat &= ~STAT_LYC_EQS_LY;
	}
}

static inline void ppu_ly_inc()
{
	ppu.ly += 1;
	check_lyc_equals_ly();
}

static inline void ppu_ly_reset()
{
	ppu.ly = 0;
	check_lyc_equals_ly();
}

static inline void ppu_set_mode(uint8_t mode)
{
	/** setup the mode value */
	mode &= STAT_PPU_MODE;
	ppu.stat &= ~STAT_PPU_MODE;
	ppu.stat |= mode;

	if (mode == 3) {
		return;
	}

	/** check IRQ_VBLANK and IRQ_LCD */
	if (mode == 1) {
		cpu_irq_raise(IRQ_VBLANK);
	}

	if ((8 << mode) & ppu.stat & 0x38) {
		cpu_irq_raise(IRQ_LCD);
	}
}

uint8_t ppu_reg_read(uint16_t addr)
{
	switch (addr) {
	case 0xFF40:
		return ppu.lcdc;
	case 0xFF41:
		return ppu.stat;
	case 0xFF42:
		return ppu.scy;
	case 0xFF43:
		return ppu.scx;
	case 0xFF44:
		return ppu.ly;
	case 0xFF45:
		return ppu.lyc;
	case 0xFF46:
		return ppu.oam_dma;
	case 0xFF47:
		return ppu.bgp;
	case 0xFF48:
		return ppu.obp0;
	case 0xFF49:
		return ppu.obp1;
	case 0xFF4A:
		return ppu.wy;
	case 0xFF4B:
		return ppu.wx;
	default:
		return 0xFF;
	}
}

static void oam_dma_start(uint8_t addr_hi);

void ppu_reg_write(uint16_t addr, uint8_t value)
{
	switch (addr) {
	case 0xFF40:
		if (!ppu_enabled() && (value & LCDC_ENABLE)) {
			ppu.stat = (ppu.stat & ~0x03) | 0x02;
			ppu.ly = 0;
			ppu.tcycles = 0;
		} else if (ppu_enabled() && !(value & LCDC_ENABLE)) {
			ppu.stat &= ~0x03;
			ppu.ly = 0;
		}
		ppu.lcdc = value;
		break;
	case 0xFF41:
		/**bit[2:0] of stat is read only */
		ppu.stat &= 0x07;
		ppu.stat |= (value & 0xF8);
		break;
	case 0xFF42:
		ppu.scy = value;
		break;
	case 0xFF43:
		ppu.scx = value;
		break;
	case 0xFF44:
		/**ly is read only */
		break;
	case 0xFF45:
		ppu.lyc = value;
		break;
	case 0xFF46:
		ppu.oam_dma = value;
		oam_dma_start(value);
		break;
	case 0xFF47:
		ppu.bgp = value;
		break;
	case 0xFF48:
		ppu.obp0 = value;
		break;
	case 0xFF49:
		ppu.obp1 = value;
		break;
	case 0xFF4A:
		ppu.wy = value;
		break;
	case 0xFF4B:
		ppu.wx = value;
		break;
	default:
		break;
	}
}

/**
 * OAM read/write
 */
static uint8_t oam[160];

uint8_t ppu_oam_read(uint16_t addr)
{
	uint8_t mode = ppu_get_mode();
	if (addr < 0xFE00 || addr > 0xFE9F || mode >= 2) {
		return 0xFF;
	}
	return oam[addr - 0xFE00];
}

void ppu_oam_write(uint16_t addr, uint8_t value)
{
	uint8_t mode = ppu_get_mode();
	if (addr < 0xFE00 || addr > 0xFE9F || mode >= 2) {
		return;
	}
	oam[addr - 0xFE00] = value;
}

/**
 * OAM DMA
 */

static struct {
	uint16_t tcycles;
	uint16_t base_addr;
	uint8_t index;
#define DMA_STOPPED 0
#define DMA_RUNNING 1
	uint8_t state;
} dma;

static void oam_dma_start(uint8_t addr_hi)
{
	dma.tcycles = 0;
	dma.base_addr = (uint16_t)addr_hi << 8;
	dma.index = 0;
	dma.state = DMA_RUNNING;
}

static void oam_dma_run(uint8_t ticks)
{
	if (dma.state != DMA_RUNNING) {
		return;
	}

	dma.tcycles += ticks;
	while (dma.index < 160 && dma.tcycles >= 4) {
		oam[dma.index] = bus_read8(dma.base_addr + dma.index);
		dma.index++;
		dma.tcycles -= 4;
	}

	if (dma.index >= 160) {
		dma.tcycles = 0;
		dma.base_addr = 0;
		dma.index = 0;
		dma.state = DMA_STOPPED;
	}
}

/**
 * VRAM read/write
 */

#define SZ_2KB 2048
#define SZ_4KB 4096
#define SZ_6KB 6144
#define SZ_7KB 7168

/**
 * Tile data and tile map are stored in vram
 */
static uint8_t vram[8192];
static uint8_t *tile_datas[3] = { vram, vram + SZ_2KB, vram + SZ_4KB };
static uint8_t *tile_maps[2] = { vram + SZ_6KB, vram + SZ_7KB };

uint8_t ppu_vram_read(uint16_t addr)
{
	if (addr < 0x8000 || addr > 0x9FFF || ppu_get_mode() == 3) {
		return 0xFF;
	}
	return vram[addr - 0x8000];
}

void ppu_vram_write(uint16_t addr, uint8_t value)
{
	if (addr < 0x8000 || addr > 0x9FFF || ppu_get_mode() == 3) {
		return;
	}
	vram[addr - 0x8000] = value;
}

struct object_attr {
	uint8_t y;
	uint8_t x;
	uint8_t tile;
#define ATTR_PRIORITY 128
#define ATTR_Y_FLIP 64
#define ATTR_X_FLIP 32
#define ATTR_PALETTE 16
#define ATTR_DMG_UNUSED 15
	uint8_t attr;
};

/** 
 * object index that overlaped with current LY 
 */
#define MAX_OBJ_TOTAL 40
#define MAX_OBJ_ACTIVE 10
#define OBJ_NONE 0xFF
static uint8_t objs_active[MAX_OBJ_ACTIVE];

static void ppu_scan_oam(void)
{
	/**
	 * TODO: sort objects with x before scaning
	 */
	uint8_t obj_max;
	uint8_t obj_size;
	uint8_t i, j;
	struct object_attr *oa;

	for (i = 0; i < MAX_OBJ_ACTIVE; i++) {
		objs_active[i] = OBJ_NONE;
	}

	if (!ppu_obj_enabled()) {
		return;
	}

	obj_max = 0;
	obj_size = ppu.lcdc & LCDC_OBJ_SIZE ? 16 : 8;
	j = 0;
	oa = (struct object_attr *)oam;

	while (obj_max < MAX_OBJ_ACTIVE && j < MAX_OBJ_TOTAL) {
		if (oa->x > 0 && oa->x < 168 && ppu.ly + 16 >= oa->y &&
		    ppu.ly + 16 < oa->y + obj_size) {
			objs_active[obj_max++] = j;
		}

		j++;
		oa++;
	}
}

/**
 * Pixel color format
 * bit[7]: object attributes.7 which means priority
 * bit[6]: not a pixel, used when window/object has no pixel at that position
 * bit[3:2]: color index
 * bit[1:0]: color value
 */
#define PIXEL_COLOR_WHITE 0
#define PIXEL_COLOR_L_GRAY 1
#define PIXEL_COLOR_D_GRAY 2
#define PIXEL_COLOR_BLACK 3

#define COLOR_MASK 0x03
#define COLOR_ID_MASK 0x0C

#define PIXEL_COLOR_NONE 64
#define PIXEL_COLOR_PRIORITY 128

static uint8_t bg_win_tile_pixel(uint16_t tile_index, uint8_t x, uint8_t y)
{
	uint8_t *tile;
	uint8_t lo, hi, color_id;

	/** 0x8000 method when lcdc.4 == 1 */
	if (ppu.lcdc & LCDC_TILE_DATA) {
		tile = tile_datas[0] + 16 * tile_index;
	}
	/** 0x8800 method otherwise */
	else {
		tile = tile_index >= 128
			       ? tile_datas[1] + 16 * (tile_index % 128)
			       : tile_datas[2] + 16 * tile_index;
	}

	lo = *(tile + 2 * y) >> (7 - x);
	hi = *(tile + 2 * y + 1) >> (7 - x);
	color_id = ((hi & 0x1) << 1) | (lo & 0x1);

	return ((ppu.bgp >> (color_id * 2)) & 0x03) | (color_id << 2);
}

static uint8_t bg_pixel_at(uint8_t x, uint8_t y)
{
	if (!(ppu.lcdc & LCDC_BG_WIN_DISPLAY)) {
		return ppu.bgp & 0x03;
	}

	uint16_t tile_map_index;
	uint8_t *tile_map;

	uint16_t tile_index;

	x += ppu.scx;
	y += ppu.scy;

	tile_map_index = y / 8 * 32 + x / 8;
	tile_map = (ppu.lcdc & LCDC_BG_MAP) ? tile_maps[1] : tile_maps[0];
	tile_index = tile_map[tile_map_index];

	return bg_win_tile_pixel(tile_index, x % 8, y % 8);
}

static uint8_t win_pixel_at(uint8_t x, uint8_t y)
{
	if (!(ppu.lcdc & LCDC_BG_WIN_DISPLAY) ||
	    !(ppu.lcdc & LCDC_WIN_ENABLE)) {
		return PIXEL_COLOR_NONE;
	}

	uint8_t wx, wy;

	uint16_t tile_map_index;
	uint8_t *tile_map;

	uint16_t tile_index;

	wx = ppu.wx < 7 ? 0 : ppu.wx - 7;
	wy = ppu.wy;

	if (x < wx || y < wy) {
		return PIXEL_COLOR_NONE;
	}

	x -= wx;
	y -= wy;

	tile_map_index = y / 8 * 32 + x / 8;
	tile_map = (ppu.lcdc & LCDC_WIN_MAP) ? tile_maps[1] : tile_maps[0];
	tile_index = tile_map[tile_map_index];

	return bg_win_tile_pixel(tile_index, x % 8, y % 8);
}

static uint8_t obj_pixel_at(uint8_t x, uint8_t y)
{
	uint8_t obj_size;

	uint8_t obj = OBJ_NONE;
	uint8_t ox, oy;
	struct object_attr *oa;

	uint8_t *tile;
	uint8_t lo, hi, color_id;
	uint8_t obp;

	obj_size = ppu.lcdc & LCDC_OBJ_SIZE ? 16 : 8;
	x += 8;
	y += 16;

	for (int i = 0; i < MAX_OBJ_ACTIVE; i++) {
		if (objs_active[i] == OBJ_NONE) {
			break;
		}

		oa = (struct object_attr *)oam + objs_active[i];
		ox = oa->x;
		oy = oa->y;

		if (x >= ox && x < ox + 8 && y >= oy && y < oy + obj_size) {
			obj = i;
			break;
		}
	}

	if (obj == OBJ_NONE) {
		return PIXEL_COLOR_NONE;
	}

	x -= ox;
	y -= oy;

	if (oa->attr & ATTR_X_FLIP) {
		x = 7 - x;
	}
	if (oa->attr & ATTR_Y_FLIP) {
		y = obj_size - 1 - y;
	}

	tile = tile_datas[0] +
	       16 * ((uint16_t)oa->tile & (obj_size == 16 ? 0xFE : 0xFF));
	if (y >= 8) {
		tile += 16;
		y -= 8;
	}

	lo = *(tile + 2 * y) >> (7 - x);
	hi = *(tile + 2 * y + 1) >> (7 - x);
	obp = oa->attr & ATTR_PALETTE ? ppu.obp1 : ppu.obp0;
	color_id = ((hi & 0x1) << 1) | (lo & 0x1);

	return ((obp >> (color_id * 2)) & 0x03) | (color_id << 2) |
	       (oa->attr & ATTR_PRIORITY);
}

/**
 * The color values for current scan line
 */
uint8_t line_colors[160];

static void ppu_draw_line(void)
{
	uint8_t pixels[3];
	uint8_t color;
	uint8_t bg_win_priority;

	for (int i = 0; i < 160; i++) {
		pixels[0] = bg_pixel_at(i, ppu.ly);
		pixels[1] = win_pixel_at(i, ppu.ly);
		pixels[2] = obj_pixel_at(i, ppu.ly);

		color = pixels[0];

		if (pixels[1] != PIXEL_COLOR_NONE) {
			color = pixels[1];
		}

		if (pixels[2] != PIXEL_COLOR_NONE &&
		    (pixels[2] & COLOR_ID_MASK)) {
			bg_win_priority = pixels[2] & PIXEL_COLOR_PRIORITY;

			if (!bg_win_priority ||
			    (bg_win_priority && !(color & COLOR_ID_MASK))) {
				color = pixels[2];
			}
		}

		line_colors[i] = color & COLOR_MASK;
	}

	platform->submit_line(line_colors, ppu.ly);
}

void ppu_step(uint8_t ticks)
{
#define TCYCLES_MODE2_3 80
#define TCYCLES_MODE3_0 252
#define TCYCLES_PER_LINE 456

#define LINES_BEFORE_VBLANK 144
#define LINES_PER_FRAME 154

	oam_dma_run(ticks);

	if (!ppu_enabled()) {
		return;
	}

	ppu.tcycles += ticks;

	switch (ppu_get_mode()) {
	case 0:
		if (ppu.tcycles >= TCYCLES_PER_LINE) {
			ppu.tcycles %= TCYCLES_PER_LINE;
			ppu_ly_inc();

			if (ppu.ly < LINES_BEFORE_VBLANK) {
				ppu_set_mode(2);
			} else {
				ppu_set_mode(1);
			}
		}
		break;
	case 1:
		if (ppu.tcycles >= TCYCLES_PER_LINE) {
			ppu.tcycles %= TCYCLES_PER_LINE;
			ppu_ly_inc();

			if (ppu.ly >= LINES_PER_FRAME) {
				ppu_ly_reset();
				ppu_set_mode(2);
			}
		}
		break;
	case 2:
		if (ppu.tcycles >= TCYCLES_MODE2_3) {
			ppu_scan_oam();
			ppu_set_mode(3);
		}
		break;
	case 3:
		if (ppu.tcycles >= TCYCLES_MODE3_0) {
			ppu_draw_line();
			ppu_set_mode(0);
		}
		break;
	}
}