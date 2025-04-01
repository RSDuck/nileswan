#include "id_print.h"
#include <nile.h>
#include "console.h"
#include "strings.h"

#define MCU_UID_BASE 0x1FFF6E50
#define MCU_UID_SIZE 12

bool op_id_print(void) {
    uint8_t buf[16];
    bool result = true;

    console_print_header(s_print_cartridge_ids);

	console_print(0, s_flash_jedec_id);
    if (console_print_status(nile_flash_wake())) {
        uint32_t flash_jedec_id = nile_flash_read_id();
        console_printf(0, s_format_1_u32, flash_jedec_id);
    } else {
        result = false;
    }
    console_print_newline();

    console_print(0, s_flash_uuid);
    if (console_print_status(nile_flash_read_uuid(buf))) {
        console_printf(0, s_format_4_bytes, buf[0], buf[1], buf[2], buf[3]);
        console_printf(0, s_format_4_bytes, buf[4], buf[5], buf[6], buf[7]);
    } else {
        result = false;
    }
    console_print_newline();

    console_print(0, s_restarting_mcu);

    if (console_print_status(nile_mcu_reset(true))) {
        console_print_newline();
        console_print(0, s_mcu_uuid);
        if (console_print_status(nile_mcu_boot_read_memory(MCU_UID_BASE, buf, MCU_UID_SIZE))) {
            console_printf(0, s_format_4_bytes, buf[11], buf[10], buf[9], buf[8]);
            console_printf(0, s_format_4_bytes, buf[7], buf[6], buf[5], buf[4]);
            console_printf(0, s_format_4_bytes, buf[3], buf[2], buf[1], buf[0]);
        } else {
            result = false;
        }
    } else {
        result = false;
    }

    console_print_newline();
    return result;
}
