#ifndef _STRINGS_H_
#define _STRINGS_H_

#define DEFINE_STRING_LOCAL(name, value) static const char __far name[] = value
#ifdef STRINGS_H_IMPLEMENTATION
#define DEFINE_STRING(name, value) const char __far name[] = value
#else
#define DEFINE_STRING(name, value) extern const char __far name[]
#endif

DEFINE_STRING(s_nileswan_recovery, "nileswan recovery");
DEFINE_STRING(s_setup_mcu_boot_flags, "Setup MCU boot flags");
DEFINE_STRING(s_restarting_mcu, "Restarting MCU...");
DEFINE_STRING(s_ok, "[OK]");
DEFINE_STRING(s_fail, "[FAIL]");
DEFINE_STRING(s_flash_optr, "FLASH_OPTR = ");
DEFINE_STRING(s_new_flash_optr, "New FLASH_OPTR = ");
DEFINE_STRING(s_format_1_u32, "%08lX");
DEFINE_STRING(s_format_4_bytes, "%02X%02X%02X%02X");
DEFINE_STRING(s_writing_changes, "Writing changes...");
DEFINE_STRING(s_nileswan_header,
    "     _ _\n"
    " _ _(_) |___ ____ __ ____ _ _ _\n"
    "| ' \\ | / -_|_-< V  V / _` | ' \\\n"
    "|_|_|_|_\\___/__/\\_/\\_/\\__,_|_|_|\n");
DEFINE_STRING(s_press_any_key_to_continue, "\nPress any button to continue...");
DEFINE_STRING(s_print_cartridge_ids, "Print cartridge IDs");
DEFINE_STRING(s_flash_jedec_id, "SPI flash JEDEC ID = ");
DEFINE_STRING(s_flash_uuid, "SPI flash UUID = ");
DEFINE_STRING(s_mcu_uuid, "MCU UUID = ");
DEFINE_STRING(s_tf_card_test, "Mount external storage");
DEFINE_STRING(s_tf_card_init, "Initializing storage...");
DEFINE_STRING(s_error_code, "Error code %d");

#endif /* _STRINGS_H_ */
