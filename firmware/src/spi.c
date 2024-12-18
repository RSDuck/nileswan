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

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stm32l052xx.h>
#include <stm32l0xx_ll_spi.h>

#include "tusb.h"

#include "spi.h"
#include "cdc.h"
#include "config.h"
#include "eeprom.h"
#include "rtc.h"
#include "spi_cmd.h"

static uint8_t spi_mode;
uint8_t spi_tx_buffer[MCU_SPI_TX_BUFFER_SIZE];
uint8_t spi_rx_buffer[MCU_SPI_RX_BUFFER_SIZE];

void mcu_spi_disable_dma_tx(void) {
    LL_DMA_DisableChannel(DMA1, MCU_DMA_CHANNEL_SPI_TX);
    LL_SPI_DisableDMAReq_TX(MCU_PERIPH_SPI);
}

void mcu_spi_disable_dma_rx(void) {
    LL_DMA_DisableChannel(DMA1, MCU_DMA_CHANNEL_SPI_RX);
    LL_SPI_DisableDMAReq_RX(MCU_PERIPH_SPI);
}

void mcu_spi_enable_dma_tx(const void *address, uint32_t length) {
    LL_SPI_DisableIT_TXE(MCU_PERIPH_SPI);
    while (!LL_SPI_IsActiveFlag_TXE(MCU_PERIPH_SPI));

    LL_DMA_ConfigAddresses(DMA1, MCU_DMA_CHANNEL_SPI_TX,
        (uint32_t) address, LL_SPI_DMA_GetRegAddr(MCU_PERIPH_SPI),
        LL_DMA_DIRECTION_MEMORY_TO_PERIPH);
    LL_DMA_SetDataLength(DMA1, MCU_DMA_CHANNEL_SPI_TX, length);

    // Initialize SPI transfer
    LL_DMA_EnableChannel(DMA1, MCU_DMA_CHANNEL_SPI_TX);
    LL_SPI_EnableDMAReq_TX(MCU_PERIPH_SPI);
}

void mcu_spi_enable_dma_rx(void *address, uint32_t length) {
    LL_DMA_ConfigAddresses(DMA1, MCU_DMA_CHANNEL_SPI_RX,
        LL_SPI_DMA_GetRegAddr(MCU_PERIPH_SPI), (uint32_t) address,
        LL_DMA_DIRECTION_PERIPH_TO_MEMORY);
    LL_DMA_SetDataLength(DMA1, MCU_DMA_CHANNEL_SPI_RX, length);

    LL_DMA_EnableChannel(DMA1, MCU_DMA_CHANNEL_SPI_RX);
    LL_SPI_EnableDMAReq_RX(MCU_PERIPH_SPI);
}

static void mcu_spi_dma_finish(void) {
    if (spi_mode == MCU_SPI_MODE_NATIVE) {
        int len = spi_native_finish_command_rx(spi_rx_buffer, spi_tx_buffer + 2);
        // cdc_debug(", returning %d bytes\r\n", len);
        spi_tx_buffer[0] = (len << 1) & 0xFF;
        spi_tx_buffer[1] = (len << 1) >> 8;

        mcu_spi_enable_dma_tx(spi_tx_buffer, len + 2);
    } else if (spi_mode == MCU_SPI_MODE_RTC) {
        LL_GPIO_ResetOutputPin(GPIOB, MCU_PIN_FPGA_READY);
        int len = rtc_finish_command_rx(spi_rx_buffer, spi_tx_buffer);
        if (len) {
            mcu_spi_enable_dma_tx(spi_tx_buffer, len);
        } else {
            LL_SPI_EnableIT_RXNE(MCU_PERIPH_SPI);
        }
        LL_GPIO_SetOutputPin(GPIOB, MCU_PIN_FPGA_READY);
    }
}

static uint8_t spi_native_idx = 0;

void mcu_spi_task(void) {
    if (spi_native_idx == 2) {
        spi_native_idx = 0;
        mcu_spi_dma_finish();
    }
}

