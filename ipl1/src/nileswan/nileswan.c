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

#include <string.h>
#include <ws.h>
#include <ws/hardware.h>
#include "nileswan.h"

uint16_t nile_spi_timeout_ms;

bool nile_spi_wait_busy(void);

#if 0
bool nile_spi_tx(const void __far* buf, uint16_t size) {
	if (!nile_spi_wait_busy()) return false;
	uint16_t cnt = inportw(IO_NILE_SPI_CNT);
	uint16_t new_cnt = (size - 1) | NILE_SPI_MODE_WRITE | (cnt & 0x7800);

	volatile uint8_t prev_flash = inportb(IO_CART_FLASH);
	volatile uint16_t prev_bank = inportw(IO_BANK_2003_RAM);
	outportw(IO_BANK_2003_RAM, NILE_SEG_RAM_TX);
	outportb(IO_CART_FLASH, 0);
	memcpy(MK_FP(0x1000, 0x0000), buf, size);

	outportw(IO_NILE_SPI_CNT, (new_cnt ^ (NILE_SPI_BUFFER_IDX | NILE_SPI_START)));

	outportb(IO_CART_FLASH, prev_flash);
	outportw(IO_BANK_2003_RAM, prev_bank);
	
	return true;
}

bool nile_spi_rx(void __far* buf, uint16_t size, uint16_t mode) {
	if (!nile_spi_wait_busy()) return false;
	uint16_t cnt = inportw(IO_NILE_SPI_CNT);
	uint16_t new_cnt = (size - 1) | mode | (cnt & 0x7800);

	outportw(IO_NILE_SPI_CNT, new_cnt | NILE_SPI_START);
	if (!nile_spi_wait_busy()) return false;

#ifndef NILESWAN_IPL1
	volatile uint16_t prev_bank = inportw(IO_BANK_2003_ROM1);
#endif
	outportw(IO_NILE_SPI_CNT, new_cnt ^ NILE_SPI_BUFFER_IDX);
	outportw(IO_BANK_2003_ROM1, NILE_SEG_ROM_RX);
	memcpy(buf, MK_FP(0x3000, 0x0000), size);
#ifndef NILESWAN_IPL1
	outportw(IO_BANK_2003_ROM1, prev_bank);
#endif

	return true;
}
#endif
