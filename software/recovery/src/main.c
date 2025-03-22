#include "console.h"
#include <string.h>
#include <wonderful.h>
#include <ws.h>
#include <ws/display.h>
#include <ws/hardware.h>

#define IRAM_IMPLEMENTATION
#include "iram.h"

#include "strings.h"
#include "vwf8.h"

// Reserve 0x2000 bytes of space for BIOS window
__attribute__((section(".rom0_ffff_e000.bios_pad")))
volatile uint8_t bios_pad[0x1FF0] = {0x00};

void main(void) {
	// FIXME: bios_pad[0] is used here solely to create a strong memory reference
	// to dodge elf2rom's limited section garbage collector
	outportb(IO_INT_NMI_CTRL, bios_pad[0] & 0x00);

	console_init();

	console_print_header(s_nileswan_recovery);

	while(1);
}
