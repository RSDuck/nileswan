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
#include <nile.h>
#include <wsx/zx0.h>
#include "assets/tiles.h"
#include "ipc.h"
#include "util.h"

typedef enum {
	MENU_OPTION_QUICK_TEST_16MB,
	MENU_OPTION_QUICK_TEST_8MB,
	MENU_OPTION_MEMORY_TEST,
	MENU_OPTION_BOOT_RECOVERY_CURRENT,
	MENU_OPTION_BOOT_RECOVERY_FACTORY,
	MENU_OPTION_MANUFACTURING_TEST,
	MENU_OPTION_RETENTION,
	MENU_OPTION_SRAM_SPEED,
	MENU_OPTIONS_COUNT
} menu_option_t;

#define SCREEN ((uint16_t*) (0x3800 + (13 * 32 * 2)))

#define PSRAM_MAX_BANK_8MB 127
#define PSRAM_MAX_BANK_16MB 255
#define PSRAM_MAX_BANK PSRAM_MAX_BANK_8MB
#define SRAM_MAX_BANK 7

/* === Test code in external files === */

// tests_asm.s
extern void ram_fault_test(void *results, uint16_t bank_count);
#define TEST_MODE_DEFAULT 0
#define TEST_MODE_ONLY_READ 1
#define TEST_MODE_BOOL_PASS 254
#define TEST_MODE_BOOL_FAIL 255
extern uint8_t ram_fault_test_mode;
bool ram_fault_test_bool(uint16_t bank_count) {
	ram_fault_test_mode = TEST_MODE_BOOL_PASS;
	ram_fault_test(NULL, bank_count);
	bool result = ram_fault_test_mode == TEST_MODE_BOOL_PASS;
	ram_fault_test_mode = TEST_MODE_DEFAULT;
	return result;
}

/* === Utility functions === */

static void clear_screen(void) {
	ws_screen_fill_tiles(SCREEN, 0x120, 0, 0, 28, 18);
}

#define DRAW_STRING(x, y, s, pal) mem_expand_8_16(SCREEN + ((y) * 32) + (x), (s), sizeof(s) - 1, 0x100 | pal);
#define DRAW_STRING_CENTERED(y, s, pal) DRAW_STRING(((29 - sizeof(s)) >> 1), y, s, pal)
#define DRAW_STRING_DYNAMIC(x, y, s, pal) mem_expand_8_16(SCREEN + ((y) * 32) + (x), (s), strlen(s), 0x100 | pal);
#define DRAW_STRING_CENTERED_DYNAMIC(y, s, pal) DRAW_STRING_DYNAMIC(((30 - strlen(s)) >> 1), y, s, pal)

/* === Memory test === */

static void draw_pass_fail(uint8_t y, bool result) {
	mem_expand_8_16(SCREEN + ((y * 32)) + 22, result ? "PASS" : "FAIL", 4, SCR_ENTRY_PALETTE(result ? 3 : 2) | 0x100);
}

static void draw_result_byte(uint8_t y, uint8_t value, bool result) {
	uint16_t* dst = SCREEN + ((y * 32)) + 22;
	if (result)
		print_hex_number(dst, value);
	else
		mem_expand_8_16(dst, "FAIL", 4, SCR_ENTRY_PALETTE(result ? 3 : 2) | 0x100);
}

static void wait_for_button(void) {
	DRAW_STRING_CENTERED(17, "press any button", 0);
	while(!ws_keypad_scan());
	while(ws_keypad_scan());
}

static bool ipc_buf_test() {
	outportw(IO_BANK_2003_RAM, 14);
	__far uint16_t* ipc_buf = MK_FP(0x1000, 0);

	for (uint16_t i = 0; i < sizeof(nile_ipc_t); i+=2)
		*(ipc_buf++) = i ^ (i >> 8);

	// IPC buffer should mirror
	for (uint16_t i = 0; i < sizeof(nile_ipc_t); i+=2)
	{
		if (*(ipc_buf++) != (i ^ (i >> 8)))
			return false;
	}

	return true;
}

