/**
 * Copyright (c) 2024 Adrian Siekierka
 *
 * Nileswan IPL1 is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * Nileswan IPL1 is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with Nileswan IPL1. If not, see <https://www.gnu.org/licenses/>.
 */

#include <nile/hardware.h>
#include <stddef.h>
#include <string.h>
#include <ws.h>
#include <ws/hardware.h>
#include <wsx/zx0.h>
#include <nile.h>
#include <nilefs.h>
#include "../../build/assets/tiles.h"
#include "util.h"

#define SCREEN ((uint16_t*) (0x3800 + (13 * 32 * 2)))

#define PSRAM_MAX_BANK 127
#define SRAM_MAX_BANK 7

// tests_asm.s
void ram_fault_test(void *results, uint16_t bank_count);
static FATFS fs;
static uint16_t bank_count;
static uint16_t bank_count_max;
static uint16_t progress_pos;

static const char fatfs_error_header[] = "TF card read failed (    )";

extern uint8_t diskio_detail_code;

__attribute__((noreturn))
static void report_fatfs_error(uint8_t result) {
	// deinitialize hardware
	outportw(IO_NILE_SPI_CNT, NILE_SPI_CLOCK_CART);
	outportb(IO_NILE_POW_CNT, NILE_POW_MCU_RESET);
	
    outportw(IO_SCR_PAL_0, MONO_PAL_COLORS(7, 0, 2, 5));
    outportw(IO_SCR_PAL_3, MONO_PAL_COLORS(7, 7, 7, 7));
	memcpy8to16(SCREEN + (3 * 32) + 1, fatfs_error_header, sizeof(fatfs_error_header) - 1, 0x0100);
	print_hex_number(SCREEN + (3 * 32) + 22, (diskio_detail_code << 8) | result);

	const char *error_detail = NULL;
	switch (result) {
		case FR_DISK_ERR: error_detail = "Disk I/O error"; break;
		case FR_INT_ERR: case FR_INVALID_PARAMETER: error_detail = "Internal error"; break;
		case FR_NOT_READY: error_detail = "Drive not ready"; break;
		case FR_NO_FILE: case FR_NO_PATH: error_detail = "File not found"; break;
		case FR_NO_FILESYSTEM: error_detail = "FAT filesystem not found"; break;
	}
	if (error_detail != NULL) {
		memcpy8to16(SCREEN + ((17 - 2) * 32) + ((28 - strlen(error_detail)) >> 1), error_detail, strlen(error_detail), 0x0100);
	}

	while(1);
}

static void update_progress(void) {
	uint16_t progress_end = ((++bank_count) << 4) / bank_count_max;
	for (; progress_pos < progress_end; progress_pos++) {
		SCREEN[13 * 32 + 6 + progress_pos] = ((uint8_t) '-') | 0x100;
	}
}

static uint8_t load_menu(void) {
	FIL fp;

	uint8_t result;
	result = f_mount(&fs, "", 1);
	if (result != FR_OK) {
		return result;
	}
	result = f_open(&fp, "/NILESWAN/MENU.WS", FA_READ);
	if (result != FR_OK) {
		return result;
	}

	outportb(IO_CART_FLASH, CART_FLASH_ENABLE);

	uint16_t offset = (f_size(&fp) - 1) ^ 0xFFFF;
	uint16_t bank = PSRAM_MAX_BANK - ((f_size(&fp) - 1) >> 16);

	bank_count = 0;
	bank_count_max = (PSRAM_MAX_BANK + 1 - bank) * 2 - (offset >= 0x8000 ? 1 : 0);
	progress_pos = 0;

	while (bank <= PSRAM_MAX_BANK) {
		outportw(IO_BANK_2003_RAM, bank);
		if (offset < 0x8000) {
			update_progress();

			if ((result = f_read(&fp, MK_FP(0x1000, offset), 0x8000 - offset, NULL)) != FR_OK) {
				return result;
			}
			offset = 0x8000;
		}

		update_progress();

		if ((result = f_read(&fp, MK_FP(0x1000, offset), -offset, NULL)) != FR_OK) {
			return result;
		}

		offset = 0x0000;
		bank++;
	}

	return 0;
}

const uint8_t swan_logo_map[] = {0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B};

void main(void) {
	outportb(IO_SYSTEM_CTRL2, 0x00); // Disable SRAM/IO wait states

	// Initialize IPC area
	outportw(IO_BANK_2003_RAM, NILE_SEG_RAM_IPC);
	if (MEM_NILE_IPC->magic != NILE_IPC_MAGIC) {
		MEM_NILE_IPC->magic = NILE_IPC_MAGIC;
		memset(((uint8_t __far*) MEM_NILE_IPC) + 2, 0, sizeof(nile_ipc_t) - 2);

		MEM_NILE_IPC->boot_entrypoint = *((uint8_t*) 0x3800);
		memcpy(&(MEM_NILE_IPC->boot_regs), (void*) 0x3802, 184 + 24);
	}

    ws_display_set_shade_lut(SHADE_LUT_DEFAULT);
    outportw(IO_SCR_PAL_0, MONO_PAL_COLORS(0, 7, 2, 5));
    outportw(IO_SCR_PAL_3, MONO_PAL_COLORS(0, 0, 0, 0));
    outportb(IO_SCR_BASE, SCR1_BASE(SCREEN));
	wsx_zx0_decompress((uint16_t*) 0x3200, gfx_tiles);
	memset(SCREEN, 0x6, (32 * 19 - 4) * sizeof(uint16_t));

	memcpy8to16(SCREEN + (8 * 32) + 12, swan_logo_map, 4, 0x100);
	memcpy8to16(SCREEN + (9 * 32) + 12, swan_logo_map + 4, 4, 0x100);
	memcpy8to16(SCREEN + (10 * 32) + 12, swan_logo_map + 8, 4, 0x100);
	outportw(IO_SCR1_SCRL_X, (14 * 8 - 4) << 8);

    outportb(IO_DISPLAY_CTRL, DISPLAY_SCR1_ENABLE);

	uint8_t result;
	if (!(result = load_menu())) {
	    outportb(IO_DISPLAY_CTRL, 0);
	    outportb(IO_SCR1_SCRL_Y, 0);
		outportb(IO_CART_FLASH, 0);
		outportw(IO_BANK_2003_ROM0, PSRAM_MAX_BANK - 13);
		outportw(IO_BANK_2003_ROM1, PSRAM_MAX_BANK - 12);
		outportw(IO_BANK_2003_RAM, 0);
		outportw(IO_BANK_ROM_LINEAR, PSRAM_MAX_BANK >> 4);
		asm volatile("ljmp $0xFFFF, $0x0000");
	} else {
		report_fatfs_error(result);
	}
}