void DMA1_Channel2_3_IRQHandler(void) {
    if (LL_DMA_IsActiveFlag_TC3(DMA1)) {
        LL_DMA_ClearFlag_TC3(DMA1);

        mcu_spi_disable_dma_tx();

        LL_SPI_EnableIT_RXNE(MCU_PERIPH_SPI);
    }

    if (LL_DMA_IsActiveFlag_TC2(DMA1)) {
        LL_DMA_ClearFlag_TC2(DMA1);

        mcu_spi_disable_dma_rx();
        LL_SPI_EnableIT_TXE(MCU_PERIPH_SPI);
        spi_native_idx = 2;
    }
}


void SPI1_IRQHandler(void) {
    if (LL_SPI_IsActiveFlag_TXE(SPI1)) {
        LL_SPI_TransmitData8(SPI1, 0xFF);
    }

    if (LL_SPI_IsActiveFlag_RXNE(SPI1)) {
        if (spi_mode == MCU_SPI_MODE_NATIVE) {
            uint8_t byte = LL_SPI_ReceiveData8(MCU_PERIPH_SPI);
            if (byte == 0xFF) {
                return;
            }
            int timeout = 2500;
            while (timeout--) {
                if (LL_SPI_IsActiveFlag_RXNE(SPI1)) {
                    break;
                }
            }
            if (!timeout) {
                return;
            }
            uint8_t byte2 = LL_SPI_ReceiveData8(MCU_PERIPH_SPI);
            uint16_t cmd = (byte) | (byte2 << 8);
            LL_SPI_DisableIT_RXNE(MCU_PERIPH_SPI);
            int rx_length = spi_native_start_command_rx(cmd);
            if (rx_length) {
                mcu_spi_enable_dma_rx(spi_rx_buffer, rx_length);
            } else {
                LL_SPI_EnableIT_TXE(MCU_PERIPH_SPI);
                spi_native_idx = 2;
            }
        } else if (spi_mode == MCU_SPI_MODE_EEPROM) {
            LL_GPIO_ResetOutputPin(GPIOB, MCU_PIN_FPGA_READY);
            LL_SPI_TransmitData16(MCU_PERIPH_SPI, eeprom_exch_word(LL_SPI_ReceiveData16(MCU_PERIPH_SPI)));
            LL_GPIO_SetOutputPin(GPIOB, MCU_PIN_FPGA_READY);
        } else if (spi_mode == MCU_SPI_MODE_RTC) {
            LL_SPI_DisableIT_RXNE(MCU_PERIPH_SPI);
            LL_GPIO_ResetOutputPin(GPIOB, MCU_PIN_FPGA_READY);
            uint8_t cmd = LL_SPI_ReceiveData8(MCU_PERIPH_SPI);
            int rx_length = rtc_start_command_rx(cmd);
            if (rx_length) {
                mcu_spi_enable_dma_rx(spi_rx_buffer, rx_length);
            } else {
                mcu_spi_dma_finish();
            }
            LL_GPIO_SetOutputPin(GPIOB, MCU_PIN_FPGA_READY);
        }
    }
}

void mcu_spi_set_freq(uint32_t freq) {
    LL_GPIO_SetPinSpeed(MCU_PORT_SPI, MCU_PIN_SPI_SCK, freq);
    LL_GPIO_SetPinSpeed(MCU_PORT_SPI, MCU_PIN_SPI_POCI, freq);
    LL_GPIO_SetPinSpeed(MCU_PORT_SPI, MCU_PIN_SPI_PICO, freq);
}