void run_quick_test(int psram_max_bank) {
	clear_screen();
	DRAW_STRING_CENTERED(0, "quick test in progress", 0);

	DRAW_STRING(2, 2, "PSRAM write/read", 0);
	outportb(IO_CART_FLASH, 1);
	draw_pass_fail(2, ram_fault_test_bool(psram_max_bank));
	DRAW_STRING(2, 3, "SRAM write/read", 0);
	outportb(IO_CART_FLASH, 0);
	draw_pass_fail(3, ram_fault_test_bool(SRAM_MAX_BANK + 1));

	DRAW_STRING(2, 4, "IPC buf write/read", 0);
	draw_pass_fail(4, ipc_buf_test());

	ws_screen_fill_tiles(SCREEN, 0x120, 0, 0, 28, 1);
	DRAW_STRING_CENTERED(0, "quick test complete", 0);
	wait_for_button();
}

void run_full_memory_test(bool loop) {
	clear_screen();
	DRAW_STRING(3, 0, "PSRAM  Mem. Test  SRAM", 0);

	do {
		outportb(IO_CART_FLASH, 1);
		ram_fault_test(SCREEN + (1 * 32), PSRAM_MAX_BANK_16MB + 1);
		outportb(IO_CART_FLASH, 0);
		ram_fault_test(SCREEN + (1 * 32) + 19, SRAM_MAX_BANK + 1);
	} while(loop);
}

void run_read_memory_test(void) {
	clear_screen();
	DRAW_STRING_CENTERED(0, "testing SRAM read", 0);
	DRAW_STRING(19, 2, "76543210", 0);

	while (true) {
		outportb(IO_CART_FLASH, 0);
		ram_fault_test_mode = TEST_MODE_ONLY_READ;
		ram_fault_test(SCREEN + (3 * 32) + 19, SRAM_MAX_BANK + 1);
		ram_fault_test_mode = TEST_MODE_DEFAULT;
	}
	wait_for_button();
}

bool load_spi_flash(uint32_t _offset, uint16_t banks) {
	uint8_t buffer[256];
	uint32_t offset;

	outportb(IO_CART_FLASH, 1);
	nile_flash_wake();

	offset = _offset;
	// Quickly load all data from SPI flash to PSRAM
	DRAW_STRING(1, 1, "writing bank ", 0);
	for (uint16_t bank = PSRAM_MAX_BANK + 1 - banks; bank <= PSRAM_MAX_BANK; bank++) {
		outportw(IO_BANK_2003_RAM, bank);
		print_hex_number(SCREEN + (1 * 32) + 14, bank);

		for (int i = 0; i < 2; i++) {
			if (!nile_flash_read(MK_FP(0x1000, i << 15), offset, 0x8000)) {
				DRAW_STRING(1, 2, "flash read error", 0);
				print_hex_number(SCREEN + (3 * 32) + 1, offset >> 16);
				print_hex_number(SCREEN + (3 * 32) + 5, offset);
				return false;
			}
			offset += 0x8000;
		}
	}

	offset = _offset;
	// Verify data was loaded correctly (in case PSRAM is damaged)
	DRAW_STRING(1, 2, "verifying bank ", 0);
	for (uint16_t bank = PSRAM_MAX_BANK + 1 - banks; bank <= PSRAM_MAX_BANK; bank++) {
		outportw(IO_BANK_2003_RAM, bank);
		print_hex_number(SCREEN + (2 * 32) + 16, bank);

		for (int i = 0; i < (65536 / sizeof(buffer)); i++) {
			if (!nile_flash_read(buffer, offset, sizeof(buffer))) {
				DRAW_STRING(1, 3, "flash read error", 0);
				print_hex_number(SCREEN + (4 * 32) + 1, offset >> 16);
				print_hex_number(SCREEN + (4 * 32) + 5, offset);
				return false;
			}
			if (memcmp(MK_FP(0x1000, i * sizeof(buffer)), buffer, sizeof(buffer))) {
				DRAW_STRING(1, 3, "PSRAM write error", 0);
				print_hex_number(SCREEN + (4 * 32) + 1, offset >> 16);
				print_hex_number(SCREEN + (4 * 32) + 5, offset);
				return false;
			}
			offset += sizeof(buffer);
		}
	}

	return true;
}

