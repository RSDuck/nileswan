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

#include <nile/mcu.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <wonderful.h>
#include <ws.h>
#include <wsx/zx0.h>
#include "crc16.h"
#include "manifest.h"
#include "nile.h"
#include "nilefs.h"
#include "console.h"
#include "input.h"
#include "strings.h"

#define IRAM_IMPLEMENTATION
#include "iram.h"

#define UNPACK_BUFFER ((uint8_t __far*) MK_FP(0x1000, 0x0000))

uint8_t update_manifest_data[512];
uint32_t flash_id;
bool mcu_restarted = false;
int update_part;
bool update_verify;

static void update_halt(void) {
	cpu_irq_disable();
	while(1) cpu_halt();
}

static void updater_early_error(const char __far *text, uint16_t code) {
	console_print_status(false);
	console_print_newline();
	console_printf(0, text, code);
	update_halt();
}

static void updater_flash_error(void) {
	console_print_status(false);
	console_print_newline();
	console_print(0, s_fatal_error);
	update_halt();
}

static void extract_um_cmd_flash(um_flash_cmd_t *cmd) {
	console_printf(0, update_verify ? s_verifying_part : s_unpacking_part, update_part);
	// Extract data to SRAM
	outportw(IO_BANK_2003_RAM, 0);
	if (cmd->cmd == UM_CMD_PACKED_FLASH) {
		wsx_zx0_decompress(UNPACK_BUFFER, MK_FP(cmd->load_segment, 0));
	} else {
		memcpy(UNPACK_BUFFER, MK_FP(cmd->load_segment, 0), cmd->unpacked_length);
	}
	// Calculate CRC16 sum
	uint16_t actual_crc = crc16(UNPACK_BUFFER, cmd->unpacked_length, 0);
	if (actual_crc != cmd->expected_crc) {
		console_print_status(false);
		console_print_newline();
		console_printf(0, s_crc_error_part, update_part);
		console_printf(0, s_crc_error_a_e, actual_crc, cmd->expected_crc);
		update_halt();
	}
}

static void run_um_cmd_flash(um_flash_cmd_t *cmd) {
	uint8_t flash_buffer[256];
	uint8_t verify_buffer[256];

	if (cmd->board_revision < 0x100 && inportb(IO_NILE_BOARD_REVISION) != cmd->board_revision)
		return;

	extract_um_cmd_flash(cmd);

	uint32_t start_address = cmd->flash_address;
	if (start_address & 0xFF) {
		updater_early_error(s_corrupt_data, start_address & 0xFF);
	}

	if (!update_verify) {
		console_print_status(true);
		console_print_newline();
		console_printf(0, s_erasing_part, update_part);

		uint32_t end_address = start_address + cmd->unpacked_length;
		uint32_t erase_address = (start_address & ~0xFFF);
		while (erase_address < end_address) {
			if (!nile_flash_erase_part(NILE_FLASH_CMD_ERASE_4K, erase_address)) {
				updater_flash_error();
			}
			erase_address += 4096;
		}

		console_print_status(true);
		console_print_newline();
		console_printf(0, s_flashing_part, update_part);

		uint32_t i = 0;
		while (start_address < end_address) {
			uint32_t len = end_address - start_address;
			if (len > sizeof(flash_buffer)) len = sizeof(flash_buffer);

			outportw(IO_BANK_2003_RAM, 0);
			memcpy(flash_buffer, UNPACK_BUFFER + i, len);
			if (!nile_flash_write_page(flash_buffer, start_address, len)) {
				updater_flash_error();
			}
			if (!nile_flash_read(verify_buffer, start_address, len)) {
				updater_flash_error();
			}

			outportw(IO_BANK_2003_RAM, 0);
			if (memcmp(verify_buffer, UNPACK_BUFFER + i, len)) {
				updater_flash_error();
			}

			start_address += len;
			i += len;
		}
	}

	console_print_status(true);
	console_print_newline();
}

static void run_um_cmd_mcu_flash(um_flash_cmd_t *cmd) {
	uint8_t flash_buffer[128];
	uint8_t verify_buffer[128];

	if (cmd->board_revision < 0x100 && inportb(IO_NILE_BOARD_REVISION) != cmd->board_revision)
		return;

	extract_um_cmd_flash(cmd);

	uint32_t start_address = cmd->flash_address;
	if (start_address & (NILE_MCU_FLASH_PAGE_SIZE - 1)) {
		updater_early_error(s_corrupt_data, start_address & (NILE_MCU_FLASH_PAGE_SIZE - 1));
	}

	if (!mcu_restarted) {
		console_print_status(true);
		console_print_newline();
		console_print(0, s_rebooting_mcu);

		if (!nile_mcu_reset(true)) {
			updater_flash_error();
		}

		mcu_restarted = true;
	}

	if (!update_verify) {
		console_print_status(true);
		console_print_newline();
		console_printf(0, s_erasing_part, update_part);

		uint32_t end_address = start_address + cmd->unpacked_length;
		uint16_t page_start = start_address / NILE_MCU_FLASH_PAGE_SIZE;
		uint16_t page_count = (end_address + NILE_MCU_FLASH_PAGE_SIZE - 1 - start_address) / NILE_MCU_FLASH_PAGE_SIZE;

		if (!nile_mcu_boot_erase_memory(page_start, page_count))
			updater_flash_error();

		console_print_status(true);
		console_print_newline();
		console_printf(0, s_flashing_part, update_part);

		start_address += NILE_MCU_FLASH_START;
		end_address += NILE_MCU_FLASH_START;

		uint32_t i = 0;
		while (start_address < end_address) {
			uint32_t len = end_address - start_address;
			if (len > sizeof(flash_buffer)) len = sizeof(flash_buffer);

			outportw(IO_BANK_2003_RAM, 0);
			memcpy(flash_buffer, UNPACK_BUFFER + i, len);
			if (!nile_mcu_boot_write_memory(start_address, flash_buffer, len)) {
				updater_flash_error();
			}
			if (!nile_mcu_boot_read_memory(start_address, verify_buffer, len)) {
				updater_flash_error();
			}

			outportw(IO_BANK_2003_RAM, 0);
			if (memcmp(verify_buffer, UNPACK_BUFFER + i, len)) {
				updater_flash_error();
			}

			start_address += len;
			i += len;
		}
	}

	console_print_status(true);
	console_print_newline();
}

