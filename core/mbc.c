#include <stdint.h>
#include <stddef.h>

#include <gamebox/gamebox.h>
extern struct platform *platform;

struct mbc_ops {
	uint8_t (*rom_read)(uint16_t addr);
	void (*ioctl)(uint16_t addr, uint8_t value);
	uint8_t (*ram_read)(uint16_t addr);
	void (*ram_write)(uint16_t addr, uint8_t value);
};

struct mbc_type {
	uint8_t type;
	uint8_t boot_rom_mapped;

	uint8_t rom_size;
	uint8_t ram_size;

	uint8_t rom_id[2];

	uint8_t ioctl_reg[4];

	struct mbc_ops *ops;
	void *rd;
};

struct mbc_header {
	uint8_t entry[4];
	uint8_t logo[48];
	uint8_t title[16];
	uint8_t license_new[2];
	uint8_t sgb;
	uint8_t mbc_type;
	uint8_t rom_size;
	uint8_t ram_size;
	uint8_t dest;
	uint8_t license_old;
	uint8_t version;
	uint8_t header_checksum;
	uint8_t global_checksum[2];
};

static uint8_t mbc_check_header(struct mbc_header *header)
{
	uint8_t checksum = 0;

	uint8_t *start = &header->title[0];
	uint8_t *end = &header->header_checksum;
	for (uint8_t *item = start; item < end; item++) {
		checksum = checksum - *item - 1;
	}

	return checksum == header->header_checksum ? 0 : 1;
}

#define ROM_SIZE_32KB 0x00
#define ROM_SIZE_64KB 0x01
#define ROM_SIZE_128KB 0x02
#define ROM_SIZE_256KB 0x03
#define ROM_SIZE_512KB 0x04
#define ROM_SIZE_1MB 0x05
#define ROM_SIZE_2MB 0x06
#define ROM_SIZE_4MB 0x07

static uint8_t rom_size_mask[] = {
	0x01, 0x03, 0x07, 0x0F, 0x1F, 0X3F, 0x7F, 0xFF,
};

#define RAM_SIZE_0KB 0x00
#define RAM_SIZE_UNUSED 0x01
#define RAM_SIZE_8KB 0x02
#define RAM_SIZE_32KB 0x03

#define ROM_BANK_SIZE 16384
#define RAM_BANK_SIZE 8192

static uint8_t rom_banks[ROM_BANK_SIZE * 2];
static uint8_t ram_banks[RAM_BANK_SIZE * 4];

/**
 * DMG boot rom, from "dmg_boot.bin"
 * https://gbdev.gg8.se/files/roms/bootroms/dmg_boot.bin
 */
static uint8_t dmg_boot_rom[256] = {
	0x31, 0xfe, 0xff, 0xaf, 0x21, 0xff, 0x9f, 0x32, 0xcb, 0x7c, 0x20, 0xfb,
	0x21, 0x26, 0xff, 0x0e, 0x11, 0x3e, 0x80, 0x32, 0xe2, 0x0c, 0x3e, 0xf3,
	0xe2, 0x32, 0x3e, 0x77, 0x77, 0x3e, 0xfc, 0xe0, 0x47, 0x11, 0x04, 0x01,
	0x21, 0x10, 0x80, 0x1a, 0xcd, 0x95, 0x00, 0xcd, 0x96, 0x00, 0x13, 0x7b,
	0xfe, 0x34, 0x20, 0xf3, 0x11, 0xd8, 0x00, 0x06, 0x08, 0x1a, 0x13, 0x22,
	0x23, 0x05, 0x20, 0xf9, 0x3e, 0x19, 0xea, 0x10, 0x99, 0x21, 0x2f, 0x99,
	0x0e, 0x0c, 0x3d, 0x28, 0x08, 0x32, 0x0d, 0x20, 0xf9, 0x2e, 0x0f, 0x18,
	0xf3, 0x67, 0x3e, 0x64, 0x57, 0xe0, 0x42, 0x3e, 0x91, 0xe0, 0x40, 0x04,
	0x1e, 0x02, 0x0e, 0x0c, 0xf0, 0x44, 0xfe, 0x90, 0x20, 0xfa, 0x0d, 0x20,
	0xf7, 0x1d, 0x20, 0xf2, 0x0e, 0x13, 0x24, 0x7c, 0x1e, 0x83, 0xfe, 0x62,
	0x28, 0x06, 0x1e, 0xc1, 0xfe, 0x64, 0x20, 0x06, 0x7b, 0xe2, 0x0c, 0x3e,
	0x87, 0xe2, 0xf0, 0x42, 0x90, 0xe0, 0x42, 0x15, 0x20, 0xd2, 0x05, 0x20,
	0x4f, 0x16, 0x20, 0x18, 0xcb, 0x4f, 0x06, 0x04, 0xc5, 0xcb, 0x11, 0x17,
	0xc1, 0xcb, 0x11, 0x17, 0x05, 0x20, 0xf5, 0x22, 0x23, 0x22, 0x23, 0xc9,
	0xce, 0xed, 0x66, 0x66, 0xcc, 0x0d, 0x00, 0x0b, 0x03, 0x73, 0x00, 0x83,
	0x00, 0x0c, 0x00, 0x0d, 0x00, 0x08, 0x11, 0x1f, 0x88, 0x89, 0x00, 0x0e,
	0xdc, 0xcc, 0x6e, 0xe6, 0xdd, 0xdd, 0xd9, 0x99, 0xbb, 0xbb, 0x67, 0x63,
	0x6e, 0x0e, 0xec, 0xcc, 0xdd, 0xdc, 0x99, 0x9f, 0xbb, 0xb9, 0x33, 0x3e,
	0x3c, 0x42, 0xb9, 0xa5, 0xb9, 0xa5, 0x42, 0x3c, 0x21, 0x04, 0x01, 0x11,
	0xa8, 0x00, 0x1a, 0x13, 0xbe, 0x20, 0xfe, 0x23, 0x7d, 0xfe, 0x34, 0x20,
	0xf5, 0x06, 0x19, 0x78, 0x86, 0x23, 0x05, 0x20, 0xfb, 0x86, 0x20, 0xfe,
	0x3e, 0x01, 0xe0, 0x50
};

