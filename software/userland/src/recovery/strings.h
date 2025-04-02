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

DEFINE_STRING(s_manual_shutdown, "The console may now be powered off.");

DEFINE_STRING(s_model_unsupported, "This action is not supported on this console.\n");
DEFINE_STRING(s_warranty_disclaimer, "This action modifies sensitive data.\nYou do so at your own risk.\n\nBy choosing to continue, you agree that the developers of this software will not be held responsible for any damage or loss resulting from the use of this action.\n\nDo you want to continue? A - Yes, other - No");

DEFINE_STRING(s_internal_eeprom_recovery, "Internal EEPROM recovery >");
DEFINE_STRING(s_ieeprom_writing, "Writing to IEEPROM...");
DEFINE_STRING(s_disable_custom_splash, "Disable custom splash");
DEFINE_STRING(s_custom_splash_already_disabled, "Custom splash already disabled!");
DEFINE_STRING(s_restore_tft_data, "Restore TFT panel data");

DEFINE_STRING(s_cartridge_tests, "Cartridge self-tests >");
DEFINE_STRING(s_flash_fsm_test, "Flash FSM test");
DEFINE_STRING(s_flash_fsm_test_no, "Flash FSM test #%d");
DEFINE_STRING(s_flash_fsm_last_byte, "\nLast byte = %02X\n");

DEFINE_STRING(s_caps_initialization, "INITIALIZATION");
DEFINE_STRING(s_caps_test_suite, "TEST SUITE");
DEFINE_STRING(s_caps_information, "INFORMATION");
DEFINE_STRING(s_mfg_test_success0, "*********************\n");
DEFINE_STRING(s_mfg_test_success1, "*                   *\n");
DEFINE_STRING(s_mfg_test_success2, "* All tests passed! *\n");
DEFINE_STRING(s_run_manufacturing_test, "Run manufacturing tests");

#endif /* _STRINGS_H_ */
