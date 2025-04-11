#include "console.h"
#include "ops/ieeprom.h"
#include "ops/tf_card.h"
#include "tests/flash_fsm.h"
#include "tests/rtc.h"
#include <nile/core.h>
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

// Reserve 0x2000 bytes of space for BIOS window
__attribute__((section(".rom0_ffff_e000.bios_pad")))
volatile uint8_t bios_pad[0x1FF0] = {0x00};

void console_press_any_key(void) {
	console_print(CONSOLE_FLAG_NO_SERIAL, s_press_any_key_to_continue);
	input_wait_any_key();
}

void main_mfg(void) {
	console_print_header(s_caps_initialization);
	if (!op_mcu_setup_boot_flags()) return;

	console_print_header(s_caps_test_suite);
	if (!test_flash_fsm()) return;
	if (!op_tf_card_test()) return;

	console_print_header(s_caps_information);
	console_print(CONSOLE_FLAG_MONOSPACE, s_mfg_test_success0);
	console_print(CONSOLE_FLAG_MONOSPACE, s_mfg_test_success1);
	console_print(CONSOLE_FLAG_MONOSPACE, s_mfg_test_success2);
	console_print(CONSOLE_FLAG_MONOSPACE, s_mfg_test_success1);
	console_print(CONSOLE_FLAG_MONOSPACE, s_mfg_test_success0);
	console_print_newline();
	if (!op_id_print()) return;
}

bool console_warranty_disclaimer(void) {
	console_print(0, s_warranty_disclaimer);
	input_wait_any_key();
	console_print_newline();
	console_print_newline();
	return (input_pressed & KEY_A);
}

static const char __wf_rom* __wf_rom menu_main[] = {
	s_internal_eeprom_recovery,
	s_tf_card_mgmt,
	s_print_cartridge_ids,
	s_setup_mcu_boot_flags,
	s_cartridge_tests,
	s_run_manufacturing_test,
	s_exit,
	NULL
};

static const char __wf_rom* __wf_rom menu_ieeprom[] = {
	s_disable_custom_splash,
	s_restore_tft_data,
	NULL
};

static const char __wf_rom* __wf_rom menu_cartridge_tests[] = {
	s_flash_fsm_test,
	s_rtc_stability_test,
	NULL
};

static const char __wf_rom* __wf_rom menu_card_mgmt[] = {
	s_tf_card_mount,
	s_benchmark_card_read,
	s_benchmark_card_write,
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

	// FIXME: nile_io_unlock() pulls BOOT0 line high
	nile_io_unlock();
	outportb(IO_NILE_POW_CNT, 0xDD);
	nile_bank_unlock();

	console_init();

	outportw(IO_BANK_2003_RAM, NILE_SEG_RAM_IPC);
	if (*((volatile uint16_t __far*) MK_FP(0x1000, 0x01FE)) == 0x3FA7) {
		main_mfg();

		console_print_newline();
		console_print(CONSOLE_FLAG_NO_SERIAL, s_manual_shutdown);
		cpu_irq_disable();
		while(1) cpu_halt();
	}

	while (true) {
		console_draw_header(s_nileswan_recovery);
		int option = menu_run(menu_main);
		int suboption;
option_loop:
		switch (option) {
		case 0:
			console_clear();
			console_draw_header(s_internal_eeprom_recovery);
			suboption = menu_run(menu_ieeprom);
			switch (suboption) {
			case 0:
				console_clear();
				op_ieeprom_disable_custom_splash();
				console_press_any_key();
				break;
			case 1:
				console_clear();
				op_ieeprom_restore_tft_data();
				console_press_any_key();
				break;
			}
			if (suboption >= 0) goto option_loop; else break;
		case 1:
			console_clear();
			console_draw_header(s_tf_card_mgmt);
			suboption = menu_run(menu_card_mgmt);
			switch (suboption) {
			case 0:
				console_clear();
				op_tf_card_init(true);
				console_press_any_key();
				break;
			case 1:
				console_clear();
				op_tf_card_benchmark_read();
				console_press_any_key();
				break;
			case 2:
				console_clear();
				op_tf_card_benchmark_write();
				console_press_any_key();
				break;
			}
			if (suboption >= 0) goto option_loop; else break;
		case 2:
			console_clear();
			op_id_print();
			console_press_any_key();
			break;
		case 3:
			console_clear();
			op_mcu_setup_boot_flags();
			console_press_any_key();
			break;
		case 4:
			console_clear();
			console_draw_header(s_cartridge_tests);
			suboption = menu_run(menu_cartridge_tests);
			switch (suboption) {
			case 0:
				console_clear();
				test_flash_fsm();
				console_press_any_key();
				break;
			case 1:
				console_clear();
				test_rtc_stability(0);
				console_press_any_key();
				break;
			}
			if (suboption >= 0) goto option_loop; else break;
		case 5:
			console_clear();
			main_mfg();
			console_press_any_key();
			break;
		case 6:
			nile_reboot();
			break;
		}
	}

	while(1);
}
