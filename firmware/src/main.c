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

#include "mcu.h"
#include "nvram.h"
#include "rtc.h"
#include "spi.h"
#include "tusb.h"
#include "cdc.h"

int main(void) {
    mcu_init();
    nvram_init();

    mcu_rtc_init();

    mcu_spi_set_freq(MCU_SPI_FREQ_384KHZ);
    mcu_spi_init(MCU_SPI_MODE_NATIVE);

    while (true) {
        mcu_usb_power_task();
        if (mcu_usb_is_powered()) {
#ifdef CONFIG_ENABLE_CDC_DEBUG_PORT
            tud_cdc_n_write_flush(1);
#endif
            tud_task();
        }
        mcu_spi_task();

        if (!mcu_usb_is_active()) {
            __WFE();
        }
    }
}
