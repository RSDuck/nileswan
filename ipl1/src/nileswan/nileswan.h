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

#ifndef __NILESWAN_H__
#define __NILESWAN_H__

#include <wonderful.h>

#define NILE_SPI_MODE_WRITE      0x0000
#define NILE_SPI_MODE_READ       0x0200
#define NILE_SPI_MODE_EXCH       0x0400
#define NILE_SPI_MODE_WAIT_READ  0x0600
#define NILE_SPI_MODE_MASK       0x0600
#define NILE_SPI_390KHZ          0x0800
#define NILE_SPI_25MHZ           0x0000
#define NILE_SPI_CS_HIGH         0x0000
#define NILE_SPI_CS_LOW          0x1000
#define NILE_SPI_CS              0x1000
#define NILE_SPI_DEV_FLASH       0x0000
#define NILE_SPI_DEV_TF          0x2000
#define NILE_SPI_BUFFER_IDX      0x4000
#define NILE_SPI_START           0x8000
#define NILE_SPI_BUSY            0x8000
#define IO_NILE_SPI_CNT    0xE0

#define NILE_POW_CLOCK     0x01
#define NILE_POW_TF        0x02
#define IO_NILE_POW_CNT    0xE2

#define NILE_IRQ_ENABLE    0x01
#define NILE_IRQ_SPI       0x02
#define IO_NILE_IRQ        0xE3

#define IO_NILE_SEG_MASK   0xE4

#define NILE_SEG_RAM_TX    15
#define NILE_SEG_ROM_RX    510
#define NILE_SEG_ROM_BOOT  511

#ifndef __ASSEMBLER__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static inline void nile_spi_wait_busy(void) {
	while(inportb(IO_NILE_SPI_CNT + 1) & (NILE_SPI_BUSY >> 8));
}

void nile_spi_tx(const void __far* buf, uint16_t size);
void nile_spi_rx(void __far* buf, uint16_t size, uint16_t mode);

#endif

#endif /* __NILESWAN_H__ */