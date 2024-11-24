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

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/spi.h>

int main(void) {
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_GPIOB);
    rcc_periph_clock_enable(RCC_GPIOC);
    rcc_periph_clock_enable(RCC_SPI1);

    gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO4|GPIO5|GPIO6|GPIO7);
    gpio_set_af(GPIOA, GPIO_AF0, GPIO4|GPIO5|GPIO6|GPIO7);

    rcc_periph_reset_pulse(RST_SPI1);

    spi_set_slave_mode(SPI1);
    spi_set_standard_mode(SPI1, 0);

    spi_enable(SPI1);

    uint8_t prev_value = 0xAB;
    while (1) {
        spi_send(SPI1, prev_value);
        prev_value++;
    }
}
