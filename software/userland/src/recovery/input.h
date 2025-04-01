#ifndef _INPUT_H_
#define _INPUT_H_

#include <stdbool.h>
#include <stdint.h>
#include <ws.h>

extern uint16_t input_pressed, input_held;

#define KEY_UP KEY_X1
#define KEY_DOWN KEY_X3
#define KEY_LEFT KEY_X4
#define KEY_RIGHT KEY_X2

#define KEY_AUP KEY_Y1
#define KEY_ADOWN KEY_Y3
#define KEY_ALEFT KEY_Y4
#define KEY_ARIGHT KEY_Y2

void vblank_input_update(void);
void input_reset(void);
void input_update(void);
void input_wait_clear(void);
void input_wait_key(uint16_t key);
void input_wait_any_key(void);

#endif /* _INPUT_H_ */
