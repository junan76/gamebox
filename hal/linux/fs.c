#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <stdint.h>

#include <sys/errno.h>

#define SZ_16KB 16384

void *hal_open(const char *rom_path)
{
	FILE *rd = fopen(rom_path, "rb");
	if (rd == NULL) {
		perror(strerror(errno));
	}

	return rd;
}

void hal_close(void *rd)
{
	if (rd == NULL) {
		return;
	}
	fclose((FILE *)rd);
}

uint8_t hal_load(void *rd, uint8_t bank_id, uint8_t *bank)
{
	int rc = fseek((FILE *)rd, bank_id * SZ_16KB, SEEK_SET);
	if (rc) {
		perror(strerror(errno));
		return rc;
	}

	if (fread(bank, 1, SZ_16KB, (FILE *)rd) != SZ_16KB) {
        rc = ferror((FILE *)rd);
        printf("fread fail: %d\n", rc);
        return rc;
	}

	return 0;
}