void try_boot_rom(void) {
	outportb(IO_CART_FLASH, 0);
	outportw(IO_BANK_2003_ROM0, PSRAM_MAX_BANK);
	outportw(IO_BANK_2003_ROM1, PSRAM_MAX_BANK - 12);
	outportw(IO_BANK_2003_RAM, 0);
	outportw(IO_BANK_ROM_LINEAR, PSRAM_MAX_BANK >> 4);

	uint8_t __far* header = MK_FP(0xFFFF, 0x0000);
	if (header[0] != 0xEA) return;

	outportb(IO_DISPLAY_CTRL, 0);
	outportb(IO_SCR1_SCRL_Y, 0);
	asm volatile("ljmp $0x2FFF, $0x0000");
}

void main(void) {
	outportw(IO_BANK_2003_RAM, NILE_SEG_RAM_IPC);
	ipc_init(MEM_NILE_IPC);

	bool sram_io_speed_limit = true;

	if (ws_system_is_color()) {
		ws_system_mode_set(WS_MODE_COLOR);
		outportb(IO_SYSTEM_CTRL2, WS_MODE_COLOR);
		sram_io_speed_limit = false;
		MEM_COLOR_PALETTE(0)[0] = 0xFFF;
		MEM_COLOR_PALETTE(0)[1] = 0x000;
		MEM_COLOR_PALETTE(1)[0] = 0x000;
		MEM_COLOR_PALETTE(1)[1] = 0xFFF;
		MEM_COLOR_PALETTE(2)[0] = 0xFFF;
		MEM_COLOR_PALETTE(2)[1] = RGB(11, 0, 0);
		MEM_COLOR_PALETTE(3)[0] = 0xFFF;
		MEM_COLOR_PALETTE(3)[1] = RGB(0, 12, 0);
	} else {
		ws_display_set_shade_lut(SHADE_LUT_DEFAULT);
		outportw(IO_SCR_PAL_0, MONO_PAL_COLORS(0, 7, 0, 0));
		outportw(IO_SCR_PAL_1, MONO_PAL_COLORS(7, 0, 0, 0));
		outportw(IO_SCR_PAL_2, MONO_PAL_COLORS(0, 6, 0, 0));
		outportw(IO_SCR_PAL_3, MONO_PAL_COLORS(0, 4, 0, 0));
	}
	outportb(IO_SCR_BASE, SCR1_BASE(SCREEN));
	wsx_zx0_decompress((uint16_t*) 0x3200, gfx_tiles);
	outportw(IO_SCR1_SCRL_X, (13 * 8) << 8);

update_full_menu:
	clear_screen();
    outportb(IO_DISPLAY_CTRL, DISPLAY_SCR1_ENABLE);

	DRAW_STRING_CENTERED(0, "nileswan safe ipl1   0.1.0", 0);
	DRAW_STRING_CENTERED(17, "copyright (c) 2024-2025", 0);

	int test_pos = 0;
	int test_menu_y = (18 - MENU_OPTIONS_COUNT) >> 1;

	DRAW_STRING_CENTERED(test_menu_y+MENU_OPTION_QUICK_TEST_16MB, "quick memory test (16MB)", 0);
	DRAW_STRING_CENTERED(test_menu_y+MENU_OPTION_QUICK_TEST_8MB, "quick memory test (8MB)", 0);
	DRAW_STRING_CENTERED(test_menu_y+MENU_OPTION_MEMORY_TEST, "full memory test", 0);
	DRAW_STRING_CENTERED(test_menu_y+MENU_OPTION_MANUFACTURING_TEST, "manufacturing test", 0);
	DRAW_STRING_CENTERED(test_menu_y+MENU_OPTION_RETENTION, "test SRAM after reboot", 0);
	DRAW_STRING_CENTERED(test_menu_y+MENU_OPTION_BOOT_RECOVERY_CURRENT, "boot recovery", 0);
	DRAW_STRING_CENTERED(test_menu_y+MENU_OPTION_BOOT_RECOVERY_FACTORY, "boot factory recovery", 0);

	uint16_t keys_pressed = 0;
	uint16_t keys_held = 0;

update_dynamic_options:
	if (sram_io_speed_limit)
		DRAW_STRING_CENTERED(test_menu_y+MENU_OPTION_SRAM_SPEED, "io speed: slow", 0)
	else
		DRAW_STRING_CENTERED(test_menu_y+MENU_OPTION_SRAM_SPEED, "io speed: fast", 0);
	for (int i = 0; i < MENU_OPTIONS_COUNT; i++) {
		ws_screen_modify_tiles(SCREEN, 0x1FF, SCR_ENTRY_PALETTE(i == test_pos ? 1 : 0),
			0, test_menu_y + i, 28, 1);
	}

	while(1) {
		while (inportb(IO_LCD_LINE) != 144);

		int last_test_pos = test_pos;
		uint16_t keys = ws_keypad_scan();
		keys_pressed = keys & ~keys_held;
		keys_held = keys;

		if (keys_pressed & KEY_X1) {
			test_pos--;
			if (test_pos < 0) test_pos = MENU_OPTIONS_COUNT - 1;
		}
		if (keys_pressed & KEY_X3) {
			test_pos++;
			if (test_pos >= MENU_OPTIONS_COUNT) test_pos = 0;
		}
		if (keys_pressed & KEY_A) {
			while(ws_keypad_scan());
			switch (test_pos) {
			case MENU_OPTION_QUICK_TEST_16MB:
			case MENU_OPTION_QUICK_TEST_8MB: {
				run_quick_test(test_pos == MENU_OPTION_QUICK_TEST_16MB ? PSRAM_MAX_BANK_16MB : PSRAM_MAX_BANK_8MB);
				goto update_full_menu;
			} break;
			case MENU_OPTION_MEMORY_TEST: {
				run_full_memory_test(true);
			} break;
			case MENU_OPTION_RETENTION: {
				run_read_memory_test();
				goto update_full_menu;
			} break;
			case MENU_OPTION_BOOT_RECOVERY_FACTORY: {
				clear_screen();
				if (load_spi_flash(0x10000, 3)) {
					try_boot_rom();
				} else {
					wait_for_button();
				}
				goto update_full_menu;
			} break;
			case MENU_OPTION_BOOT_RECOVERY_CURRENT: {
				clear_screen();
				if (load_spi_flash(0x40000, 4)) {
					try_boot_rom();
				} else {
					wait_for_button();
				}
				goto update_full_menu;
			} break;
			case MENU_OPTION_MANUFACTURING_TEST: {
				clear_screen();
				run_full_memory_test(false);
				clear_screen();
				if (!ipc_buf_test()) {
					DRAW_STRING(1, 1, "IPC buffer error", 0);
					wait_for_button();
				} else {
					// set magic byte
					*((volatile uint16_t __far*) MK_FP(0x1000, 0x01FE)) = 0x3FA7;
					// run factory recovery
					if (load_spi_flash(0x10000, 3)) {
						try_boot_rom();
					} else {
						wait_for_button();
					}
				}
				goto update_full_menu;
			} break;
			case MENU_OPTION_SRAM_SPEED: {
				if (ws_system_color_active()) {
					sram_io_speed_limit = !sram_io_speed_limit;
					if (sram_io_speed_limit) {
						outportb(IO_SYSTEM_CTRL2, WS_MODE_COLOR | SYSTEM_CTRL2_SRAM_WAIT | SYSTEM_CTRL2_CART_IO_WAIT);
					} else {
						outportb(IO_SYSTEM_CTRL2, WS_MODE_COLOR);
					}
				}
			} break;
			}
			last_test_pos = -1;
		}
		if (last_test_pos != test_pos) {
			goto update_dynamic_options;
		}
	}
}
