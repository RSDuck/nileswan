#ifndef _STRINGS_H_
#define _STRINGS_H_

#define DEFINE_STRING_LOCAL(name, value) static const char __far name[] = value
#ifdef STRINGS_H_IMPLEMENTATION
#define DEFINE_STRING(name, value) const char __far name[] = value
#else
#define DEFINE_STRING(name, value) extern const char __far name[]
#endif

DEFINE_STRING(s_nileswan_header,
    "     _ _\n"
    " _ _(_) |___ ____ __ ____ _ _ _\n"
    "| ' \\ | / -_|_-< V  V / _` | ' \\\n"
    "|_|_|_|_\\___/__/\\_/\\_/\\__,_|_|_|\n");

DEFINE_STRING(s_initializing_updater, "Initializing updater");
DEFINE_STRING(s_unpacking_part, "Unpacking #%d");
DEFINE_STRING(s_verifying_part, "Verifying #%d");
DEFINE_STRING(s_erasing_part, "Erasing #%d");
DEFINE_STRING(s_flashing_part, "Flashing #%d");
DEFINE_STRING(s_fatal_error, "Fatal error");
DEFINE_STRING(s_crc_error_part, "CRC error #%d\n");
DEFINE_STRING(s_crc_error_a_e, "A: %04X E: %04X\n");
DEFINE_STRING(s_unknown_flash_error, "Unknown flash %lX");
DEFINE_STRING(s_corrupt_data, "Corrupt data! %04X");

DEFINE_STRING(s_low_battery, "Low battery! Update not possible.");

DEFINE_STRING(s_update_title, "nileswan firmware updater");
DEFINE_STRING(s_update_version, "Update version: ");
DEFINE_STRING(s_update_disclaimer, "Do not turn your console off during the update!\n\nPress [A] to continue.");

DEFINE_STRING(s_update_success, "== Firmware update success! ==\n\n");
DEFINE_STRING(s_press_a_shutdown, "Press [A] to shut down.");
DEFINE_STRING(s_manual_shutdown, "The console may now be powered off.");

#endif /* _STRINGS_H_ */
