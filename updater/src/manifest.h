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

#ifndef __MANIFEST_H__
#define __MANIFEST_H__

#include <stdbool.h>
#include <stdint.h>
#include <wonderful.h>

#define UM_CMD_END 0x00
#define UM_CMD_FLASH 0x01
#define UM_CMD_PACKED_FLASH 0x02

typedef struct __attribute__((packed)) {
	uint16_t major;
	uint16_t minor;
	uint16_t patch;
} um_version_t;

typedef struct __attribute__((packed)) {
    um_version_t version;
} um_header_t;

typedef struct __attribute__((packed)) {
	uint8_t cmd;
	uint16_t load_segment;
	uint16_t unpacked_length;
	uint32_t flash_address;
	uint16_t expected_crc;
} um_flash_cmd_t;

#endif /* __MANIFEST_H__ */
