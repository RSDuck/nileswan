#include "console.h"
#include <string.h>
#include <wonderful.h>
#include <ws.h>
#include <nile.h>

#define IRAM_IMPLEMENTATION
#include "iram.h"

#include "input.h"
#include "menu.h"
#include "strings.h"
#include "vwf8.h"

#include "ops/id_print.h"
#include "ops/mcu_setup.h"

volatile uint16_t vbl_ticks;

__attribute__((assume_ss_data, interrupt))
void __far vblank_int_handler(void) {
        ws_hwint_ack(HWINT_VBLANK);
        vbl_ticks++;
        vblank_input_update();
}

void wait_for_vblank(void) {
        uint16_t vbl_ticks_last = vbl_ticks;

        while (vbl_ticks == vbl_ticks_last) {
                cpu_halt();
        }
}

// Reserve 0x2000 bytes of space for BIOS window
__attribute__((section(".rom0_ffff_e000.bios_pad")))
volatile uint8_t bios_pad[0x1FF0] = {0x00};

void console_press_any_key(void) {
	console_print(CONSOLE_FLAG_NO_SERIAL, s_press_any_key_to_continue);
	input_wait_any_key();
}

static const char __wf_rom* __wf_rom menu_main[] = {
	s_setup_mcu_boot_flags,
	s_print_cartridge_ids,
	NULL
};

void main(void) {
	// FIXME: bios_pad[0] is used here solely to create a strong memory reference
	// to dodge elf2rom's limited section garbage collector
	outportb(IO_INT_NMI_CTRL, bios_pad[0] & 0x00);

	cpu_irq_disable();
	ws_hwint_set_handler(HWINT_IDX_VBLANK, vblank_int_handler);
	ws_hwint_enable(HWINT_VBLANK);
	cpu_irq_enable();

	console_init();

	while (true) {
		console_draw_header(s_nileswan_recovery);
		switch (menu_run(menu_main)) {
		case 0:
			console_clear();
			op_mcu_setup_boot_flags();
			console_press_any_key();
			break;
		case 1:
			console_clear();
			op_id_print();
			console_press_any_key();
			break;
		}
	}

	while(1);
}
