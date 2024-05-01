; obviously inspired by WonderSwan.inc
; http://sebastianmihai.com/swan-driving.html

IRAMSeg         equ 0x0000
SRAMSeg         equ 0x1000
ROMSeg0         equ 0x2000
ROMSeg1         equ 0x3000
ROMSegLinear    equ 0x4000

; WonderSwan hardware registers

DISPLAY_CTRL    equ 0x00
LCD_SEG         equ 0x15

; WonderSwan key mapping for keypadScan

KEY_Y4          equ 0x0800
KEY_Y3          equ 0x0400
KEY_Y2          equ 0x0200
KEY_Y1          equ 0x0100
KEY_X4          equ 0x0080
KEY_X3          equ 0x0040
KEY_X2          equ 0x0020
KEY_X1          equ 0x0010
KEY_B           equ 0x0008
KEY_A           equ 0x0004
KEY_START       equ 0x0002

; Bandai mapper registers

RAM_BANK        equ 0xC1
ROM_BANK_0      equ 0xC2
ROM_BANK_1      equ 0xC3
MEMORY_CTRL     equ 0xCE
RAM_BANK_2003	equ 0xD0
ROM_BANK_0_2003	equ 0xD2
ROM_BANK_1_2003	equ 0xD4

MEMORY_ENABLE_SELF_FLASH equ 0x01

; nileswan extension registers

NILE_SPI_CNT	equ 0xE0
SPI_MODE_WRITE  equ 0x0000
SPI_MODE_READ   equ 0x0200
SPI_MODE_EXCH   equ 0x0400
SPI_MODE_WAIT_READ equ 0x0600
SPI_CNT_SLOW	equ 0x0800
SPI_CNT_DEV_FLASH equ 0x2000
SPI_CNT_BUFFER  equ 0x4000
SPI_CNT_BUSY    equ 0x8000
SPI_CNT_KEEP_CS equ 0x02

NILE_BANK_MASK	equ 0xE4

NILE_BANK_RAM_TX equ 15
NILE_BANK_ROM_RX equ 510
