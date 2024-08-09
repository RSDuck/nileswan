/**
 * Copyright (c) 2024 Adrian Siekierka
 *
 * Nileswan Updater is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * Nileswan Updater is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with Nileswan Updater. If not, see <https://www.gnu.org/licenses/>.
 */

static const char __far initializing_updater[] = "Initializing updater";
static const char __far unpacking_part[] = "Unpacking #%d";
static const char __far verifying_part[] = "Verifying #%d";
static const char __far erasing_part[] = "Erasing #%d";
static const char __far flashing_part[] = "Flashing #%d";
static const char __far fatal_error[] = "Fatal error";
static const char __far crc_error_part[] = "CRC error #%d";
static const char __far crc_error_a_e[] = "A: %04X E: %04X";
static const char __far unknown_flash_error[] = "Unknown flash %lX";
static const char __far corrupt_data[] = "Corrupt data! %04X";

static const char __far low_battery_line0[] = "Low battery!";
static const char __far low_battery_line1[] = "Update not possible.";
static const char __far * __far low_battery_screen[] = {
    low_battery_line0,
    low_battery_line1
};

//                                              12345678901234567890123456
static const char __far update_start_line0[] = " nileswan firmware update";
static const char __far update_start_line2[] = "Current version:";
static const char __far update_start_line3[] = " Update version:";
static const char __far update_start_line5[] = " Do not turn your console";
static const char __far update_start_line6[] = "  off during the update!";
static const char __far update_start_line8[] = "  Press [A] to continue.";
static const char __far * __far update_start_screen[] = {
    update_start_line0,
    NULL,
    NULL,
    update_start_line2,
    update_start_line3,
    NULL,
    NULL,
    update_start_line5,
    update_start_line6,
    NULL,
    NULL,
    update_start_line8
};
#define UPDATE_START_SCREEN_LINES 12
#define UPDATE_START_SCREEN_Y_OFFSET ((DISPLAY_HEIGHT - UPDATE_START_SCREEN_LINES) >> 1)
#define UPDATE_START_SCREEN_VER_X_OFFSET 18
#define UPDATE_START_SCREEN_VER_Y_OFFSET (UPDATE_START_SCREEN_Y_OFFSET + 3)

//                                                12345678901234567890123456
static const char __far update_success_line0[] = " Firmware update success!";
static const char __far update_success_line2[] = " Press [A] to shut down.";
static const char __far * __far update_success_screen[] = {
    update_success_line0,
    NULL,
    update_success_line2,
    NULL
};
#define UPDATE_SUCCESS_SCREEN_LINES 4
#define UPDATE_SUCCESS_SCREEN_Y_OFFSET ((DISPLAY_HEIGHT - UPDATE_SUCCESS_SCREEN_LINES) >> 1)

//                                                 12345678901234567890123456
static const char __far update_successM_line2[] = "  The console may now be";
static const char __far update_successM_line3[] = "       powered off.";
static const char __far * __far update_successM_screen[] = {
    update_success_line0,
    NULL,
    update_successM_line2,
    update_successM_line3
};
