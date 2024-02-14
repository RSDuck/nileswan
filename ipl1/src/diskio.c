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
#define TFC_ACMD(n) (0xC0 | (n))
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
#define TFC_APP_SEND_OP_COND     TFC_ACMD(41)
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

/* Wait until the TF card is finished */
/* Returns resp on success, 0xFF on failure */
static uint8_t nile_tf_wait_ready(uint8_t resp) {
	uint32_t timeout = 1500000;
	uint8_t resp_busy[1];
	while (--timeout) {
		// wait for 0xFF to signify end of busy time
		if (!nile_spi_rx_copy(&resp_busy, 1, NILE_SPI_MODE_READ))
			return 0xFF;
		if (resp_busy[0] == 0xFF)
			break;
	}

	if (!timeout)
		return 0xFF;
	return resp;
}

static bool nile_tf_cs_high(void) {
	if (!nile_spi_wait_busy())
		return false;
	outportw(IO_NILE_SPI_CNT, inportw(IO_NILE_SPI_CNT) & ~NILE_SPI_CS);
	if (!nile_spi_rx(1, NILE_SPI_MODE_READ))
		return false;
	return true;
}

static bool nile_tf_cs_low(void) {
	if (!nile_spi_wait_busy())
		return false;
	outportw(IO_NILE_SPI_CNT, inportw(IO_NILE_SPI_CNT) | NILE_SPI_CS);
	if (!nile_spi_rx(1, NILE_SPI_MODE_READ))
		return false;
	if (nile_tf_wait_ready(0x00))
		return false;
	return true;
}

static uint8_t nile_tf_read_response_r1b(void) {
	uint8_t resp = 0xFF;

	if (!nile_spi_rx_copy(&resp, 1, NILE_SPI_MODE_WAIT_READ) || resp)
		return resp;

	return nile_tf_wait_ready(resp);
}

static uint8_t nile_tf_command(uint8_t cmd, uint32_t arg, uint8_t crc, uint8_t *recv_buffer, uint16_t size) {
	uint8_t cmd_buffer[6];
	recv_buffer[0] = 0xFF;

	if (cmd & 0x80) {
		uint8_t resp = nile_tf_command(TFC_APP_PREFIX, 0x95, 0, recv_buffer, 1);
		if (resp & ~TFC_R1_IDLE)
			return resp;
	}

	if (!nile_tf_cs_high())
		return 0xFF;
	if (!nile_tf_cs_low())
		return 0xFF;

	cmd_buffer[0] = cmd & 0x7F;
	cmd_buffer[1] = arg >> 24;
	cmd_buffer[2] = arg >> 16;
	cmd_buffer[3] = arg >> 8;
	cmd_buffer[4] = arg;
	cmd_buffer[5] = crc;
	if (!nile_spi_tx(cmd_buffer, sizeof(cmd_buffer)))
		return 0xFF;
	if (!nile_spi_rx_copy(recv_buffer, size + 1, NILE_SPI_MODE_WAIT_READ))
		return 0xFF;
	return recv_buffer[0];
}

DSTATUS disk_status(BYTE pdrv) {
	return nile_ipl_data->card_state == 0 ? STA_NOINIT : 0;
}

#define MAX_RETRIES 200

