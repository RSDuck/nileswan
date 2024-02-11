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
#include <ws/util.h>
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

#ifdef NILESWAN_IPL1
uint8_t diskio_detail_code;
#define set_detail_code(v) diskio_detail_code = v
#else
#define set_detail_code(v)
#endif

static uint8_t card_status = STA_NOINIT;
static bool card_hc = false;

static bool tfc_send_cmd(uint8_t cmd, uint8_t crc, uint32_t arg) {
	uint8_t buffer[7];
	buffer[0] = 0xFF;
	buffer[1] = cmd;
	buffer[2] = arg >> 24;
	buffer[3] = arg >> 16;
	buffer[4] = arg >> 8;
	buffer[5] = arg;
	buffer[6] = crc;
	return nile_spi_tx(buffer, 7);
}

static uint8_t tfc_read_response(uint8_t *buffer, uint16_t size) {
	buffer[0] = 0xFF;
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

	card_hc = false;
	card_status = STA_NOINIT;

	set_detail_code(0);
	nile_spi_wait_busy();
	outportw(IO_NILE_SPI_CNT, NILE_SPI_DEV_TF | NILE_SPI_390KHZ | NILE_SPI_CS_HIGH);

	uint8_t powcnt = inportb(IO_NILE_POW_CNT);
	if (!(powcnt & NILE_POW_TF)) {
		// Power card on
		powcnt |= NILE_POW_TF;
		outportb(IO_NILE_POW_CNT, powcnt);
		// Wait a few milliseconds
		ws_busywait(5000);
	}

	// Initialize: emit clocks with CS high
	nile_spi_rx(buffer, 10, NILE_SPI_MODE_READ);
	outportw(IO_NILE_SPI_CNT, NILE_SPI_DEV_TF | NILE_SPI_390KHZ | NILE_SPI_CS_LOW);

	// Reset card
	tfc_send_cmd(TFC_GO_IDLE_STATE, 0x95, 0);
	if (tfc_read_response(buffer, 1) & ~TFC_R1_IDLE) {
		// Error/No response
		set_detail_code(1);
		goto card_init_failed;
	}

	// Query interface configuration
	tfc_send_cmd(TFC_SEND_IF_COND, 0x87, 0x000001AA);
	if (!(tfc_read_response(buffer, 5) & ~TFC_R1_IDLE)) {
		// Check voltage/pattern value match
		if ((buffer[3] & 0xF) == 0x1 && buffer[4] == 0xAA) {
			// Attempt high-capacity card init
			retries = MAX_RETRIES ^ 0xFF;
			while (++retries) {
				tfc_send_cmd(TFC_APP_PREFIX, 0x95, 0);
				if (!(tfc_read_response(buffer, 1) & ~TFC_R1_IDLE)) {
					tfc_send_cmd(TFC_APP_SEND_OP_COND, 0x95, 1UL << 30);
					uint8_t init_response = tfc_read_response(buffer, 1);
					if (init_response & ~TFC_R1_IDLE) {
						// Initialization error
						retries = 0;
						break;
					} else if (!init_response) {
						// Initialization success
						break;
					}
					// Card still idle, try again
				}
			}

			// Card init successful?
			if (retries) {
				// Read OCR to check for HC card
				tfc_send_cmd(TFC_READ_OCR, 0x95, 0);
				if (!tfc_read_response(buffer, 5)) {
					if (buffer[1] & 0x40) {
						card_hc = true;
					}
				}
				goto card_init_complete_hc;
			}
		} else {
			// Voltage/pattern value mismatch
			set_detail_code(2);
			return card_status;
		}
	}

	// Attempt card init
	retries = MAX_RETRIES ^ 0xFF;
	while (++retries) {
		tfc_send_cmd(TFC_APP_PREFIX, 0x95, 0);
		if (!(tfc_read_response(buffer, 1) & ~TFC_R1_IDLE)) {
			tfc_send_cmd(TFC_APP_SEND_OP_COND, 0x95, 0);
			uint8_t init_response = tfc_read_response(buffer, 1);
			if (init_response & ~TFC_R1_IDLE) {
				// Initialization error
				retries = 0;
				break;
			} else if (!init_response) {
				// Initialization success
				goto card_init_complete;
			}
		}
	}

	// Attempt legacy card init
	retries = MAX_RETRIES ^ 0xFF;
	while (++retries) {
		tfc_send_cmd(TFC_SEND_OP_COND, 0x95, 0);
		uint8_t init_response = tfc_read_response(buffer, 1);
		if (init_response & ~TFC_R1_IDLE) {
			// Initialization error
			retries = 0;
			break;
		} else if (!init_response) {
			// Initialization success
			goto card_init_complete;
		}
	}

	set_detail_code(3);
card_init_failed:
	// Power off card
	outportb(IO_NILE_POW_CNT, 0);
	return card_status;

card_init_complete:
	// set block size to 512
	tfc_send_cmd(TFC_SET_BLOCKLEN, 0x95, 512);
	if (tfc_read_response(buffer, 1)) {
		set_detail_code(4);
		return card_status;
	}

card_init_complete_hc:
	outportb(IO_NILE_POW_CNT, powcnt | NILE_POW_CLOCK);
	outportw(IO_NILE_SPI_CNT, NILE_SPI_DEV_TF | NILE_SPI_25MHZ | NILE_SPI_CS_HIGH);
	card_status = 0;
	return card_status;
}

