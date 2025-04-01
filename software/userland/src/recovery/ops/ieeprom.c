#include "ieeprom.h"
#include <nile.h>
#include <ws/system.h>
#include "../console.h"
#include "../main.h"
#include "../strings.h"

#define WORD_0X82_CUSTOM_SPLASH (IEEP_C_OPTIONS1_CUSTOM_SPLASH << 8)
static const uint8_t __far swancrystal_factory_tft_data[] = {
    0xD0, 0x77, 0xF7, 0x06, 0xE2, 0x0A, 0xEA, 0xEE
};

bool op_ieeprom_disable_custom_splash(void) {
    bool result = false;

    console_print_header(s_disable_custom_splash);

    if (!ws_system_color_active()) {
        console_print(0, s_model_unsupported);
        return false;
    }

    if (!console_warranty_disclaimer()) {
        return false;
    }

    uint16_t word_0x82 = ws_eeprom_read_word(ws_eeprom_handle_internal(), 0x82);
    if (!(word_0x82 & WORD_0X82_CUSTOM_SPLASH)) {
        console_print(0, s_custom_splash_already_disabled);
    } else {
        console_print(0, s_ieeprom_writing);
        word_0x82 &= ~WORD_0X82_CUSTOM_SPLASH;
        result = console_print_status(ws_eeprom_write_word(ws_eeprom_handle_internal(), 0x82, word_0x82));
    }
    console_print_newline();

    return result;
}

bool op_ieeprom_restore_tft_data(void) {
    bool result = true;

    console_print_header(s_restore_tft_data);

    if (!ws_system_color_active()) {
        console_print(0, s_model_unsupported);
        return false;
    }

    if (!console_warranty_disclaimer()) {
        return false;
    }

    console_print(0, s_ieeprom_writing);
    ws_eeprom_handle_t ieep_handle = ws_eeprom_handle_internal();

    for (uint8_t i = 0; i < 8; i += 2) {
        uint16_t w = *((uint16_t __far*) (swancrystal_factory_tft_data + i));
        uint16_t w2 = ws_eeprom_read_word(ieep_handle, 0xAE + i);
        if (w != w2) {
            if (ws_eeprom_write_word(ieep_handle, 0xAE + i, w)) {
                console_putc(0, 'o');
            } else {
                console_putc(0, '!');
                console_print_status(false);
                result = false;
                break;
            }
        } else {
            console_putc(0, '.');
        }
    }

    if (result) {
        console_print_status(true);
    }
    console_print_newline();

    return result;
}