#define MBC_NO_MBC 0x00
#define MBC_MBC1 0x01
#define MBC_MBC1_RAM 0x02
#define MBC_MBC1_RAM_BATTERY 0x03
#define MBC_MBC3 0x11
#define MBC_MBC3_RAM 0x12
#define MBC_MBC3_RAM_BATTERY 0x13
#define MBC_MBC5 0x19
#define MBC_MBC5_RAM 0x1A
#define MBC_MBC5_RAM_BATTERY 0x1B
#define MAX_MBC_SUPPORT 10
#define MBC_NO_SUCH_TYPE 0xCC

static uint8_t mbc_supported[MAX_MBC_SUPPORT] = {
	MBC_NO_MBC,
	MBC_MBC1,
	MBC_MBC1_RAM,
	MBC_MBC1_RAM_BATTERY,
};
static struct mbc_type mbc;

uint8_t mbc_rom_read(uint16_t addr)
{
	if (addr < 0x100 && mbc.boot_rom_mapped) {
		return dmg_boot_rom[addr];
	}

	if (mbc.type == MBC_NO_MBC) {
		return addr <= 0x7FFF ? rom_banks[addr] : 0xFF;
	}

	if (!mbc.ops || !mbc.ops->rom_read) {
		return 0xFF;
	}

	return mbc.ops->rom_read(addr);
}

void mbc_ioctl(uint16_t addr, uint8_t value)
{
	if (addr > 0x7FFF || !mbc.ops || !mbc.ops->ioctl) {
		return;
	}
	mbc.ops->ioctl(addr, value);
}

void mbc_bootmap_write(uint8_t value)
{
	if (!mbc.boot_rom_mapped) {
		return;
	}
	if (value) {
		mbc.boot_rom_mapped = 0;
	}
}

uint8_t mbc_ram_read(uint16_t addr)
{
	if (addr < 0xA000 || addr > 0xBFFF) {
		return 0xFF;
	}

	if (!mbc.ops || !mbc.ops->ram_read) {
		return 0xFF;
	}

	return mbc.ops->ram_read(addr);
}

void mbc_ram_write(uint16_t addr, uint8_t value)
{
	if (addr < 0xA000 || addr > 0xBFFF) {
		return;
	}

	if (!mbc.ops || !mbc.ops->ram_write) {
		return;
	}

	mbc.ops->ram_write(addr, value);
}

static uint8_t mbc1_rom_read(uint16_t addr)
{
	uint8_t mode = mbc.ioctl_reg[3];
	uint8_t rom_id_expected = 0;

	if (addr <= 0x3FFF) {
		if (mode == 1 && mbc.rom_size > ROM_SIZE_512KB) {
			rom_id_expected = (mbc.ioctl_reg[2] << 5);
			rom_id_expected &= rom_size_mask[mbc.rom_size];
		}

		if (mbc.rom_id[0] != rom_id_expected) {
			mbc.rom_id[0] = rom_id_expected;
			platform->read(mbc.rd, rom_banks, rom_id_expected);
		}
	} else if (addr <= 0x7FFF) {
		rom_id_expected = (mbc.ioctl_reg[2] << 5) | mbc.ioctl_reg[1];
		rom_id_expected &= rom_size_mask[mbc.rom_size];

		if (mbc.rom_id[1] != rom_id_expected) {
			mbc.rom_id[1] = rom_id_expected;
			platform->read(mbc.rd, rom_banks + ROM_BANK_SIZE,
				       rom_id_expected);
		}
	} else {
		return 0xFF;
	}

	return rom_banks[addr];
}

