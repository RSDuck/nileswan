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

#include <stddef.h>
#include <string.h>
#include <wonderful.h>
#include <ws.h>
#include <wsx/zx0.h>
#include <nile.h>
#include <nilefs.h>
#include "assets/tiles.h"
#include "ipc.h"
#include "util.h"

#define SCREEN ((uint16_t*) (0x3800 + (12 * 32 * 2)))

#define PSRAM_MAX_BANK 127
#define SRAM_MAX_BANK 7

#define PROGRESS_BAR_Y ((13 * 8) - 4)

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
	uint8_t buffer[12];

	// print FatFs error
    outportw(IO_SCR_PAL_0, MONO_PAL_COLORS(7, 0, 2, 5));
    outportw(IO_SCR_PAL_3, MONO_PAL_COLORS(7, 7, 7, 7));
	mem_expand_8_16(SCREEN + (3 * 32) + 1, fatfs_error_header, sizeof(fatfs_error_header) - 1, 0x0100);
	print_hex_number(SCREEN + (3 * 32) + 22, (diskio_detail_code << 8) | result);

	const char *error_detail = NULL;
	switch (result) {
		case FR_DISK_ERR: error_detail = "Storage I/O error"; break;
		case FR_INT_ERR: case FR_INVALID_PARAMETER: error_detail = "Internal error"; break;
		case FR_NOT_READY: error_detail = "Storage not ready"; break;
		case FR_NO_FILE: case FR_NO_PATH: error_detail = "File not found"; break;
		case FR_NO_FILESYSTEM: error_detail = "FAT filesystem not found"; break;
	}
	if (error_detail != NULL) {
		mem_expand_8_16(SCREEN + (5 * 32) + ((28 - strlen(error_detail)) >> 1), error_detail, strlen(error_detail), 0x0100);
	}

	// try fetching flash ID
	nile_flash_wake();
	result = nile_flash_read_uuid(buffer);
	nile_flash_sleep();
	if (result) {
		for (int i = 0; i < 4; i++) {
			print_hex_number(SCREEN + (15 * 32) + 6 + 4 * i, __builtin_bswap16(((uint16_t*) buffer)[i]));
		}
	}

	// deinitialize hardware
	outportw(IO_NILE_SPI_CNT, NILE_SPI_CLOCK_CART);
	outportb(IO_NILE_POW_CNT, NILE_POW_MCU_RESET);
	cpu_irq_disable();
	while(1) cpu_halt();
}

static void update_progress(void) {
	uint16_t progress_end = ((uint32_t)(++bank_count) << 7) / bank_count_max;
	if (progress_end > 128) progress_end = 128;
	outportb(IO_SCR2_WIN_X2, (6 << 3) + progress_end - 1);
}

static uint8_t load_menu(void) {
	FIL fp;
	uint16_t bw;

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

			if ((result = f_read(&fp, MK_FP(0x1000, offset), 0x8000 - offset, &bw)) != FR_OK) {
				return result;
			}
			offset = 0x8000;
		}

		update_progress();

		if ((result = f_read(&fp, MK_FP(0x1000, offset), -offset, &bw)) != FR_OK) {
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
	nile_flash_sleep(); // Put flash chip to sleep

	// Back up IPC
	outportw(IO_BANK_2003_RAM, NILE_SEG_RAM_IPC);
	memcpy((void __far*) SCREEN, MEM_NILE_IPC, 0x200);

#ifndef PROGRAM_factory
	// Start loading warmboot image
	outportb(IO_NILE_WARMBOOT_CNT, 0);
	// Configure HBlank timer to count down until 20ms have passed
	outportw(IO_HBLANK_TIMER, 241);
	outportw(IO_TIMER_CTRL, HBLANK_TIMER_ENABLE);
#endif

	// Initialize IPC in backup
	ipc_init((nile_ipc_t __far*) SCREEN);

	outportw(IO_LCD_SHADE_01, SHADE_LUT_DEFAULT & 0xFF);
	outportw(IO_LCD_SHADE_45, SHADE_LUT_DEFAULT >> 16);
#ifndef PROGRAM_factory
	outportw(IO_SCR_PAL_0, MONO_PAL_COLORS(0, 3, 1, 2));
#else
	outportw(IO_SCR_PAL_0, MONO_PAL_COLORS(0, 7, 2, 5));
#endif
	outportw(IO_SCR_PAL_3, MONO_PAL_COLORS(0, 0, 0, 0));
	wsx_zx0_decompress((uint16_t*) 0x3200, gfx_tiles);

	// Initialize screen 2
	for (int i = 0; i < 28; i++)
		SCREEN[32 * 19 + i] = ((uint8_t) '-') | 0x100;
	outportw(IO_SCR2_SCRL_X, (248 - PROGRESS_BAR_Y) << 8);
	outportw(IO_SCR2_WIN_X1, (6 | (PROGRESS_BAR_Y << 5)) << 3);
	outportw(IO_SCR2_WIN_X2, ((6 | ((PROGRESS_BAR_Y + 8) << 5)) << 3) - 0x101);

#ifndef PROGRAM_factory
	while(inportw(IO_HBLANK_COUNTER));
	outportw(IO_SCR_PAL_0, MONO_PAL_COLORS(0, 7, 2, 5));
#endif

	// Restore IPC
	outportw(IO_BANK_2003_RAM, NILE_SEG_RAM_IPC);
	memcpy(MEM_NILE_IPC, (void __far*) SCREEN, 0x200);

	// Initialize screen 1
	memset(SCREEN, 0x6, (32 * 19 - 4) * sizeof(uint16_t));
	mem_expand_8_16(SCREEN + (8 * 32) + 12, swan_logo_map, 4, 0x100);
	mem_expand_8_16(SCREEN + (9 * 32) + 12, swan_logo_map + 4, 4, 0x100);
	mem_expand_8_16(SCREEN + (10 * 32) + 12, swan_logo_map + 8, 4, 0x100);
	outportw(IO_SCR1_SCRL_X, (13 * 8 - 4) << 8);

	// Show screens
	outportb(IO_SCR_BASE, SCR1_BASE(SCREEN) | SCR2_BASE(SCREEN));
	outportw(IO_DISPLAY_CTRL, DISPLAY_SCR1_ENABLE | DISPLAY_SCR2_ENABLE | DISPLAY_SCR2_WIN_INSIDE);

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
