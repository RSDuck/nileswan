/**
 * Copyright (c) 2024 Adrian Siekierka
 *
 * Nileswan Updater is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * Nileswan Updater is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with Nileswan Updater. If not, see <https://www.gnu.org/licenses/>.
 */

#include <nile/flash.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <wonderful.h>
#include <ws.h>
#include <ws/display.h>
#include <ws/hardware.h>
#include <ws/keypad.h>
#include <ws/system.h>
#include <wsx/zx0.h>
#include "crc16.h"
#include "manifest.h"
#include "nile.h"
#include "nilefs.h"
#include "text.h"

#define UNPACK_BUFFER ((uint8_t __far*) MK_FP(0x1000, 0x0000))

uint8_t update_manifest_data[512];
uint32_t flash_id;

#include "strings.inc"

static void updater_early_error(const char __far *text, uint16_t code) {
	text_scroll_up_middle(true);
	text_printf(screen_1, 0, TEXT_CENTERED, DISPLAY_HEIGHT >> 1, text, code);
	while(1);
}

static void updater_flash_error(void) {
	text_scroll_up_middle(true);
	text_puts(screen_1, 0, TEXT_CENTERED, 9, fatal_error);
	while(1);
}

void run_update_manifest(bool verify) {
	uint8_t flash_buffer[256];
	uint8_t verify_buffer[256];
	uint8_t *cmd_ptr = update_manifest_data;
	cmd_ptr += sizeof(um_header_t);

	int part = 0;
	while (*cmd_ptr) {
		text_scroll_up_middle(true);	
		++part;

		switch (*cmd_ptr) {
			case UM_CMD_END:
				return;
			case UM_CMD_FLASH:
			case UM_CMD_PACKED_FLASH:
			{
				um_flash_cmd_t *cmd = (um_flash_cmd_t*) cmd_ptr;
				cmd_ptr += sizeof(um_flash_cmd_t);

				text_printf(screen_1, 0, TEXT_CENTERED, 9, verify ? verifying_part : unpacking_part, part);
				// Extract data to SRAM
				if (cmd->cmd == UM_CMD_PACKED_FLASH) {
					wsx_zx0_decompress(UNPACK_BUFFER, MK_FP(cmd->load_segment, 0));
				} else {
					memcpy(UNPACK_BUFFER, MK_FP(cmd->load_segment, 0), cmd->unpacked_length);
				}
				// Calculate CRC16 sum
				uint16_t actual_crc = crc16(UNPACK_BUFFER, cmd->unpacked_length);
				if (actual_crc != cmd->expected_crc) {
					text_scroll_up_middle(true);
					text_printf(screen_1, 0, TEXT_CENTERED, DISPLAY_HEIGHT >> 1, crc_error_part, part);
					text_scroll_up_middle(false);
					text_printf(screen_1, 0, TEXT_CENTERED, DISPLAY_HEIGHT >> 1, crc_error_a_e, actual_crc, cmd->expected_crc);
					while(1);
				}

				uint32_t start_address = cmd->flash_address;
				if (start_address & 0xFF) {
					updater_early_error(corrupt_data, part);
				}
				
				if (!verify) {
					text_scroll_up_middle(true);
					text_printf(screen_1, 0, TEXT_CENTERED, 9, erasing_part, part);

					uint32_t end_address = start_address + cmd->unpacked_length;
					uint32_t erase_address = (start_address & ~0xFFF);
					while (erase_address < end_address) {
						if (!nile_flash_erase_part(NILE_FLASH_CMD_ERASE_4K, erase_address)) {
							updater_flash_error();
						}
						erase_address += 4096;
					}

					text_scroll_up_middle(true);
					text_printf(screen_1, 0, TEXT_CENTERED, 9, flashing_part, part);

					uint32_t i = 0;
					while (start_address < end_address) {
						uint32_t len = end_address - start_address;
						if (len > 256) len = 256;
						memcpy(flash_buffer, UNPACK_BUFFER + i, len);
						if (!nile_flash_write_page(flash_buffer, start_address, len)) {
							updater_flash_error();
						}
						if (!nile_flash_read(verify_buffer, start_address, len)) {
							updater_flash_error();
						}
						if (memcmp(verify_buffer, UNPACK_BUFFER + i, len)) {
							updater_flash_error();
						}
						start_address += len;
						i += len;
					}
				}
			} break;
			default:
				updater_early_error(corrupt_data, *cmd_ptr);
				break;
		}
	}
}