static void mbc1_ioctl(uint16_t addr, uint8_t value)
{
	/**
	 * 0000–1FFF: RAM Enable (Write Only)
	 * 2000–3FFF: ROM Bank Number (Write Only)
	 * 4000–5FFF: RAM Bank Number or Upper Bits of ROM Bank Number (Write Only)
	 * 6000–7FFF: Banking Mode Select (Write Only)
	 */
	if (addr <= 0x1FFF) {
		mbc.ioctl_reg[0] = value & 0x0F;
	} else if (addr <= 0x3FFF) {
		value &= 0x1F;
		if (!value) {
			value = 1;
		}
		mbc.ioctl_reg[1] = value;
	} else if (addr <= 0x5FFF) {
		value &= 0x03;
		mbc.ioctl_reg[2] = value;
	} else if (addr <= 0x7FFF) {
		mbc.ioctl_reg[3] = value & 0x01;
	}
}

static uint8_t mbc1_ram_read(uint16_t addr)
{
	uint8_t ram_enabled = (mbc.ioctl_reg[0] == 0x0A);
	if (!ram_enabled || mbc.ram_size < RAM_SIZE_8KB) {
		return 0xFF;
	}

	uint8_t ram_id = 0;
	if (mbc.ram_size == RAM_SIZE_32KB && mbc.ioctl_reg[3]) {
		ram_id = mbc.ioctl_reg[2];
	}

	return (ram_banks + RAM_BANK_SIZE * ram_id)[addr - 0xA000];
}

static void mbc1_ram_write(uint16_t addr, uint8_t value)
{
	uint8_t ram_enabled = (mbc.ioctl_reg[0] == 0x0A);
	if (!ram_enabled || mbc.ram_size < RAM_SIZE_8KB) {
		return;
	}

	uint8_t ram_id = 0;
	if (mbc.ram_size == RAM_SIZE_32KB && mbc.ioctl_reg[3]) {
		ram_id = mbc.ioctl_reg[2];
	}

	(ram_banks + RAM_BANK_SIZE * ram_id)[addr - 0xA000] = value;
}

static struct mbc_ops mbc1_ops = {
	.rom_read = mbc1_rom_read,
	.ioctl = mbc1_ioctl,
	.ram_read = mbc1_ram_read,
	.ram_write = mbc1_ram_write,
};

static uint8_t mbc_check_size(void)
{
	switch (mbc.type) {
	case MBC_NO_MBC:
		if (mbc.rom_size != ROM_SIZE_32KB ||
		    mbc.ram_size != RAM_SIZE_0KB) {
			return 1;
		}
		break;
	case MBC_MBC1:
		if (mbc.rom_size > ROM_SIZE_2MB ||
		    mbc.ram_size != RAM_SIZE_0KB) {
			return 1;
		}
		break;
	case MBC_MBC1_RAM:
	case MBC_MBC1_RAM_BATTERY:
		if (mbc.rom_size > ROM_SIZE_2MB ||
		    (mbc.rom_size > ROM_SIZE_512KB &&
		     mbc.ram_size > RAM_SIZE_8KB) ||
		    mbc.ram_size > RAM_SIZE_32KB ||
		    mbc.ram_size == RAM_SIZE_UNUSED) {
			return 1;
		}
		break;
	default:
		return 1;
	}

	return 0;
}

uint8_t mbc_init(const char *rom_path)
{
	mbc.rd = platform->open(rom_path);
	if (mbc.rd == NULL) {
		return 1;
	}

	int rc = platform->read(mbc.rd, rom_banks, 0);
	if (rc)
		goto error;

	struct mbc_header *mbc_header =
		(struct mbc_header *)(rom_banks + 0x100);
	if (mbc_check_header(mbc_header))
		goto error;

	rc = platform->read(mbc.rd, rom_banks + ROM_BANK_SIZE, 1);
	if (rc)
		goto error;

	mbc.type = MBC_NO_SUCH_TYPE;
	for (int i = 0; i < MAX_MBC_SUPPORT; i++) {
		if (mbc_supported[i] == mbc_header->mbc_type) {
			mbc.type = mbc_header->mbc_type;
			break;
		}
	}
	if (mbc.type == MBC_NO_SUCH_TYPE)
		goto error;

#ifdef USE_BOOT_ROM
	mbc.boot_rom_mapped = 1;
#else
	mbc.boot_rom_mapped = 0;
#endif

	mbc.rom_size = mbc_header->rom_size;
	mbc.ram_size = mbc_header->ram_size;
	if (mbc_check_size())
		goto error;

	mbc.rom_id[0] = 0;
	mbc.rom_id[1] = 1;

	mbc.ioctl_reg[0] = 0;
	mbc.ioctl_reg[1] = 1;
	mbc.ioctl_reg[2] = 0;
	mbc.ioctl_reg[3] = 0;

	switch (mbc.type) {
	case MBC_MBC1:
	case MBC_MBC1_RAM:
	case MBC_MBC1_RAM_BATTERY:
		mbc.ops = &mbc1_ops;
		break;
	default:
		mbc.ops = NULL;
		break;
	}

	return 0;

error:
	platform->close(mbc.rd);
	mbc.rd = NULL;
	return 1;
}

void mbc_exit(void)
{
	if (mbc.rd) {
		platform->close(mbc.rd);
		mbc.rd = NULL;
	}
}