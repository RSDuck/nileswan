/**
 * Copyright (c) 2024 Kemal Afzal
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
#ifndef __MCU_COMM_H__
#define __MCU_COMM_H__

#include <stdbool.h>
#include <stdint.h>

#include <wonderful.h>

#define MCU_FLASH_START 0x08000000

#define MCU_FLASH_PAGE_SIZE 128
#define MCU_FLASH_SECTOR_SIZE 4096

bool mcu_comm_start();
void mcu_comm_stop();

bool mcu_comm_bootloader_version(uint8_t __far* version);
bool mcu_comm_bootloader_erase_memory(uint16_t page_address, uint8_t num_pages);
bool mcu_comm_bootloader_read_memory(uint32_t addr, uint8_t __far* data, uint8_t size_minus_one);
bool mcu_comm_bootloader_write_memory(uint32_t addr, const uint8_t __far* data, uint8_t size_minus_one);
bool mcu_comm_bootloader_go(uint32_t addr);

#endif