static uint32_t supported_flash_chips[] = {
	NILE_FLASH_ID_W25Q16JV_IQ,
	NILE_FLASH_ID_W25Q16JV_IM
};
#define SUPPORTED_FLASH_CHIPS_COUNT (sizeof(supported_flash_chips) / sizeof(uint32_t))
static bool flash_chip_supported(void) {
	for (int i = 0; i < SUPPORTED_FLASH_CHIPS_COUNT; i++)
		if (supported_flash_chips[i] == flash_id)
			return true;
	return false;
}

static const char __far version_template[] = "%d.%d.%d";
static void print_version(um_version_t __far* version, int x, int y) {
	text_printf(screen_1, 0, x, y, version_template, version->major, version->minor, version->patch);
}

__attribute__((interrupt, assume_ss_data))
void __far low_battery_nmi_handler(void) {
	text_clear();
	if (ws_system_color_active()) {
		MEM_COLOR_PALETTE(0)[0] = 0x0F00;
		MEM_COLOR_PALETTE(0)[1] = 0x0FFF;
	} else {
		outportw(IO_SCR_PAL(0), MONO_PAL_COLORS(7, 0, 2, 5));
	}
	text_put_screen(screen_1, 0, TEXT_CENTERED, (DISPLAY_HEIGHT - 2) >> 1,
		2, low_battery_screen);
	while(1);
}

static void wait_for_key(uint16_t key) {
	while (ws_keypad_scan());
	while ((ws_keypad_scan() & key) != key);
}

void main(void) {
	text_init();
	text_puts(screen_1, 0, TEXT_CENTERED, DISPLAY_HEIGHT >> 1, initializing_updater);

	ws_system_model_t model = ws_system_get_model();

	nile_io_unlock();
	nile_clear_seg_mask();
	outportw(IO_BANK_2003_RAM, 0);

	nile_flash_wake();

	flash_id = nile_flash_read_id();
	if (!flash_chip_supported()) {
		text_scroll_up_middle(true);
		text_printf(screen_1, 0, TEXT_CENTERED, DISPLAY_HEIGHT >> 1, unknown_flash_error, flash_id);
		while(1);
	}

	nile_flash_write_unlock_global();

	// == LOW BATTERY NMI ENABLED ==
	ws_cpuint_set_handler(CPUINT_IDX_NMI, low_battery_nmi_handler);
	outportb(IO_INT_NMI_CTRL, NMI_ON_LOW_BATTERY);	

	// Copy and validate update manifest
	uint16_t update_manifest_seg = (*((uint8_t __far*) MK_FP(0xF000, 0xFFF6))) << 8;
	uint16_t __far* update_manifest_start = MK_FP(update_manifest_seg, 0);
	if (update_manifest_start[0] > sizeof(update_manifest_data))
		updater_early_error(corrupt_data, update_manifest_seg);
	memset(update_manifest_data, 0, sizeof(update_manifest_data));
	memcpy(update_manifest_data, update_manifest_start + 2, update_manifest_start[0]);
	uint16_t actual_crc16 = crc16((const char*) update_manifest_data, update_manifest_start[0]);
	if (actual_crc16 != update_manifest_start[1])
		updater_early_error(corrupt_data, actual_crc16);

	run_update_manifest(true);

	// Display update screen
	text_clear();
	text_put_screen(screen_1, 0,
		1, (DISPLAY_HEIGHT - UPDATE_START_SCREEN_LINES) >> 1,
		UPDATE_START_SCREEN_LINES,
		update_start_screen);
	print_version(&((um_header_t*) update_manifest_data)->version,
		UPDATE_START_SCREEN_VER_X_OFFSET,
		UPDATE_START_SCREEN_VER_Y_OFFSET + 1);
	
	// Wait for [A] key
	wait_for_key(model == WS_MODEL_PCV2 ? KEY_PCV2_CIRCLE : KEY_A);

	outportb(IO_INT_NMI_CTRL, 0);
	// == LOW BATTERY NMI DISABLED ==

	text_clear();
	run_update_manifest(false);
	
	text_clear();
	text_put_screen(screen_1, 0,
		1, (DISPLAY_HEIGHT - UPDATE_SUCCESS_SCREEN_LINES) >> 1,
		UPDATE_SUCCESS_SCREEN_LINES,
		ws_system_is_color() ? update_success_screen : update_successM_screen);
	if (ws_system_is_color()) {
		wait_for_key(model == WS_MODEL_PCV2 ? KEY_PCV2_CIRCLE : KEY_A);

		outportb(IO_SYSTEM_CTRL3, SYSTEM_CTRL3_POWEROFF);
	}	
	while(1);
}
