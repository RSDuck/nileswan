#include <ws.h>
#include "input.h"
#include "main.h"

extern volatile uint16_t vbl_ticks;

uint16_t input_keys = 0;
uint16_t input_keys_repressed = 0;
uint16_t input_keys_released = 0;
uint16_t input_pressed, input_held;

void vblank_input_update(void) {
	uint16_t keys = ws_keypad_scan();
	if (keys && ws_system_get_model() == WS_MODEL_PCV2) {
		// WS:   ....yyyyxxxxbas.
		// PCv2: ....pc1Cre1vud1l
		// remapped:
		//       ...p....urldcCe.
		keys = 
			  ((keys & 0x0800) <<  1) /*p*/
			| ((keys & 0x0400) >>  7) /*c*/
			| ((keys & 0x0100) >>  6) /*C*/
			| ((keys & 0x0080) >>  1) /*r*/
			| ((keys & 0x0040) >>  5) /*e*/
			| ((keys & 0x0008) <<  4) /*u*/
			| ((keys & 0x0004) <<  2) /*d*/
			| ((keys & 0x0001) <<  5);/*l*/
	}
	input_keys |= keys;
	input_keys_repressed |= (keys & input_keys_released);
	input_keys_released |= (input_held & (~keys));
}

void input_reset(void) {
	cpu_irq_disable();
	input_keys = 0;
	input_keys_repressed = 0;
	input_keys_released = 0;
	cpu_irq_enable();
}

#define JOY_REPEAT_DELAY 18
#define JOY_REPEAT_DELAY_NEXT 5

static uint8_t input_vbls_next[11];

void input_update(void) {
	uint16_t keys_pressed;
	uint16_t keys_repressed;
	uint16_t keys_released;

	cpu_irq_disable();
	keys_pressed = input_keys;
	keys_repressed = input_keys_repressed;
	keys_released = input_keys_released;
	cpu_irq_enable();

	input_pressed = 0;
	uint16_t input_mask = 2;
	for (uint8_t i = 0; i < 11; i++, input_mask <<= 1) {
		if (keys_pressed & input_mask) {
			if (keys_repressed & input_mask) {
				goto KeyRepressed;
			} else if (input_held & input_mask) {
				if (keys_released & input_mask) {
					input_held &= ~input_mask;
				}
				if (((uint8_t) (input_vbls_next[i] - vbl_ticks)) < 0x80) continue;
				if (!(keys_released & input_mask)) {
					input_pressed |= input_mask;
					input_vbls_next[i] = vbl_ticks + JOY_REPEAT_DELAY_NEXT;
				}
			} else {
				if (!(keys_released & input_mask)) {
KeyRepressed:
					input_pressed |= input_mask;
					input_held |= input_mask;
					input_vbls_next[i] = vbl_ticks + JOY_REPEAT_DELAY;
				}
			}
			break;
		} else {
			input_held &= ~input_mask;
		}
	}

	input_reset();
}

void input_wait_clear(void) {
	do {
		input_reset();
		wait_for_vblank();
	} while (input_keys != 0);
	input_update();
}

void input_wait_key(uint16_t key) {
	input_wait_clear();
	do {
		input_reset();
		wait_for_vblank();
	} while ((input_keys & key) != key);
	input_update();
}

void input_wait_any_key(void) {
	input_wait_clear();
	do {
		input_reset();
		wait_for_vblank();
	} while (input_keys == 0);
	input_update();
}
