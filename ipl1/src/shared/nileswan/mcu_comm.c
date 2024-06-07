/**
 * Copyright (c) 2024 Kemal Afzal
 *
 * Nileswan IPL1 is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * Nileswan IPL1 is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with Nileswan IPL1. If not, see <https://www.gnu.org/licenses/>.
 */

#include "mcu_comm.h"

#include "nileswan.h"

#define BOOTLOADER_SYNC_FRAME 0x5A

#define BOOTLOADER_ACK 0x79
#define BOOTLOADER_NAK 0x1F

#define BOOTLOADER_CMD_GET_VERSION 0x01
#define BOOTLOADER_CMD_ERASE 0x44
#define BOOTLOADER_CMD_WRITE 0x31
#define BOOTLOADER_CMD_READ 0x11
#define BOOTLOADER_CMD_GO 0x21

static bool initialised;

bool mcu_bootloader_ack() {
	// one zero byte is required
	uint8_t value = 0;
	nile_spi_tx(&value, 1);

	bool success = false;

	//uint16_t timeout = 0xFFFF;
	while (1)
	{
		nile_spi_rx_copy(&value, 1, NILE_SPI_MODE_READ);

		if (value == BOOTLOADER_ACK)
		{
			success = true;
			break;
		}
		if (value == BOOTLOADER_NAK)
			break;

		/*timeout--;
		if (timeout)
			return false;*/
	}

	value = BOOTLOADER_ACK;
	nile_spi_tx(&value, 1);
	nile_spi_wait_busy();

	return success;
}

bool mcu_bootloader_start_command(uint8_t command) {
	uint8_t buffer[3];
	buffer[0] = BOOTLOADER_SYNC_FRAME;
	buffer[1] = command;
	buffer[2] = ~command;
	nile_spi_tx(buffer, 3);

	return mcu_bootloader_ack();
}

bool mcu_bootloader_send_32(uint32_t value) {
	uint8_t tx_data[5];
	tx_data[0] = value >> 24;
	tx_data[1] = value >> 16;
	tx_data[2] = value >> 8;
	tx_data[3] = value;
	tx_data[4] = tx_data[0] ^ tx_data[1] ^ tx_data[2] ^ tx_data[3];

	nile_spi_tx(tx_data, 5);

	return mcu_bootloader_ack();
}

bool mcu_bootloader_send_16(uint16_t value)
{
	uint8_t tx_data[3];
	tx_data[0] = value >> 8;
	tx_data[1] = value;
	tx_data[2] = tx_data[0] ^ tx_data[1];

	nile_spi_tx(tx_data, 3);

	return mcu_bootloader_ack();
}

bool mcu_bootloader_send_8(uint8_t value)
{
	uint8_t tx_data[2];
	tx_data[0] = value;
	tx_data[1] = ~tx_data[1];

	nile_spi_tx(tx_data, 2);

	return mcu_bootloader_ack();
}

bool mcu_comm_start() {
	outportw(IO_NILE_SPI_CNT, NILE_SPI_CS_DEV_MCU_SEL | NILE_SPI_390KHZ);

	if (!initialised)
	{
		uint8_t sync_frame = BOOTLOADER_SYNC_FRAME;
		nile_spi_tx(&sync_frame, 1);

		if (!mcu_bootloader_ack())
			return false;
	
		initialised = true;
	}

	return true;
}

void mcu_comm_stop() {
	outportw(IO_NILE_SPI_CNT, NILE_SPI_CS_DEV_TF_DESEL | NILE_SPI_25MHZ);
}

bool mcu_comm_bootloader_version(uint8_t __far* version) {
	if (!mcu_bootloader_start_command(BOOTLOADER_CMD_GET_VERSION))
		return false;

	uint8_t dummy = 0;
	nile_spi_tx(&dummy, 1);
	nile_spi_rx_copy(version, 1, NILE_SPI_MODE_READ);

	return mcu_bootloader_ack();
}

bool mcu_comm_bootloader_erase_memory(uint16_t page_address, uint8_t num_pages) {
	if (!mcu_bootloader_start_command(BOOTLOADER_CMD_ERASE))
		return false;

	// TODO: mass erase

	num_pages -= 1;

	if (!mcu_bootloader_send_16(num_pages))
		return false;

	// a bit slow and ugly, but we churn these commands out at 384 kHz so there isn't
	// really much to be lost here
	uint8_t tx_data[3];
	tx_data[2] = 0;
	uint16_t last_page = page_address + num_pages;
	while (page_address <= last_page) {
		tx_data[0] = page_address >> 8;
		tx_data[1] = page_address;
		tx_data[2] ^= tx_data[0];
		tx_data[2] ^= tx_data[1];

		uint16_t xfer_len = 2;
		if (page_address == last_page)
			xfer_len = 3;

		nile_spi_tx(tx_data, xfer_len);
	
		page_address++;
	}
	
	return mcu_bootloader_ack();
}

bool mcu_comm_bootloader_read_memory(uint32_t addr, uint8_t __far* data, uint8_t size_minus_one) {
	if (!mcu_bootloader_start_command(BOOTLOADER_CMD_READ))
		return false;
	
	if (!mcu_bootloader_send_32(addr))
		return false;

	if (!mcu_bootloader_send_8(size_minus_one))
		return false;

	uint8_t dummy = 0;
	nile_spi_tx(&dummy, 1);
	nile_spi_rx_copy(data, (uint16_t)size_minus_one + 1, NILE_SPI_MODE_READ);

	return true;
}

#include <ws/hardware.h>

bool mcu_comm_bootloader_write_memory(uint32_t addr, const uint8_t __far* data, uint8_t size_minus_one) {
	if (!mcu_bootloader_start_command(BOOTLOADER_CMD_WRITE))
		return false;

	outportb(IO_LCD_SEG, 0x11);

	if (!mcu_bootloader_send_32(addr))
		return false;

	outportb(IO_LCD_SEG, 0x12);

	nile_spi_tx(&size_minus_one, 1);
	nile_spi_tx(data, (uint16_t)size_minus_one + 1);

	outportb(IO_LCD_SEG, 0x13);
	// calculate checksum in the meantime
	uint8_t checksum = size_minus_one;
	for (uint16_t i = 0; i <= size_minus_one; i++) {
		checksum ^= data[i];
	}
	outportb(IO_LCD_SEG, 0x14);
	nile_spi_tx(&checksum, 1);
	outportb(IO_LCD_SEG, 0x15);

	return mcu_bootloader_ack();
}

bool mcu_comm_bootloader_go(uint32_t addr) {
	if (!mcu_bootloader_start_command(BOOTLOADER_CMD_GO))
		return false;

	return mcu_bootloader_send_32(addr);
}

bool mcu_comm_is_bootloader() {
	return false;
}