DSTATUS disk_initialize(BYTE pdrv) {
	uint8_t retries;
	uint8_t buffer[8];

	if (nile_ipl_data->card_state != 0) return 0;

	nile_ipl_data->card_state = 0;
	nile_spi_timeout_ms = 1000;

	set_detail_code(0);
	outportw(IO_NILE_SPI_CNT, NILE_SPI_DEV_TF | NILE_SPI_390KHZ | NILE_SPI_CS_HIGH);
	nile_tf_cs_high();

	uint8_t powcnt = inportb(IO_NILE_POW_CNT);
	if (!(powcnt & NILE_POW_TF)) {
		// Power card on
		powcnt |= NILE_POW_TF;
		outportb(IO_NILE_POW_CNT, powcnt);
		// Wait 250 milliseconds
		for (uint8_t i = 0; i < 5; i++)
			ws_busywait(50000);
	}

	nile_spi_rx(10, NILE_SPI_MODE_READ);

	// Reset card
	if (nile_tf_command(TFC_GO_IDLE_STATE, 0, 0x95, buffer, 1) & ~TFC_R1_IDLE) {
		// Error/No response
		set_detail_code(1);
		goto card_init_failed;
	}

	// Query interface configuration
	if (!(nile_tf_command(TFC_SEND_IF_COND, 0x000001AA, 0x87, buffer, 5) & ~TFC_R1_IDLE)) {
		// Check voltage/pattern value match
		if ((buffer[3] & 0xF) == 0x1 && buffer[4] == 0xAA) {
			// Attempt high-capacity card init
			retries = MAX_RETRIES;
			nile_spi_timeout_ms = 10;
			while (--retries) {
				uint8_t init_response = nile_tf_command(TFC_APP_SEND_OP_COND, 1UL << 30, 0x95, buffer, 1);
				if (init_response & ~TFC_R1_IDLE) {
					// Initialization error
					retries = 0;
					break;
				} else if (!init_response) {
					// Initialization success
					nile_ipl_data->card_state = NILE_CARD_TYPE_TF2;
					break;
				}
				// Card still idle, try again
			}

			// Card init successful?
			if (retries) {
				// Read OCR to check for HC card
				if (!nile_tf_command(TFC_READ_OCR, 0, 0x95, buffer, 5)) {
					if (buffer[1] & 0x40) {
						nile_ipl_data->card_state |= NILE_CARD_BLOCK_ADDRESSING;
					}
				}
				goto card_init_complete_hc;
			}
		} else {
			// Voltage/pattern value mismatch
			set_detail_code(2);
			goto card_init_failed;
		}
	}

	// Attempt card init
	retries = MAX_RETRIES;
	nile_spi_timeout_ms = 10;
	while (--retries) {
		uint8_t init_response = nile_tf_command(TFC_APP_SEND_OP_COND, 0, 0x95, buffer, 1);
		if (init_response & ~TFC_R1_IDLE) {
			// Initialization error
			retries = 0;
			break;
		} else if (!init_response) {
			// Initialization success
			nile_ipl_data->card_state = NILE_CARD_TYPE_TF1;
			goto card_init_complete;
		}
	}

	// Attempt legacy card init
	retries = MAX_RETRIES;
	while (--retries) {
		uint8_t init_response = nile_tf_command(TFC_SEND_OP_COND, 0, 0x95, buffer, 1);
		if (init_response & ~TFC_R1_IDLE) {
			// Initialization error
			retries = 0;
			break;
		} else if (!init_response) {
			// Initialization success
			nile_ipl_data->card_state = NILE_CARD_TYPE_MMC;
			goto card_init_complete;
		}
	}

	set_detail_code(3);
card_init_failed:
	// Power off card
	outportb(IO_NILE_POW_CNT, 0);
	outportw(IO_NILE_SPI_CNT, 0);
	return STA_NOINIT;

card_init_complete:
	nile_spi_timeout_ms = 100;
	if (!(nile_ipl_data->card_state & NILE_CARD_BLOCK_ADDRESSING)) {
		// set block size to 512
		if (nile_tf_command(TFC_SET_BLOCKLEN, 512, 0x95, buffer, 1)) {
			set_detail_code(4);
			goto card_init_failed;
		}
	}

card_init_complete_hc:
	nile_spi_timeout_ms = 100;
	nile_tf_cs_high();
	outportb(IO_NILE_POW_CNT, powcnt | NILE_POW_CLOCK);
	outportw(IO_NILE_SPI_CNT, NILE_SPI_DEV_TF | NILE_SPI_25MHZ | NILE_SPI_CS_HIGH);
	return 0;
}

