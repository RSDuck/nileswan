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
#include <ws/hardware.h>
#include <ws/keypad.h>
#include "fatfs/ff.h"
#include "nileswan/nileswan.h"
#include "../build/data/default_font_bin.h"

#define SCREEN ((uint16_t*) 0x3800)
const uint8_t hexchars[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

#define PSRAM_MAX_BANK 127
#define SRAM_MAX_BANK 7

static FATFS fs;

static uint8_t load_menu(void) {
	FIL fp;

	uint8_t result;
	result = f_mount(&fs, "", 1);
	if (result != FR_OK) {
		return result;
	}
	result = f_open(&fp, "/MENU.WS", FA_READ);
	if (result != FR_OK) {
		return result;
	}

	outportb(IO_CART_FLASH, CART_FLASH_ENABLE);

	uint16_t offset = (f_size(&fp) - 1) ^ 0xFFFF;
	uint16_t bank = PSRAM_MAX_BANK - ((f_size(&fp) - 1) >> 16);

	while (bank <= PSRAM_MAX_BANK) {
		outportw(IO_BANK_2003_RAM, bank);
		if (offset < 0x8000) {
			if ((result = f_read(&fp, MK_FP(0x1000, offset), 0x8000 - offset, NULL)) != FR_OK) {
				return result;
			}
			offset = 0x8000;
		}
		if ((result = f_read(&fp, MK_FP(0x1000, offset), -offset, NULL)) != FR_OK) {
			return result;
		}
		offset = 0x0000;
		bank++;
	}

	return 0;
}

static const char header_string[] = "PSRAM  Self Test  SRAM";
static const char footer_string[] = "(c) nileswan 2024 @";

// tests_asm.s
void ram_fault_test(void *results, uint16_t bank_count);
// util.s
void memcpy8to16(void *dst, const void *src, uint16_t count, uint16_t fill_value);
void print_hex_number(void *dst, uint16_t value);

void run_selftest(void) {
	memcpy8to16(SCREEN + 3, header_string, sizeof(header_string) - 1, 0x0100);
	memcpy8to16(SCREEN + (17 * 32) + 9, footer_string, sizeof(footer_string) - 1, 0x0100);

	while (true) {
		outportb(IO_CART_FLASH, 1);
		ram_fault_test(SCREEN + (1 * 32), PSRAM_MAX_BANK + 1);
		outportb(IO_CART_FLASH, 0);
		ram_fault_test(SCREEN + (1 * 32) + 19, 8);
	}
}

#define KEYBIND_SELF_TEST (KEY_X3 | KEY_A)

void main(void) {
    ws_display_set_shade_lut(SHADE_LUT_DEFAULT);
    outportw(IO_SCR_PAL_0, MONO_PAL_COLORS(0, 7, 2, 5));
    outportw(IO_SCR_PAL_3, MONO_PAL_COLORS(0, 0, 0, 0));
    outportb(IO_SCR_BASE, SCR1_BASE(SCREEN));
    outportw(IO_SCR1_SCRL_X, 0);
	memcpy8to16((uint16_t*) 0x3200, default_font, default_font_size, 0x0000);
	memset(SCREEN, 0x6, (32 * 18 - 4) * sizeof(uint16_t));
    outportb(IO_DISPLAY_CTRL, DISPLAY_SCR1_ENABLE);

	uint16_t keys_pressed = ws_keypad_scan() & 0xDDD;
	if ((keys_pressed & KEYBIND_SELF_TEST) == KEYBIND_SELF_TEST) {
		while(1) run_selftest();
	}

	uint8_t result;
	if (!(result = load_menu())) {
		outportb(IO_CART_FLASH, 0);
		outportw(IO_BANK_2003_ROM0, PSRAM_MAX_BANK - 13);
		outportw(IO_BANK_2003_ROM1, PSRAM_MAX_BANK - 12);
		outportw(IO_BANK_2003_RAM, 0);
		outportw(IO_BANK_ROM_LINEAR, PSRAM_MAX_BANK >> 4);
		asm volatile("ljmp $0xFFFF, $0x0000");
	} else {
		// TODO: Add error handling
		while(1);
	}
}