/* Wait until the TF card is finished (responds 0xFF...) */
static uint8_t disk_wait_busy(uint8_t resp) {
	uint16_t timeout = 0;
#ifdef __OPTIMIZE_SIZE__
	// smaller but slower code variant
	uint16_t resp_busy[24];
	while (--timeout) {
		// wait for 0xFFFF to signify end of busy time
		if (!nile_spi_rx(&resp_busy, 48, NILE_SPI_MODE_READ))
			return 0xFF;
		if (resp_busy[23] == 0xFFFF)
			break;
	}

	if (!timeout)
		return 0xFF;
	return resp;
#else
	if (!nile_spi_wait_busy())
		return 0xFF;

	uint16_t cnt = inportw(IO_NILE_SPI_CNT);
	uint16_t new_cnt = (48 - 1) | NILE_SPI_MODE_READ | (cnt & 0x7800);
	volatile uint16_t prev_bank = inportw(IO_BANK_2003_ROM1);
	outportw(IO_BANK_2003_ROM1, NILE_SEG_ROM_RX);

	// send first read request
	outportw(IO_NILE_SPI_CNT, new_cnt | NILE_SPI_START);

	while (--timeout) {
		if (!nile_spi_wait_busy()) {
			resp = 0xFF;
			break;
		}

		// send second read request
		new_cnt ^= NILE_SPI_BUFFER_IDX;
		outportw(IO_NILE_SPI_CNT, new_cnt | NILE_SPI_START);

		// read first read request
		if (*((uint16_t __far*) MK_FP(0x3000, 46)) == 0xFFFF)
			break;
	}

	outportw(IO_BANK_2003_ROM1, prev_bank);
	return resp;
#endif
}

static uint8_t disk_wait_r1b(void) {
	uint8_t resp = 0xFF;

	if (!nile_spi_rx(&resp, 1, NILE_SPI_MODE_WAIT_READ) || resp)
		return resp;

	return disk_wait_busy(resp);
}

