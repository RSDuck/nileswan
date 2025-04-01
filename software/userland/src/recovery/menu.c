#include "console.h"
#include "input.h"
#include "iram.h"
#include "main.h"
#include "menu.h"

int menu_run(const char __far* __far* options) {
    console_clear();

    int option_count = 0;
    for (const char __far * __far * option = options; *option != NULL; option++) {
        console_draw(1, option_count, 0, *option);
        option_count++;
    }

    int selected_option = 0;
    int prev_selected_option = -1;
    while (true) {
        if (selected_option != prev_selected_option) {
            if (prev_selected_option >= 0) {
                ws_screen_modify_tiles(screen_1, ~SCR_ENTRY_PALETTE_MASK, SCR_ENTRY_PALETTE(0), 0, prev_selected_option, 28, 1);
            }
            ws_screen_modify_tiles(screen_1, ~SCR_ENTRY_PALETTE_MASK, SCR_ENTRY_PALETTE(2), 0, selected_option, 28, 1);
            prev_selected_option = selected_option;
        }

        wait_for_vblank();
        input_update();
        if (input_pressed & KEY_UP) {
            if (selected_option > 0) {
                selected_option--;
            }
        }
        if (input_pressed & KEY_DOWN) {
            if (selected_option < (option_count - 1)) {
                selected_option++;
            }
        }
        if (input_pressed & KEY_A) {
            break;
        }
        if (input_pressed & KEY_B) {
            selected_option = -1;
            break;
        }
    }

    ws_screen_modify_tiles(screen_1, ~SCR_ENTRY_PALETTE_MASK, SCR_ENTRY_PALETTE(0), 0, prev_selected_option, 28, 1);
    return selected_option;
}