void mcu_spi_init(mcu_spi_mode_t mode) {
    LL_DMA_DisableChannel(DMA1, MCU_DMA_CHANNEL_SPI_TX);
    LL_DMA_DisableChannel(DMA1, MCU_DMA_CHANNEL_SPI_RX);

    mcu_spi_disable();

    // Initialize SPI
    LL_SPI_SetMode(MCU_PERIPH_SPI, LL_SPI_MODE_SLAVE);
    LL_SPI_SetTransferDirection(MCU_PERIPH_SPI, LL_SPI_FULL_DUPLEX);
    LL_SPI_SetClockPolarity(MCU_PERIPH_SPI, LL_SPI_POLARITY_LOW);
    LL_SPI_SetClockPhase(MCU_PERIPH_SPI, LL_SPI_PHASE_1EDGE);
    LL_SPI_SetBaudRatePrescaler(MCU_PERIPH_SPI, LL_SPI_BAUDRATEPRESCALER_DIV2);
    LL_SPI_SetNSSMode(MCU_PERIPH_SPI, LL_SPI_NSS_HARD_INPUT);

    // Initialize SPI DMA
    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA1);

    LL_DMA_SetPeriphRequest(DMA1, MCU_DMA_CHANNEL_SPI_TX, LL_DMA_REQUEST_1);
    LL_DMA_SetDataTransferDirection(DMA1, MCU_DMA_CHANNEL_SPI_TX, LL_DMA_DIRECTION_MEMORY_TO_PERIPH);
    LL_DMA_SetChannelPriorityLevel(DMA1, MCU_DMA_CHANNEL_SPI_TX, LL_DMA_PRIORITY_LOW);
    LL_DMA_SetMode(DMA1, MCU_DMA_CHANNEL_SPI_TX, LL_DMA_MODE_NORMAL);
    LL_DMA_SetPeriphIncMode(DMA1, MCU_DMA_CHANNEL_SPI_TX, LL_DMA_PERIPH_NOINCREMENT);
    LL_DMA_SetMemoryIncMode(DMA1, MCU_DMA_CHANNEL_SPI_TX, LL_DMA_MEMORY_INCREMENT);
    LL_DMA_SetPeriphSize(DMA1, MCU_DMA_CHANNEL_SPI_TX, LL_DMA_PDATAALIGN_BYTE);
    LL_DMA_SetMemorySize(DMA1, MCU_DMA_CHANNEL_SPI_TX, LL_DMA_MDATAALIGN_BYTE);

    LL_DMA_SetPeriphRequest(DMA1, MCU_DMA_CHANNEL_SPI_RX, LL_DMA_REQUEST_1);
    LL_DMA_SetDataTransferDirection(DMA1, MCU_DMA_CHANNEL_SPI_RX, LL_DMA_DIRECTION_PERIPH_TO_MEMORY);
    LL_DMA_SetChannelPriorityLevel(DMA1, MCU_DMA_CHANNEL_SPI_RX, LL_DMA_PRIORITY_LOW);
    LL_DMA_SetMode(DMA1, MCU_DMA_CHANNEL_SPI_RX, LL_DMA_MODE_NORMAL);
    LL_DMA_SetPeriphIncMode(DMA1, MCU_DMA_CHANNEL_SPI_RX, LL_DMA_PERIPH_NOINCREMENT);
    LL_DMA_SetMemoryIncMode(DMA1, MCU_DMA_CHANNEL_SPI_RX, LL_DMA_MEMORY_INCREMENT);
    LL_DMA_SetPeriphSize(DMA1, MCU_DMA_CHANNEL_SPI_RX, LL_DMA_PDATAALIGN_BYTE);
    LL_DMA_SetMemorySize(DMA1, MCU_DMA_CHANNEL_SPI_RX, LL_DMA_MDATAALIGN_BYTE);

    spi_mode = mode;
    if (spi_mode == MCU_SPI_MODE_EEPROM) {
        LL_SPI_SetDataWidth(MCU_PERIPH_SPI, LL_SPI_DATAWIDTH_16BIT);
        LL_SPI_TransmitData16(MCU_PERIPH_SPI, 0xFFFF);
    } else {
        LL_SPI_SetDataWidth(MCU_PERIPH_SPI, LL_SPI_DATAWIDTH_8BIT);
    }
    LL_SPI_EnableIT_RXNE(MCU_PERIPH_SPI);
    LL_DMA_ClearFlag_TC2(DMA1);
    LL_DMA_ClearFlag_TC3(DMA1);
    LL_DMA_EnableIT_TC(DMA1, MCU_DMA_CHANNEL_SPI_RX);
    LL_DMA_EnableIT_TC(DMA1, MCU_DMA_CHANNEL_SPI_TX);

    NVIC_SetPriority(SPI1_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 0, 0));
    NVIC_EnableIRQ(SPI1_IRQn);
    NVIC_SetPriority(DMA1_Channel2_3_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 0, 0));
    NVIC_EnableIRQ(DMA1_Channel2_3_IRQn);

    mcu_spi_disable_dma_tx();

    // Enable SPI
    mcu_spi_enable();
}
