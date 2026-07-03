#include <stdint.h>

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
	uint8_t dma;
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

static inline uint8_t ppu_mode(void)
{
	return ppu.stat & STAT_PPU_MODE;
}

static inline void ppu_set_mode(uint8_t mode)
{
	ppu.stat &= ~STAT_PPU_MODE;
	ppu.stat |= (mode & STAT_PPU_MODE);
}

static uint8_t video_ram[8192];

uint8_t ppu_vram_read(uint16_t addr)
{
	if (addr < 0x8000 || addr > 0x9FFF) {
		return 0xFF;
	}
	return video_ram[addr - 0x8000];
}

void ppu_vram_write(uint16_t addr, uint8_t value)
{
	if (addr < 0x8000 || addr > 0x9FFF) {
		return;
	}
	video_ram[addr - 0x8000] = value;
}

void ppu_step(uint8_t ticks)
{
#define TCYCLES_MODE2_3 80
#define TCYCLES_MODE3_0 252
#define TCYCLES_PER_LINE 456

#define LINES_BEFORE_VBLANK 144
#define LINES_PER_FRAME 154

	static uint16_t tcycles = 0;
	uint8_t mode = ppu_mode();

	/** TODO: raise irq, OAM DMA maybe */
	tcycles += ticks;

	switch (mode) {
	case 0:
		if (tcycles >= TCYCLES_PER_LINE) {
			tcycles %= TCYCLES_PER_LINE;
			ppu.ly += 1;
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
			ppu.ly += 1;
		}
		if (ppu.ly >= LINES_PER_FRAME) {
			ppu.ly = 0;
			ppu_set_mode(2);
		}
		break;
	case 2:
		if (tcycles >= TCYCLES_MODE2_3) {
			/** TODO: scan OAM */
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