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
#include <stm32u0xx_ll_rcc.h>

#include "mcu.h"
#include "tusb.h"

#include "class/cdc/cdc_device.h"
#include "spi.h"
#include "cdc.h"
#include "config.h"
#include "eeprom.h"
#include "rtc.h"
#include "spi_cmd.h"

static uint8_t spi_mode = MCU_SPI_MODE_NATIVE;
static uint8_t spi_freq = MCU_SPI_FREQ_384KHZ;

__attribute__((section(".noinit")))
uint8_t spi_tx_buffer[MCU_SPI_TX_BUFFER_SIZE];
uint8_t spi_rx_buffer[MCU_SPI_RX_BUFFER_SIZE];
uint32_t spi_rx_buffer_circular = 0xFFFFFFFF;

static void mcu_spi_clear_rx_queue(void) {
    while (LL_SPI_GetRxFIFOLevel(MCU_PERIPH_SPI)) {
        LL_SPI_ReceiveData8(MCU_PERIPH_SPI);
    }
}

void mcu_spi_disable_dma_tx(void) {
    LL_DMA_DisableChannel(DMA1, MCU_DMA_CHANNEL_SPI_TX);
    LL_SPI_DisableDMAReq_TX(MCU_PERIPH_SPI);
}

void mcu_spi_disable_dma_rx(void) {
    LL_DMA_DisableChannel(DMA1, MCU_DMA_CHANNEL_SPI_RX);
    LL_SPI_DisableDMAReq_RX(MCU_PERIPH_SPI);
}

void mcu_spi_enable_dma_tx_empty(void) {
    LL_DMA_SetMode(DMA1, MCU_DMA_CHANNEL_SPI_TX, LL_DMA_MODE_CIRCULAR);
    LL_DMA_ConfigAddresses(DMA1, MCU_DMA_CHANNEL_SPI_TX,
        (uint32_t) &spi_rx_buffer_circular, LL_SPI_DMA_GetRegAddr(MCU_PERIPH_SPI),
        LL_DMA_DIRECTION_MEMORY_TO_PERIPH);
    LL_DMA_SetDataLength(DMA1, MCU_DMA_CHANNEL_SPI_TX, sizeof(spi_rx_buffer_circular));

    // Initialize SPI transfer
    LL_DMA_EnableChannel(DMA1, MCU_DMA_CHANNEL_SPI_TX);
    LL_SPI_EnableDMAReq_TX(MCU_PERIPH_SPI);
}

void mcu_spi_enable_dma_tx(const void *address, uint32_t length) {
    LL_DMA_SetMode(DMA1, MCU_DMA_CHANNEL_SPI_TX, LL_DMA_MODE_NORMAL);
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
        if (len < 0) return;

#ifdef CONFIG_DEBUG_SPI_NATIVE_CMD
        cdc_debug(", returning %d bytes\r\n", len);
#endif
        *((uint16_t*) spi_tx_buffer) = (len << 1);

        mcu_spi_disable_dma_tx();
        mcu_spi_enable_dma_tx(spi_tx_buffer, len + 2);
    } else if (spi_mode == MCU_SPI_MODE_RTC) {
        mcu_fpga_start_busy();
        int len = rtc_finish_command_rx(spi_rx_buffer, spi_tx_buffer);
        if (len) {
            mcu_spi_enable_dma_tx(spi_tx_buffer, len);
        } else {
            LL_SPI_EnableIT_RXNE(MCU_PERIPH_SPI);
        }
        mcu_fpga_finish_busy();
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
        if (spi_mode == MCU_SPI_MODE_NATIVE) {
            LL_SPI_SetRxFIFOThreshold(MCU_PERIPH_SPI, LL_SPI_RX_FIFO_TH_HALF);
        }

        LL_SPI_EnableIT_RXNE(MCU_PERIPH_SPI);
    }

    if (LL_DMA_IsActiveFlag_TC2(DMA1)) {
        LL_DMA_ClearFlag_TC2(DMA1);

        mcu_spi_disable_dma_rx();
        
        if (spi_mode == MCU_SPI_MODE_NATIVE) {
            spi_native_idx = 2;
        } else {
            mcu_spi_dma_finish();
        }
    }
}

