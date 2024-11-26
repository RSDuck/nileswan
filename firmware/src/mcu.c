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
#include <stm32l0xx_ll_bus.h>

#include "mcu.h"
#include "tusb.h"

typedef enum {
    USB_INIT_STATUS_OFF = 0,
    USB_INIT_STATUS_REQUEST_ON = 1,
    USB_INIT_STATUS_ON = 2,
    // TODO
    USB_INIT_STATUS_REQUEST_OFF = 3
} usb_init_status_t;

static uint8_t usb_init_status = USB_INIT_STATUS_OFF;

static void __mcu_usb_on_power_change(void) {
    if (mcu_usb_is_power_connected()) {
        if (usb_init_status == USB_INIT_STATUS_OFF) {
            usb_init_status = USB_INIT_STATUS_REQUEST_ON;
        } else if (usb_init_status == USB_INIT_STATUS_ON) {
            USB->BCDR |= USB_BCDR_DPPU;
        }
    } else {
        USB->BCDR &= ~USB_BCDR_DPPU;
    }
}

void EXTI4_15_IRQHandler(void) {
    if ((EXTI->PR & EXTI_PR_PIF8) != 0) {
        EXTI->PR = EXTI_PR_PIF8;
        __mcu_usb_on_power_change();
    }
}

void mcu_spi_set_freq(uint32_t freq) {
    LL_GPIO_SetPinSpeed(MCU_PORT_SPI, MCU_PIN_MASK_SPI_SCK, freq);
    LL_GPIO_SetPinSpeed(MCU_PORT_SPI, MCU_PIN_MASK_SPI_POCI, freq);
    LL_GPIO_SetPinSpeed(MCU_PORT_SPI, MCU_PIN_MASK_SPI_PICO, freq);
}

void mcu_init(void) {
    // Set system clock to 16 MHz

    LL_RCC_HSI_Enable();
    while (!LL_RCC_HSI_IsReady());

    LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_HSI);
    while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_HSI);

    LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
    LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_1);
    LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_1);
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG);

    LL_SetSystemCoreClock(16000000);

    LL_Init1msTick(16000000);
    LL_SYSTICK_SetClkSource(LL_SYSTICK_CLKSOURCE_HCLK);
    LL_SYSTICK_EnableIT();

    LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOA | LL_IOP_GRP1_PERIPH_GPIOB);

    // Initialize SPI
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SPI1);

    for (uint32_t pin = LL_GPIO_PIN_4; pin <= LL_GPIO_PIN_7; pin <<= 1) {
        LL_GPIO_SetAFPin_0_7(MCU_PORT_SPI, pin, LL_GPIO_AF_0);
        LL_GPIO_SetPinOutputType(MCU_PORT_SPI, pin, LL_GPIO_OUTPUT_PUSHPULL);
        LL_GPIO_SetPinPull(MCU_PORT_SPI, pin, LL_GPIO_PULL_NO);
        LL_GPIO_SetPinMode(MCU_PORT_SPI, pin, LL_GPIO_MODE_ALTERNATE);
    }
    LL_GPIO_SetPinSpeed(MCU_PORT_SPI, MCU_PIN_MASK_SPI_NSS, LL_GPIO_SPEED_FREQ_LOW);
    mcu_spi_set_freq(MCU_SPI_FREQ_384KHZ);

    LL_SPI_Disable(MCU_PERIPH_SPI);
    LL_SPI_SetMode(MCU_PERIPH_SPI, LL_SPI_MODE_SLAVE);
    LL_SPI_SetTransferDirection(MCU_PERIPH_SPI, LL_SPI_FULL_DUPLEX);
    LL_SPI_SetClockPolarity(MCU_PERIPH_SPI, LL_SPI_POLARITY_LOW);
    LL_SPI_SetClockPhase(MCU_PERIPH_SPI, LL_SPI_PHASE_1EDGE);
    LL_SPI_SetBaudRatePrescaler(MCU_PERIPH_SPI, LL_SPI_BAUDRATEPRESCALER_DIV2);
    LL_SPI_SetDataWidth(MCU_PERIPH_SPI, LL_SPI_DATAWIDTH_8BIT);
    LL_SPI_SetNSSMode(MCU_PERIPH_SPI, LL_SPI_NSS_HARD_INPUT);

    // Initialize FPGA IRQ pin
    LL_GPIO_SetPinOutputType(GPIOB, MCU_PIN_MASK_FPGA_IRQ, LL_GPIO_OUTPUT_PUSHPULL);
    LL_GPIO_SetPinPull(GPIOB, MCU_PIN_MASK_FPGA_IRQ, LL_GPIO_PULL_NO);
    LL_GPIO_SetPinSpeed(GPIOB, MCU_PIN_MASK_FPGA_IRQ, LL_GPIO_SPEED_FREQ_LOW); // 400 kHz
    LL_GPIO_SetOutputPin(GPIOB, MCU_PIN_MASK_FPGA_IRQ);
    LL_GPIO_SetPinMode(GPIOB, MCU_PIN_MASK_FPGA_IRQ, LL_GPIO_MODE_OUTPUT);

    // Initialize VBUS sensing
    LL_GPIO_SetPinPull(GPIOB, MCU_PIN_MASK_USB_POWER, LL_GPIO_PULL_DOWN);
    LL_GPIO_SetPinSpeed(GPIOB, MCU_PIN_MASK_USB_POWER, LL_GPIO_SPEED_FREQ_LOW);
    LL_GPIO_SetPinMode(GPIOB, MCU_PIN_MASK_USB_POWER, LL_GPIO_MODE_INPUT);
    LL_SYSCFG_SetEXTISource(LL_SYSCFG_EXTI_PORTB, LL_SYSCFG_EXTI_LINE8);
    LL_EXTI_EnableRisingTrig_0_31(LL_EXTI_LINE_8);
    LL_EXTI_EnableFallingTrig_0_31(LL_EXTI_LINE_8);
    LL_EXTI_EnableIT_0_31(LL_EXTI_LINE_8);
    NVIC_SetPriority(EXTI4_15_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 0, 0));
    NVIC_EnableIRQ(EXTI4_15_IRQn);
}