void run_update_manifest(bool verify) {
	uint8_t *cmd_ptr = update_manifest_data;
	cmd_ptr += sizeof(um_header_t);

	update_verify = verify;
	update_part = 0;

	while (*cmd_ptr) {
		++update_part;

		switch (*cmd_ptr) {
			case UM_CMD_END:
				return;
			case UM_CMD_FLASH:
			case UM_CMD_PACKED_FLASH:
			{
				um_flash_cmd_t *cmd = (um_flash_cmd_t*) cmd_ptr;
				cmd_ptr += sizeof(um_flash_cmd_t);
				run_um_cmd_flash(cmd);
			} break;
			case UM_CMD_MCU_FLASH:
			case UM_CMD_MCU_PACKED_FLASH:
			{
				um_flash_cmd_t *cmd = (um_flash_cmd_t*) cmd_ptr;
				cmd_ptr += sizeof(um_flash_cmd_t);
				run_um_cmd_mcu_flash(cmd);
			} break;
			default:
				updater_early_error(s_corrupt_data, *cmd_ptr);
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

static const char __far version_template[] = "%d.%d.%d (%08lx)";
static void print_version(um_version_t __far* version) {
	console_printf(0, version_template, version->major, version->minor, version->patch,
		__builtin_bswap32(*((uint32_t*) version->commit_id)));
}

__attribute__((interrupt, assume_ss_data))
void __far low_battery_nmi_handler(void) {
	console_clear();
	if (ws_system_color_active()) {
		MEM_COLOR_PALETTE(0)[0] = 0x0F00;
		MEM_COLOR_PALETTE(0)[1] = 0x0FFF;
	} else {
		outportw(IO_SCR_PAL(0), MONO_PAL_COLORS(7, 0, 2, 5));
	}
	console_draw(0, 7, CONSOLE_FLAG_CENTER, s_low_battery);
	while(1);
}

void main(void) {
	cpu_irq_disable();
	ws_hwint_set_handler(HWINT_IDX_VBLANK, vblank_int_handler);
	ws_hwint_enable(HWINT_VBLANK);
	cpu_irq_enable();

	console_init();

	console_draw_header(s_update_title);
	console_print(0, s_initializing_updater);

	nile_io_unlock();
	nile_bank_unlock();

	nile_flash_wake();

	flash_id = nile_flash_read_id();
	if (!flash_chip_supported()) {
		console_print_status(false);
		console_print_newline();
		console_printf(0, s_unknown_flash_error, flash_id);
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
		updater_early_error(s_corrupt_data, update_manifest_seg);
	memset(update_manifest_data, 0, sizeof(update_manifest_data));
	memcpy(update_manifest_data, update_manifest_start + 2, update_manifest_start[0]);
	uint16_t actual_crc16 = crc16((const char*) update_manifest_data, update_manifest_start[0], 0);
	if (actual_crc16 != update_manifest_start[1])
		updater_early_error(s_corrupt_data, actual_crc16);

	console_print_status(true);
	console_print_newline();
	run_update_manifest(true);

	// Display update screen
	console_print_newline();
	console_print(CONSOLE_FLAG_MONOSPACE, s_nileswan_header);
	console_print(0, s_update_title);
	console_print_newline();
	console_print(0, s_update_version);
	print_version(&((um_header_t*) update_manifest_data)->version);
	console_print_newline();
	console_print_newline();
	console_print(0, s_update_disclaimer);
	console_print_newline();
	
	input_wait_key(KEY_A);
	
	outportb(IO_INT_NMI_CTRL, 0);
	// == LOW BATTERY NMI DISABLED ==

	console_print_newline();
	run_update_manifest(false);
	
	nile_flash_sleep();
	outportb(IO_NILE_POW_CNT, 0);

	console_print_newline();
	console_print(0, s_update_success);
	if (ws_system_is_color()) {
		console_print(0, s_press_a_shutdown);
		console_print_newline();
		input_wait_key(KEY_A);
		outportb(IO_SYSTEM_CTRL3, SYSTEM_CTRL3_POWEROFF);
	} else {
		console_print(0, s_manual_shutdown);
		console_print_newline();
	}

	update_halt();
}
