/**
 * Copyright (c) 2024 Adrian Siekierka
 *
 * Nileswan MCU is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * Nileswan MCU is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with Nileswan MCU. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef _SPI_H_
#define _SPI_H_

#include "config.h"
#include "mcu.h"

extern uint8_t spi_tx_buffer[MCU_SPI_TX_BUFFER_SIZE];
extern uint8_t spi_rx_buffer[MCU_SPI_RX_BUFFER_SIZE];

typedef enum {
    // Native (libnile) MCU commands
    MCU_SPI_MODE_NATIVE = 0,
    // EEPROM emulation<->SPI passthrough
    MCU_SPI_MODE_EEPROM = 1,
    // RTC emulation<->SPI passthrough
    MCU_SPI_MODE_RTC = 2,
    // USB CDC passthrough mode
    MCU_SPI_MODE_CDC_OUTPUT = 3
} mcu_spi_mode_t;

void mcu_spi_init(mcu_spi_mode_t mode);
mcu_spi_mode_t mcu_spi_get_mode(void);
uint32_t mcu_spi_get_freq(void);
void mcu_spi_set_freq(uint32_t freq);
void mcu_spi_disable_dma_tx(void);
void mcu_spi_disable_dma_rx(void);
void mcu_spi_enable_dma_tx(const void *address, uint32_t length);
void mcu_spi_enable_dma_rx(void *address, uint32_t length);
void mcu_spi_task(void);

void mcu_spi_enable(void);
void mcu_spi_disable(void);

#endif /* _SPI_H_ */
