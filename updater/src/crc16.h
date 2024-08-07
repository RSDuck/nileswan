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

#ifndef __CRC16_H__
#define __CRC16_H__

#include <stdint.h>
#include <wonderful.h>

uint16_t crc16(const void __far *data, uint16_t len);

#endif /* __CRC16_H__ */