DRESULT disk_read (BYTE pdrv, BYTE __far* buff, LBA_t sector, UINT count) {
	uint8_t result = RES_ERROR;
	uint8_t resp[8];

	if (!(nile_ipl_data->card_state & NILE_CARD_BLOCK_ADDRESSING))
		sector <<= 9;

#ifdef USE_MULTI_TRANSFER_READS
	bool multi_transfer = count > 1;
	if (nile_tf_command(multi_transfer ? TFC_READ_MULTIPLE_BLOCK : TFC_READ_SINGLE_BLOCK, sector, 0x95, resp, 0)) {
		set_detail_code(0x10);
		goto disk_read_end;
	}

	while (count) {
		if (!nile_spi_rx_copy(resp, 1, NILE_SPI_MODE_WAIT_READ)) {
			set_detail_code(0x11);
			goto disk_read_stop;
		}
		if (resp[0] != 0xFE) {
			set_detail_code(0xE0 | resp[0]);
			goto disk_read_stop;
		}
		if (!nile_spi_rx_copy(buff, 512, NILE_SPI_MODE_READ)) {
			set_detail_code(0x13);
			goto disk_read_stop;
		}
		if (!nile_spi_rx(2, NILE_SPI_MODE_READ)) {
			set_detail_code(0x14);
			goto disk_read_stop;
		}
		buff += 512;
		count--;
	}

disk_read_stop:
	if (multi_transfer) {
		resp[0] = TFC_STOP_TRANSMISSION;
		resp[5] = 0x95;
		resp[6] = 0xFF; // skip one byte
		if (!nile_spi_tx(resp, 7)) {
			set_detail_code(0x15);
			goto disk_read_end;
		}
		if (nile_tf_read_response_r1b()) {
			set_detail_code(0x16);
			goto disk_read_end;
		}
	}
#else
	while (count) {
		if (nile_tf_command(TFC_READ_SINGLE_BLOCK, sector, 0x95, resp, 0)) {
			set_detail_code(0x10);
			goto disk_read_end;
		}

		if (!nile_spi_rx_copy(resp, 1, NILE_SPI_MODE_WAIT_READ)) {
			set_detail_code(0x11);
			goto disk_read_end;
		}
		if (resp[0] != 0xFE) {
			set_detail_code(0xE0 | resp[0]);
			goto disk_read_end;
		}
		if (!nile_spi_rx_copy(buff, 512, NILE_SPI_MODE_READ)) {
			set_detail_code(0x13);
			goto disk_read_end;
		}
		if (!nile_spi_rx(2, NILE_SPI_MODE_READ)) {
			set_detail_code(0x14);
			goto disk_read_end;
		}
		buff += 512;
		sector += (nile_ipl_data->card_state & NILE_CARD_BLOCK_ADDRESSING) ? 1 : 512;
		count--;
	}
#endif

	result = RES_OK;
disk_read_end:
	nile_spi_wait_busy();
	nile_tf_cs_high();
	return result;
}

#if FF_FS_READONLY == 0

DRESULT disk_write (BYTE pdrv, const BYTE __far* buff, LBA_t sector, UINT count) {
	uint8_t result = RES_ERROR;
	uint8_t resp[8];

	if (!(nile_ipl_data->card_state & NILE_CARD_BLOCK_ADDRESSING))
		sector <<= 9;

#ifdef USE_MULTI_TRANSFER_WRITES
	bool multi_transfer = count > 1;
	if (nile_tf_command(multi_transfer ? TFC_WRITE_MULTIPLE_BLOCK : TFC_WRITE_BLOCK, sector, 0x95, resp, 1)) {
		set_detail_code(0x20);
		goto disk_read_end;
	}

	while (count) {
		resp[0] = 0xFF;
		resp[1] = multi_transfer ? 0xFC : 0xFE;
		if (!nile_spi_tx(resp, 2)) {
			set_detail_code(0x21);
			goto disk_read_stop;
		}
		if (!nile_spi_tx(buff, 512)) {
			set_detail_code(0x22);
			goto disk_read_stop;
		}
		if (!nile_spi_rx_copy(resp, 3, NILE_SPI_MODE_WAIT_READ)) {
			set_detail_code(0x23);
			goto disk_read_stop;
		}
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
		if (nile_tf_wait_ready(0x00)) {
			set_detail_code(0x24);
			goto disk_read_end;
		}
	}
#else
	while (count) {
		if (nile_tf_command(TFC_WRITE_BLOCK, sector, 0x95, resp, 1)) {
			set_detail_code(0x20);
			goto disk_read_end;
		}

		resp[0] = 0xFF;
		resp[1] = 0xFE;
		if (!nile_spi_tx(resp, 2)) {
			set_detail_code(0x21);
			goto disk_read_end;
		}
		if (!nile_spi_tx(buff, 512)) {
			set_detail_code(0x22);
			goto disk_read_end;
		}
		if (!nile_spi_rx_copy(resp, 3, NILE_SPI_MODE_WAIT_READ)) {
			set_detail_code(0x23);
			goto disk_read_end;
		}
		// TODO: error handling?
		buff += 512;
		sector += (nile_ipl_data->card_state & NILE_CARD_BLOCK_ADDRESSING) ? 1 : 512;
		count--;
	}
#endif

	result = RES_OK;
disk_read_end:
	nile_spi_wait_busy();
	nile_tf_cs_high();
	return result;
}

#endif

DRESULT disk_ioctl (BYTE pdrv, BYTE cmd, void *buff) {
	if (cmd == CTRL_SYNC)
		return RES_OK;
	return RES_PARERR;
}

#endif
