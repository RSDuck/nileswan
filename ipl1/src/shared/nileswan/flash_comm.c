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

#include "flash_comm.h"

#include "nileswan.h"

#include <ws/hardware.h>

void flash_read(uint32_t addr, uint16_t size, uint8_t __far *out) {
    outportw(IO_NILE_SPI_CNT, NILE_SPI_CS_DEV_FLASH_SEL|NILE_SPI_25MHZ);

    uint8_t data[4] = {0x03, addr >> 16, (addr >> 8) & 0xFF, addr & 0xFF};
    outportb(IO_LCD_SEG, 0xF);
    nile_spi_tx(data, 4);
    outportb(IO_LCD_SEG, 0xE);
    nile_spi_rx_copy(out, size, NILE_SPI_MODE_READ);
    outportb(IO_LCD_SEG, 0xA);

    outportw(IO_NILE_SPI_CNT, NILE_SPI_CS_DEV_TF_DESEL);
}
