#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/spi.h>

int main(void) {
    rcc_periph_clock_enable(RCC_SPI1);
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_GPIOB);
    rcc_periph_clock_enable(RCC_GPIOC);

    rcc_periph_reset_pulse(RST_SPI1);

    spi_set_slave_mode(SPI1);
    spi_set_standard_mode(SPI1, 0);

    spi_enable(SPI1);

    uint8_t prev_value = 0xAB;
    while (1) {
        prev_value = spi_xfer(SPI1, prev_value);
    }
}