void SPI1_IRQHandler(void) {
    if (LL_SPI_IsActiveFlag_RXNE(SPI1)) {
        if (spi_mode == MCU_SPI_MODE_NATIVE) {
            uint8_t byte = LL_SPI_ReceiveData8(MCU_PERIPH_SPI);
            if (byte == 0xFF) {
                return;
            }
            uint16_t cmd = byte | (LL_SPI_ReceiveData8(MCU_PERIPH_SPI) << 8);
            mcu_spi_enable_dma_tx_empty();
            LL_SPI_DisableIT_RXNE(MCU_PERIPH_SPI);
            LL_SPI_SetRxFIFOThreshold(MCU_PERIPH_SPI, LL_SPI_RX_FIFO_TH_QUARTER);
            int rx_length = spi_native_start_command_rx(cmd);
            if (rx_length) {
                mcu_spi_enable_dma_rx(spi_rx_buffer, rx_length);
#ifdef CONFIG_DEBUG_SPI_NATIVE_CMD
                cdc_debug("spi/native: starting command %04X (%d bytes)\r\n", cmd, rx_length);
#endif
            } else {
                spi_native_idx = 2;
            }
        } else if (spi_mode == MCU_SPI_MODE_EEPROM) {
            uint16_t data = LL_SPI_ReceiveData16(MCU_PERIPH_SPI);
#ifdef CONFIG_DEBUG_SPI_EEPROM_CMD
            cdc_debug_write_hex16(data);
#endif
#ifdef CONFIG_FULL_EEPROM_EMULATION
            LL_SPI_TransmitData16(MCU_PERIPH_SPI, eeprom_exch_word(data));
#else
            eeprom_exch_word(data);
#endif
        } else if (spi_mode == MCU_SPI_MODE_RTC) {
            LL_SPI_DisableIT_RXNE(MCU_PERIPH_SPI);
            mcu_fpga_start_busy();
            uint8_t cmd = LL_SPI_ReceiveData8(MCU_PERIPH_SPI);
#ifdef CONFIG_DEBUG_SPI_DISABLE_PROCESSING
#ifdef CONFIG_DEBUG_SPI_RTC_CMD
            cdc_debug_write_hex16(cmd);
#endif
#else
            int rx_length = rtc_start_command_rx(cmd);
            if (rx_length) {
                mcu_spi_enable_dma_rx(spi_rx_buffer, rx_length);
            } else {
                mcu_spi_dma_finish();
            }
#endif
            mcu_fpga_finish_busy();
        } else if (spi_mode == MCU_SPI_MODE_CDC_OUTPUT) {
            uint8_t ch = LL_SPI_ReceiveData8(MCU_PERIPH_SPI);
            if (tud_cdc_connected()) {
                if (ch == '\n') tud_cdc_write_char('\r');
                tud_cdc_write_char(ch);
                tud_cdc_write_flush();
            }
        }
    }
}

uint32_t mcu_spi_get_freq(void) {
    return spi_freq;
}

void mcu_spi_set_freq(uint32_t freq) {
    uint32_t pin_speed = LL_GPIO_SPEED_FREQ_LOW;
    if (pin_speed >= MCU_SPI_FREQ_24MHZ) pin_speed = LL_GPIO_SPEED_FREQ_HIGH;
    else if (pin_speed >= MCU_SPI_FREQ_6MHZ) pin_speed = LL_GPIO_SPEED_FREQ_MEDIUM;

    spi_freq = freq;
    LL_GPIO_SetPinSpeed(MCU_PORT_SPI, MCU_PIN_SPI_SCK, pin_speed);
    LL_GPIO_SetPinSpeed(MCU_PORT_SPI, MCU_PIN_SPI_POCI, pin_speed);
    LL_GPIO_SetPinSpeed(MCU_PORT_SPI, MCU_PIN_SPI_PICO, pin_speed);

    mcu_update_clock_speed();
}

mcu_spi_mode_t mcu_spi_get_mode(void) {
    return spi_mode;
}

void mcu_spi_enable(void) {
    LL_SPI_Enable(MCU_PERIPH_SPI);
}

void mcu_spi_disable(void) {
    LL_mDelay(1);
    LL_SPI_Disable(MCU_PERIPH_SPI);
    mcu_spi_clear_rx_queue();
}

