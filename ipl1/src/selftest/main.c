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
#include <ws.h>
#include "fatfs/ff.h"
#include "nileswan/nileswan.h"
#include "../../build/assets/tiles.h"
#include "util.h"

#define SCREEN ((uint16_t*) (0x3800 + (13 * 32 * 2)))

#define PSRAM_MAX_BANK 127
#define SRAM_MAX_BANK 7

// tests_asm.s
void ram_fault_test(void *results, uint16_t bank_count);

static const char header_string[] = "PSRAM  Self Test  SRAM";
static const char footer_string[] = "(c) nileswan 2024 @";

void run_selftest(void) {
	// deinitialize hardware
	outportw(IO_NILE_SPI_CNT, NILE_SPI_390KHZ);
	outportb(IO_NILE_POW_CNT, 0);

	memcpy8to16(SCREEN + 3, header_string, sizeof(header_string) - 1, 0x0100);
	memcpy8to16(SCREEN + (17 * 32) + 9, footer_string, sizeof(footer_string) - 1, 0x0100);

	while (true) {
		outportb(IO_CART_FLASH, 1);
		ram_fault_test(SCREEN + (1 * 32), PSRAM_MAX_BANK + 1);
		outportb(IO_CART_FLASH, 0);
		ram_fault_test(SCREEN + (1 * 32) + 19, 8);
	}
}

void main(void) {
    ws_display_set_shade_lut(SHADE_LUT_DEFAULT);
    outportw(IO_SCR_PAL_0, MONO_PAL_COLORS(0, 7, 2, 5));
    outportw(IO_SCR_PAL_3, MONO_PAL_COLORS(0, 0, 0, 0));
    outportb(IO_SCR_BASE, SCR1_BASE(SCREEN));
	lzsa2_decompress_small((uint16_t*) 0x3200, gfx_tiles);
	_nmemset(SCREEN, 0x6, (32 * 19 - 4) * sizeof(uint16_t));

	outportw(IO_SCR1_SCRL_X, (13 * 8) << 8);
    outportb(IO_DISPLAY_CTRL, DISPLAY_SCR1_ENABLE);

	while(1) run_selftest();
}
