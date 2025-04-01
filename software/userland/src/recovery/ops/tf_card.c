#include "tf_card.h"
#include <ws.h>
#include <nile.h>
#include <nilefs.h>
#include "console.h"
#include "strings.h"

FATFS fs;

bool op_tf_card_init(bool force) {
    char blank = 0;
    int result;

    // Already initialized?
    if (fs.fs_type && !force) {
        return true;
    }

    console_print(0, s_tf_card_init);
    result = f_mount(&fs, &blank, 1);
    console_print_status(result == FR_OK);
    if (result != FR_OK) {
        console_print_newline();
        console_printf(0, s_error_code, result);
    }
    console_print_newline();

    return result == FR_OK;
}

bool op_tf_card_test(void) {
    bool result = op_tf_card_init(true);
    
    // Maybe return information about the card?

    return result;
}