DRESULT disk_read (BYTE pdrv, BYTE __far* buff, LBA_t sector, UINT count) {
	uint8_t result = RES_ERROR;
	uint8_t resp[7];

	if (!card_hc)
		sector <<= 9;

	outportw(IO_NILE_SPI_CNT, NILE_SPI_DEV_TF | NILE_SPI_25MHZ | NILE_SPI_CS_LOW);

#ifdef USE_MULTI_TRANSFER_READS
	bool multi_transfer = count > 1;
	if (!tfc_send_cmd(multi_transfer ? TFC_READ_MULTIPLE_BLOCK : TFC_READ_SINGLE_BLOCK, 0x95, sector))
		goto disk_read_end;
	if (!nile_spi_rx(resp, 1, NILE_SPI_MODE_WAIT_READ))
		goto disk_read_end;
	if (resp[0])
		goto disk_read_end;

	while (count) {
		if (!nile_spi_rx(resp, 1, NILE_SPI_MODE_WAIT_READ))
			goto disk_read_stop;
		if (resp[0] != 0xFE)
			goto disk_read_stop;
		if (!nile_spi_rx(buff, 512, NILE_SPI_MODE_READ))
			goto disk_read_stop;
		if (!nile_spi_rx(resp, 2, NILE_SPI_MODE_READ))
			goto disk_read_stop;
		buff += 512;
		count--;
	}

disk_read_stop:
	if (multi_transfer) {
		resp[0] = TFC_STOP_TRANSMISSION;
		resp[5] = 0x95;
		resp[6] = 0xFF; // skip one byte
		if (!nile_spi_tx(resp, 7))
			goto disk_read_end;
		if (disk_wait_r1b())
			goto disk_read_end;
	}
#else
	while (count) {
		if (!tfc_send_cmd(TFC_READ_SINGLE_BLOCK, 0x95, sector))
			goto disk_read_end;
		if (!nile_spi_rx(resp, 1, NILE_SPI_MODE_WAIT_READ))
			goto disk_read_end;
		if (resp[0])
			goto disk_read_end;

		if (!nile_spi_rx(resp, 1, NILE_SPI_MODE_WAIT_READ))
			goto disk_read_end;
		if (resp[0] != 0xFE)
			goto disk_read_end;
		if (!nile_spi_rx(buff, 512, NILE_SPI_MODE_READ))
			goto disk_read_end;
		if (!nile_spi_rx(resp, 2, NILE_SPI_MODE_READ))
			goto disk_read_end;
		buff += 512;
		sector += card_hc ? 1 : 512;
		count--;
	}
#endif

	result = RES_OK;
disk_read_end:
	nile_spi_wait_busy();
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
	if (!tfc_send_cmd(multi_transfer ? TFC_WRITE_MULTIPLE_BLOCK : TFC_WRITE_BLOCK, 0x95, sector))
		goto disk_read_end;
	if (!nile_spi_rx(resp, 1, NILE_SPI_MODE_WAIT_READ))
		goto disk_read_end;
	if (resp[0])
		goto disk_read_end;

	while (count) {
		resp[0] = 0xFF;
		resp[1] = multi_transfer ? 0xFC : 0xFE;
		if (!nile_spi_tx(resp, 2))
			goto disk_read_stop;
		if (!nile_spi_tx(buff, 512))
			goto disk_read_stop;
		if (!nile_spi_tx(resp, 2))
			goto disk_read_stop;
		if (!nile_spi_rx(resp, 1, NILE_SPI_MODE_WAIT_READ))
			goto disk_read_stop;
		// TODO: error handling?
		buff += 512;
		count--;
	}
disk_read_stop:

	if (multi_transfer) {
		resp[0] = 0xFF;
		resp[1] = 0xFD;
		resp[2] = 0xFF;
		nile_spi_tx(resp, 3);
		if (disk_wait_busy(0x00))
			goto disk_read_end;
	}
#else
	while (count) {
		if (!tfc_send_cmd(TFC_WRITE_BLOCK, 0x95, sector))
			goto disk_read_end;
		if (!nile_spi_rx(resp, 1, NILE_SPI_MODE_WAIT_READ))
			goto disk_read_end;
		if (resp[0])
			goto disk_read_end;

		resp[0] = 0xFF;
		resp[1] = 0xFE;
		if (!nile_spi_tx(resp, 2))
			goto disk_read_end;
		if (!nile_spi_tx(buff, 512))
			goto disk_read_end;
		if (!nile_spi_tx(resp, 2))
			goto disk_read_end;
		if (!nile_spi_rx(resp, 1, NILE_SPI_MODE_WAIT_READ))
			goto disk_read_end;
		// TODO: error handling?
		buff += 512;
		sector += 512;
		count--;
	}
#endif

	result = RES_OK;
disk_read_end:
	nile_spi_wait_busy();
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
