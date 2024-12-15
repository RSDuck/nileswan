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

#ifndef _RTC_H_
#define _RTC_H_

#include <stdbool.h>
#include <stdint.h>

#define S3511A_AMPM       0x80
#define S3511A_POWER_LOST 0x80
#define S3511A_1224       0x40
#define S3511A_INTAE      0x20
#define S3511A_INTME      0x08
#define S3511A_INTFE      0x02

void mcu_rtc_init(void);

void rtc_reset(void);
void rtc_write_status(uint8_t value);
uint8_t rtc_read_status(void);
void rtc_write_datetime(const uint8_t *buffer, bool date);
void rtc_read_datetime(uint8_t *buffer, bool date);
void rtc_write_alarm(uint8_t hour, uint8_t minute);

int rtc_start_command_rx(uint8_t cmd);
int rtc_finish_command_rx(uint8_t *rx, uint8_t *tx);

#endif /* _RTC_H_ */
