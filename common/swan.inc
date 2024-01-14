; obviously inspired by WonderSwan.inc
; http://sebastianmihai.com/swan-driving.html

IRAMSeg         equ 0x0000
SRAMSeg         equ 0x1000
ROMSeg0         equ 0x2000
ROMSeg1         equ 0x3000
ROMSegLinear    equ 0x4000


; graphics registers
LCD_SEG_DATA    equ 0x15

; regular Bandai chip registers
RAM_BANK        equ 0xC1
ROM_BANK_0      equ 0xC2
ROM_BANK_1      equ 0xC3
MEMORY_CTRL     equ 0xCE

MEMORY_ENABLE_SELF_FLASH equ 0x01

; nileswan extension registers
SPI_DATA        equ 0xE0
SPI_CNT         equ 0xE1

SPI_CNT_BUSY    equ 0x01
SPI_CNT_KEEP_CS equ 0x02

BLKMEM_CTRL     equ 0xE2
PSRAM_UPPER_BITS equ 0xE3
PSRAM_BANK_MASK equ 0xE4

BLKMEM_ENABLE_BOOTROM equ 0x01

; Savetypes
SAVETYPE_NONE   equ 0x0

SAVETYPE_SRAM_8KB equ 0x1
SAVETYPE_SRAM_32KB equ 0x2
SAVETYPE_SRAM_128KB equ 0x3
SAVETYPE_SRAM_256KB equ 0x4
SAVETYPE_SRAM_512KB equ 0x5

SAVETYPE_EEPROM_128B equ 0x10
SAVETYPE_SRAM_2KB equ 0x20
SAVETYPE_SRAM_1KB equ 0x50

%macro wsheader 5
    ; %1 - start of last segment
    ; %2 - segment of program start
    ; %3 - address of the program start
    ; %4 - ROM size alignment
    ; %5 - save type
    times	(%4-16)-$+%1 db 0xFF

    jmp %2:%3

    db	0x00
    db	0x42	; Developer ID
    db	0x01    ; Color
    db	0x01	; Cart number
    db	0x00    ; Version
    db	0x00    ; ROM size
    db	%5    ; Save type
    dw	0x0004  ; Flags
    dw	0x0000	; Checksum
%endmacro