void mcu_spi_init(mcu_spi_mode_t mode) {
    mcu_spi_disable_dma_tx();
    mcu_spi_disable_dma_rx();

    mcu_spi_disable();

    // Force SPI reset to clear TX queue
    LL_APB1_GRP2_ForceReset(LL_APB1_GRP2_PERIPH_SPI1);
    LL_APB1_GRP2_ReleaseReset(LL_APB1_GRP2_PERIPH_SPI1);

    // Initialize SPI
    MCU_PERIPH_SPI->CR1 = LL_SPI_MODE_SLAVE | LL_SPI_MSB_FIRST | LL_SPI_FULL_DUPLEX
        | LL_SPI_POLARITY_LOW | LL_SPI_PHASE_1EDGE;
    LL_SPI_SetBaudRatePrescaler(MCU_PERIPH_SPI, LL_SPI_BAUDRATEPRESCALER_DIV2);
    LL_SPI_SetNSSMode(MCU_PERIPH_SPI, LL_SPI_NSS_HARD_INPUT);

    bool dma_enabled = spi_mode != MCU_SPI_MODE_EEPROM;

    if (dma_enabled) {
        // Initialize SPI DMA
        LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA1);

#ifdef TARGET_U0
        LL_DMA_SetPeriphRequest(DMA1, MCU_DMA_CHANNEL_SPI_TX, LL_DMAMUX_REQ_SPI1_TX);
        LL_DMA_SetPeriphRequest(DMA1, MCU_DMA_CHANNEL_SPI_RX, LL_DMAMUX_REQ_SPI1_RX);
#else
        LL_DMA_SetPeriphRequest(DMA1, MCU_DMA_CHANNEL_SPI_TX, LL_DMA_REQUEST_1);
        LL_DMA_SetPeriphRequest(DMA1, MCU_DMA_CHANNEL_SPI_RX, LL_DMA_REQUEST_1);
#endif

        LL_DMA_SetDataTransferDirection(DMA1, MCU_DMA_CHANNEL_SPI_TX, LL_DMA_DIRECTION_MEMORY_TO_PERIPH);
        LL_DMA_SetChannelPriorityLevel(DMA1, MCU_DMA_CHANNEL_SPI_TX, LL_DMA_PRIORITY_LOW);
        LL_DMA_SetPeriphIncMode(DMA1, MCU_DMA_CHANNEL_SPI_TX, LL_DMA_PERIPH_NOINCREMENT);
        LL_DMA_SetMemoryIncMode(DMA1, MCU_DMA_CHANNEL_SPI_TX, LL_DMA_MEMORY_INCREMENT);
        LL_DMA_SetPeriphSize(DMA1, MCU_DMA_CHANNEL_SPI_TX, LL_DMA_PDATAALIGN_BYTE);
        LL_DMA_SetMemorySize(DMA1, MCU_DMA_CHANNEL_SPI_TX, LL_DMA_MDATAALIGN_BYTE);

        LL_DMA_SetDataTransferDirection(DMA1, MCU_DMA_CHANNEL_SPI_RX, LL_DMA_DIRECTION_PERIPH_TO_MEMORY);
        LL_DMA_SetChannelPriorityLevel(DMA1, MCU_DMA_CHANNEL_SPI_RX, LL_DMA_PRIORITY_LOW);
        LL_DMA_SetMode(DMA1, MCU_DMA_CHANNEL_SPI_RX, LL_DMA_MODE_NORMAL);
        LL_DMA_SetPeriphIncMode(DMA1, MCU_DMA_CHANNEL_SPI_RX, LL_DMA_PERIPH_NOINCREMENT);
        LL_DMA_SetMemoryIncMode(DMA1, MCU_DMA_CHANNEL_SPI_RX, LL_DMA_MEMORY_INCREMENT);
        LL_DMA_SetPeriphSize(DMA1, MCU_DMA_CHANNEL_SPI_RX, LL_DMA_PDATAALIGN_BYTE);
        LL_DMA_SetMemorySize(DMA1, MCU_DMA_CHANNEL_SPI_RX, LL_DMA_MDATAALIGN_BYTE);
    } else {
        LL_AHB1_GRP1_DisableClock(LL_AHB1_GRP1_PERIPH_DMA1);
    }

    spi_mode = mode;
    if (spi_mode == MCU_SPI_MODE_RTC || spi_mode == MCU_SPI_MODE_CDC_OUTPUT) {
        LL_SPI_SetRxFIFOThreshold(MCU_PERIPH_SPI, LL_SPI_RX_FIFO_TH_QUARTER);
        LL_SPI_SetDataWidth(MCU_PERIPH_SPI, LL_SPI_DATAWIDTH_8BIT);
    } else if (spi_mode == MCU_SPI_MODE_EEPROM) {
        LL_SPI_SetRxFIFOThreshold(MCU_PERIPH_SPI, LL_SPI_RX_FIFO_TH_HALF);
        LL_SPI_SetDataWidth(MCU_PERIPH_SPI, LL_SPI_DATAWIDTH_16BIT);
#ifdef CONFIG_FULL_EEPROM_EMULATION
        LL_SPI_TransmitData16(MCU_PERIPH_SPI, 0xFFFF);
#else
        LL_SPI_SetTransferDirection(MCU_PERIPH_SPI, LL_SPI_SIMPLEX_RX);
#endif
    } else {
        LL_SPI_SetRxFIFOThreshold(MCU_PERIPH_SPI, LL_SPI_RX_FIFO_TH_HALF);
        LL_SPI_SetDataWidth(MCU_PERIPH_SPI, LL_SPI_DATAWIDTH_8BIT);
    }
    LL_SPI_EnableIT_RXNE(MCU_PERIPH_SPI);
    LL_DMA_ClearFlag_TC2(DMA1);
    LL_DMA_ClearFlag_TC3(DMA1);
    if (dma_enabled) {
        LL_DMA_EnableIT_TC(DMA1, MCU_DMA_CHANNEL_SPI_RX);
        LL_DMA_EnableIT_TC(DMA1, MCU_DMA_CHANNEL_SPI_TX);
    }

    NVIC_SetPriority(SPI1_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 100, 0));
    NVIC_EnableIRQ(SPI1_IRQn);
    if (dma_enabled) {
        NVIC_SetPriority(DMA1_Channel2_3_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 90, 0));
        NVIC_EnableIRQ(DMA1_Channel2_3_IRQn);
    } else {
        NVIC_DisableIRQ(DMA1_Channel2_3_IRQn);
    }

    mcu_update_clock_speed();
    
    // Enable SPI
    mcu_spi_enable();
}
