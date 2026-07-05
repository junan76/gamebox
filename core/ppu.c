#include <stdint.h>
#include "bus.h"
#include "cpu.h"

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
	/** Reg addr: 0x40 */
	uint8_t lcdc;

#define STAT_LYC_INT 64
#define STAT_MODE2_INT 32
#define STAT_MODE1_INT 16
#define STAT_MODE0_INT 8
#define STAT_LYC_EQS_LY 4
#define STAT_PPU_MODE 3
	/** Reg addr: 0x41 */
	uint8_t stat;

	/** Reg addr: 0x42 */
	uint8_t scy;
	/** Reg addr: 0x43 */
	uint8_t scx;
	/** Reg addr: 0x44 */
	uint8_t ly;
	/** Reg addr: 0x45 */
	uint8_t lyc;
	/** Reg addr: 0x46 */
	uint8_t oam_dma;
	/** Reg addr: 0x47 */
	uint8_t bgp;
	/** Reg addr: 0x48 */
	uint8_t obp0;
	/** Reg addr: 0x49 */
	uint8_t obp1;
	/** Reg addr: 0x4A */
	uint8_t wy;
	/** Reg addr: 0x4B */
	uint8_t wx;
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

#define LY_RESET 0
#define LY_INCREASE 1
static void ppu_update_ly(uint8_t op)
{
	if (op == LY_RESET) {
		ppu.ly = 0;
	} else if (op == LY_INCREASE) {
		ppu.ly += 1;
	} else {
		return;
	}

	if (ppu.ly == ppu.lyc) {
		ppu.stat |= STAT_LYC_EQS_LY;
		if (ppu.stat & STAT_LYC_INT) {
			cpu_irq_raise(IRQ_LCD);
		}
	} else {
		ppu.stat &= ~STAT_LYC_EQS_LY;
	}
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

	if ((mode << 3) & ppu.stat & 0x38) {
		cpu_irq_raise(IRQ_LCD);
	}
}

uint8_t ppu_reg_read(uint16_t addr)
{
	switch (addr) {
	case 0x40:
		return ppu.lcdc;
	case 0x41:
		return ppu.stat;
	case 0x42:
		return ppu.scy;
	case 0x43:
		return ppu.scx;
	case 0x44:
		return ppu.ly;
	case 0x45:
		return ppu.lyc;
	case 0x46:
		return ppu.oam_dma;
	case 0x47:
		return ppu.bgp;
	case 0x48:
		return ppu.obp0;
	case 0x49:
		return ppu.obp1;
	case 0x4A:
		return ppu.wy;
	case 0x4B:
		return ppu.wx;
	default:
		return 0xFF;
	}
}

static void oam_dma_start(uint8_t addr_hi);

void ppu_reg_write(uint16_t addr, uint8_t value)
{
	switch (addr) {
	case 0x40:
		ppu.lcdc = value;
		break;
	case 0x41:
		/**bit[2:0] of stat is read only */
		ppu.stat &= 0x07;
		ppu.stat |= (value & 0xF8);
		break;
	case 0x42:
		ppu.scy = value;
		break;
	case 0x43:
		ppu.scx = value;
		break;
	case 0x44:
		/**ly is read only */
		break;
	case 0x45:
		ppu.lyc = value;
		break;
	case 0x46:
		oam_dma_start(value);
		break;
	case 0x47:
		ppu.bgp = value;
		break;
	case 0x48:
		ppu.obp0 = value;
		break;
	case 0x49:
		ppu.obp1 = value;
		break;
	case 0x4A:
		ppu.wy = value;
		break;
	case 0x4B:
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
	uint8_t mode = ppu_get_mode();
	/** OAM is accessible only when ppu is in mode 0 and 1 */
	if (dma.state != DMA_RUNNING || mode >= 2) {
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
static uint8_t vram[8192];

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
static uint8_t objs_active[MAX_OBJ_ACTIVE];

static void ppu_scan_oam(void)
{
	uint8_t obj_max;
	uint8_t obj_size;
	uint8_t i, j;
	struct object_attr *oa;

	if (!ppu_obj_enabled()) {
		return;
	}

	for (i = 0; i < MAX_OBJ_ACTIVE; i++) {
		objs_active[i] = 0xFF;
	}

	obj_max = 0;
	obj_size = ppu.lcdc & LCDC_OBJ_SIZE ? 16 : 8;
	j = 0;
	oa = (struct object_attr *)oam;

	while (obj_max < MAX_OBJ_ACTIVE && j < MAX_OBJ_TOTAL) {
		if (oa->x > 0 && oa->x < 168 && ppu.ly + 16 >= oa->y &&
		    ppu.ly + 16 <= oa->y + obj_size) {
			objs_active[obj_max++] = j;
		}

		j++;
		oa++;
	}
}

static void ppu_draw_line(void)
{
}

void ppu_step(uint8_t ticks)
{
#define TCYCLES_MODE2_3 80
#define TCYCLES_MODE3_0 252
#define TCYCLES_PER_LINE 456

#define LINES_BEFORE_VBLANK 144
#define LINES_PER_FRAME 154

	static uint16_t tcycles = 0;
	uint8_t mode = ppu_get_mode();

	tcycles += ticks;
	oam_dma_run(ticks);

	switch (mode) {
	case 0:
		if (tcycles >= TCYCLES_PER_LINE) {
			tcycles %= TCYCLES_PER_LINE;
			ppu_update_ly(LY_INCREASE);
		}
		if (ppu.ly < LINES_BEFORE_VBLANK) {
			ppu_set_mode(2);
		} else {
			ppu_set_mode(1);
		}
		break;
	case 1:
		if (tcycles >= TCYCLES_PER_LINE) {
			tcycles %= TCYCLES_PER_LINE;
			ppu_update_ly(LY_INCREASE);
		}
		if (ppu.ly >= LINES_PER_FRAME) {
			ppu_update_ly(LY_RESET);
			ppu_set_mode(2);
		}
		break;
	case 2:
		if (tcycles >= TCYCLES_MODE2_3) {
			ppu_scan_oam();
			ppu_set_mode(3);
		}
		break;
	case 3:
		if (tcycles >= TCYCLES_MODE3_0) {
			/** TODO: draw line pixels */
			ppu_set_mode(0);
		}
		break;
	}
}