#include "mcu_setup.h"
#include <ws.h>
#include <nile.h>
#include "../console.h"
#include "../strings.h"

#define MCU_FLASH_OPTR_ADDR 0x40022020U

bool op_mcu_setup_boot_flags(void) {
    bool result = false;
    uint16_t prev_spi_cnt = inportw(IO_NILE_SPI_CNT);
    outportw(IO_NILE_SPI_CNT, NILE_SPI_CLOCK_CART | NILE_SPI_DEV_MCU);

    console_print_header(s_setup_mcu_boot_flags);
    console_clear();
    console_print(0, s_restarting_mcu);

    if (console_print_status(nile_mcu_reset(true))) {
        uint8_t flash_optr[4];

        console_print(0, s_flash_optr);
        if (console_print_status(nile_mcu_boot_read_memory(MCU_FLASH_OPTR_ADDR, flash_optr, sizeof(flash_optr)))) {
            console_printf(0, s_format_4_bytes, flash_optr[3], flash_optr[2], flash_optr[1], flash_optr[0]);

            flash_optr[3] &= ~1; // Unset NBOOT_SEL

            console_print(0, s_new_flash_optr);
            console_printf(0, s_format_4_bytes, flash_optr[3], flash_optr[2], flash_optr[1], flash_optr[0]);

            console_print(0, s_writing_changes);
            if (console_print_status(nile_mcu_boot_write_memory(MCU_FLASH_OPTR_ADDR, flash_optr, sizeof(flash_optr)))) {
                result = true;
            }
        }
    }

    outportw(IO_NILE_SPI_CNT, prev_spi_cnt);
    return result;
}
