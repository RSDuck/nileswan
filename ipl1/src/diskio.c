/**
 * Copyright (c) 2024 Adrian Siekierka
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

#include <string.h>
#include <ws.h>
#include <ws/display.h>
#include "nileswan/nileswan.h"
#include "fatfs/ff.h"
#include "fatfs/diskio.h"

// #define USE_DEVFS
#define USE_MULTI_TRANSFER_READS
#define USE_MULTI_TRANSFER_WRITES

#ifdef USE_DEVFS

DSTATUS disk_status(BYTE pdrv) {
	return 0;
}

DSTATUS disk_initialize(BYTE pdrv) {
	return 0;
}

DRESULT disk_read (BYTE pdrv, BYTE __far* buff, LBA_t sector, UINT count) {
	sector <<= 9;

	while (count) {
		cpu_irq_disable();
		outportw(IO_BANK_ROM1, sector >> 16);
		memcpy(buff, MK_FP(0x3000, (uint16_t) sector), 512);
		cpu_irq_enable();

		buff += 512;
		sector += 512;
		count--;
	}

	return RES_OK;
}

#if FF_FS_READONLY == 0

DRESULT disk_write (BYTE pdrv, const BYTE __far* buff, LBA_t sector, UINT count) {
	return RES_PARERR;
}

#endif

DRESULT disk_ioctl (BYTE pdrv, BYTE cmd, void *buff) {
	return RES_PARERR;
}

#else

// TF card-related defines

#define TFC_CMD(n) (0x40 | (n))
#define TFC_GO_IDLE_STATE        TFC_CMD(0)
#define TFC_SEND_OP_COND         TFC_CMD(1)
#define TFC_SEND_IF_COND         TFC_CMD(8)
#define TFC_SEND_CSD             TFC_CMD(9)
#define TFC_SEND_CID             TFC_CMD(10)
#define TFC_STOP_TRANSMISSION    TFC_CMD(12)
#define TFC_SET_BLOCKLEN         TFC_CMD(16)
#define TFC_READ_SINGLE_BLOCK    TFC_CMD(17)
#define TFC_READ_MULTIPLE_BLOCK  TFC_CMD(18)
#define TFC_SET_BLOCK_COUNT      TFC_CMD(23)
#define TFC_WRITE_BLOCK          TFC_CMD(24)
#define TFC_WRITE_MULTIPLE_BLOCK TFC_CMD(25)
#define TFC_APP_SEND_OP_COND     TFC_CMD(41)
#define TFC_APP_PREFIX           TFC_CMD(55)
#define TFC_READ_OCR             TFC_CMD(58)
#define TFC_R1_IDLE        0x01
#define TFC_R1_ERASE_RESET 0x02
#define TFC_R1_ILLEGAL_CMD 0x04
#define TFC_R1_CRC_ERROR   0x08
#define TFC_R1_ERASE_ERROR 0x10
#define TFC_R1_ADDR_ERROR  0x20
#define TFC_R1_PARAM_ERROR 0x40

// FatFS API implementation

static uint8_t card_status = STA_NOINIT;
static bool card_hc = false;

static void tfc_send_cmd(uint8_t cmd, uint8_t crc, uint32_t arg) {
	uint8_t buffer[6];
	buffer[0] = cmd;
	buffer[1] = arg >> 24;
	buffer[2] = arg >> 16;
	buffer[3] = arg >> 8;
	buffer[4] = arg;
	buffer[5] = crc;
	nile_spi_tx(buffer, 6);
}

static uint8_t tfc_read_response(uint8_t *buffer, uint16_t size) {
	nile_spi_rx(buffer, size + 2, NILE_SPI_MODE_WAIT_READ);
	return buffer[0];
}

DSTATUS disk_status(BYTE pdrv) {
	return card_status;
}

#define MAX_RETRIES 10

DSTATUS disk_initialize(BYTE pdrv) {
	uint8_t retries;
	uint8_t buffer[10];

	nile_spi_wait_busy();
	outportw(IO_NILE_SPI_CNT, NILE_SPI_DEV_TF | NILE_SPI_390KHZ | NILE_SPI_CS_HIGH);
	nile_spi_rx(buffer, 10, NILE_SPI_MODE_READ);
	outportw(IO_NILE_SPI_CNT, NILE_SPI_DEV_TF | NILE_SPI_390KHZ | NILE_SPI_CS_LOW);

	tfc_send_cmd(TFC_GO_IDLE_STATE, 0x95, 0); // Reset card
	if (tfc_read_response(buffer, 1) & ~TFC_R1_IDLE) {
		return card_status;
	}

	tfc_send_cmd(TFC_SEND_IF_COND, 0x87, 0x000001AA);
	if (!(tfc_read_response(buffer, 5) & ~TFC_R1_IDLE)) {
		if ((buffer[3] & 0xF) == 0x1 && buffer[4] == 0xAA) {
			// attempt ACMD41 HC init
			retries = MAX_RETRIES ^ 0xFF;
			while (++retries) {
				tfc_send_cmd(TFC_APP_PREFIX, 0x95, 0);
				if (!(tfc_read_response(buffer, 1) & ~TFC_R1_IDLE)) {
					tfc_send_cmd(TFC_APP_SEND_OP_COND, 0x95, 1UL << 30);
					if (!tfc_read_response(buffer, 1)) {
						break;
					}
				}
			}
			if (retries) {
				// read OCR to check for HC card
				tfc_send_cmd(TFC_READ_OCR, 0x95, 0);
				if (!tfc_read_response(buffer, 5)) {
					if (buffer[1] & 0x40) {
						card_hc = true;
					}
				}
				goto card_init_complete;
			}
		}
	}

	// attempt ACMD41 init
	retries = MAX_RETRIES ^ 0xFF;
	while (++retries) {
		tfc_send_cmd(TFC_APP_PREFIX, 0x95, 0);
		if (!(tfc_read_response(buffer, 1) & ~TFC_R1_IDLE)) {
			tfc_send_cmd(TFC_APP_SEND_OP_COND, 0x95, 1UL << 30);
			if (!tfc_read_response(buffer, 1)) {
				goto card_init_complete;
			}
		}
	}

	// attempt CMD1 init
	retries = MAX_RETRIES ^ 0xFF;
	while (++retries) {
		tfc_send_cmd(TFC_SEND_OP_COND, 0x95, 0);
		if (!tfc_read_response(buffer, 1)) {
			goto card_init_complete;
		}
	}

	// no more init modes
	return card_status;

card_init_complete:
	// set block size to 512
	tfc_send_cmd(TFC_SET_BLOCKLEN, 0x95, 512);
	if (tfc_read_response(buffer, 1)) {
		return card_status;
	}

	outportw(IO_NILE_SPI_CNT, NILE_SPI_DEV_TF | NILE_SPI_25MHZ | NILE_SPI_CS_HIGH);
	card_status = 0;
	return card_status;
}

DRESULT disk_read (BYTE pdrv, BYTE __far* buff, LBA_t sector, UINT count) {
	uint8_t result = RES_ERROR;
	uint8_t resp[4];

	if (!card_hc)
		sector <<= 9;

	outportw(IO_NILE_SPI_CNT, NILE_SPI_DEV_TF | NILE_SPI_25MHZ | NILE_SPI_CS_LOW);

#ifdef USE_MULTI_TRANSFER_READS
	bool multi_transfer = count > 1;
	tfc_send_cmd(multi_transfer ? TFC_READ_MULTIPLE_BLOCK : TFC_READ_SINGLE_BLOCK, 0x95, sector);
	nile_spi_rx(resp, 1, NILE_SPI_MODE_WAIT_READ);
	if (resp[0])
		goto disk_read_end;


	while (count) {
		nile_spi_rx(resp, 1, NILE_SPI_MODE_WAIT_READ);
		if (resp[0] != 0xFE)
			goto disk_read_end;
		nile_spi_rx(buff, 512, NILE_SPI_MODE_READ);
		nile_spi_rx(resp, 2, NILE_SPI_MODE_READ);
		buff += 512;
		count--;
	}

	if (multi_transfer) {
		tfc_send_cmd(TFC_STOP_TRANSMISSION, 0x95, sector);
		nile_spi_rx(resp, 1, NILE_SPI_MODE_READ);
		tfc_read_response(resp, 1);
	}
#else
	while (count) {
		tfc_send_cmd(TFC_READ_SINGLE_BLOCK, 0x95, sector);
		nile_spi_rx(resp, 1, NILE_SPI_MODE_WAIT_READ);
		if (resp[0])
			goto disk_read_end;

		nile_spi_rx(resp, 1, NILE_SPI_MODE_WAIT_READ);
		if (resp[0] != 0xFE)
			goto disk_read_end;
		nile_spi_rx(buff, 512, NILE_SPI_MODE_READ);
		nile_spi_rx(resp, 2, NILE_SPI_MODE_READ);
		buff += 512;
		sector += card_hc ? 1 : 512;
		count--;
	}
#endif

	result = RES_OK;
disk_read_end:
	outportw(IO_NILE_SPI_CNT, NILE_SPI_DEV_TF | NILE_SPI_25MHZ | NILE_SPI_CS_HIGH);
	return result;
}

#if FF_FS_READONLY == 0

DRESULT disk_write (BYTE pdrv, const BYTE __far* buff, LBA_t sector, UINT count) {
	uint8_t result = RES_ERROR;
	uint8_t resp[4];

	if (!card_hc)
		sector <<= 9;

	outportw(IO_NILE_SPI_CNT, NILE_SPI_DEV_TF | NILE_SPI_25MHZ | NILE_SPI_CS_LOW);

#ifdef USE_MULTI_TRANSFER_WRITES
	bool multi_transfer = count > 1;
	tfc_send_cmd(multi_transfer ? TFC_WRITE_MULTIPLE_BLOCK : TFC_WRITE_BLOCK, 0x95, sector);
	nile_spi_rx(resp, 1, NILE_SPI_MODE_WAIT_READ);
	if (resp[0])
		goto disk_read_end;

	while (count) {
		resp[0] = 0xFF;
		resp[1] = multi_transfer ? 0xFC : 0xFE;
		nile_spi_tx(resp, 2);
		nile_spi_tx(buff, 512);
		nile_spi_tx(resp, 2);
		nile_spi_rx(resp, 1, NILE_SPI_MODE_WAIT_READ);
		// TODO: error handling?
		buff += 512;
		count--;
	}

	if (multi_transfer) {
		resp[0] = 0xFF;
		resp[1] = 0xFD;
		resp[2] = 0xFF;
		nile_spi_tx(resp, 3);
	}
#else
	while (count) {
		tfc_send_cmd(TFC_WRITE_BLOCK, 0x95, sector);
		nile_spi_rx(resp, 1, NILE_SPI_MODE_WAIT_READ);
		if (resp[0])
			goto disk_read_end;

		resp[0] = 0xFF;
		resp[1] = 0xFE;
		nile_spi_tx(resp, 2);
		nile_spi_tx(buff, 512);
		nile_spi_tx(resp, 2);
		nile_spi_rx(resp, 1, NILE_SPI_MODE_WAIT_READ);
		// TODO: error handling?
		buff += 512;
		sector += 512;
		count--;
	}
#endif

	result = RES_OK;
disk_read_end:
	outportw(IO_NILE_SPI_CNT, NILE_SPI_DEV_TF | NILE_SPI_25MHZ | NILE_SPI_CS_HIGH);
	return result;
}

#endif

DRESULT disk_ioctl (BYTE pdrv, BYTE cmd, void *buff) {
	if (cmd == CTRL_SYNC)
		return RES_OK;
	return RES_PARERR;
}

#endif