bool mcu_usb_is_active(void) {
    return usb_init_status == USB_INIT_STATUS_ON;
}

void SysTick_Handler(void) {
}

void USB_IRQHandler(void) {
    if (usb_init_status >= USB_INIT_STATUS_ON) {
        tud_int_handler(0);
    }
}

static void __mcu_usb_power_on(void) {
    // Enable 48 MHz internal oscillator
    LL_SYSCFG_VREFINT_EnableHSI48();
    LL_RCC_HSI48_Enable();
    while (!LL_RCC_HSI48_IsReady());
    LL_RCC_SetUSBClockSource(LL_RCC_USB_CLKSOURCE_HSI48);

    // Enable clock recovery system
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_CRS);
    LL_CRS_SetSyncDivider(LL_CRS_SYNC_DIV_1);
    LL_CRS_SetSyncPolarity(LL_CRS_SYNC_POLARITY_RISING);
    LL_CRS_SetSyncSignalSource(LL_CRS_SYNC_SOURCE_USB);
    LL_CRS_SetReloadCounter(__LL_CRS_CALC_CALCULATE_RELOADVALUE(48000000,1000));
    LL_CRS_SetFreqErrorLimit(34);
    LL_CRS_SetHSI48SmoothTrimming(64);
    
    // Enable USB clock
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_USB);
}

static void __mcu_usb_power_off(void) {
    LL_APB1_GRP1_DisableClock(LL_APB1_GRP1_PERIPH_USB);
    LL_APB1_GRP1_DisableClock(LL_APB1_GRP1_PERIPH_CRS);
    LL_RCC_SetUSBClockSource(LL_RCC_USB_CLKSOURCE_PLL);
    LL_RCC_HSI48_Disable();
    LL_SYSCFG_VREFINT_DisableHSI48();
}

void mcu_usb_power_task(void) {
    if (usb_init_status == USB_INIT_STATUS_REQUEST_ON) {
        tusb_rhport_init_t dev_init = {
            .role = TUSB_ROLE_DEVICE,
            .speed = TUSB_SPEED_AUTO
        };

        __mcu_usb_power_on();
        tusb_init(0, &dev_init);
        usb_init_status = USB_INIT_STATUS_ON;

        __mcu_usb_on_power_change();
    } else if (usb_init_status == USB_INIT_STATUS_ON) {
        tud_task();
    }
}
