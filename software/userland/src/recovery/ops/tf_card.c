#include "tf_card.h"
#include <nilefs/ff.h>
#include <string.h>
#include <ws.h>
#include <nile.h>
#include <nilefs.h>
#include <ws/hardware.h>
#include "console.h"
#include "strings.h"

FATFS fs;

bool op_tf_card_init(bool force) {
    char blank = 0;
    int result;

    // Already initialized?
    if (fs.fs_type && !force) {
        goto done;
    }

    console_print(0, s_tf_card_init);
    result = f_mount(&fs, &blank, 1);
    console_print_status(result == FR_OK);
    if (result != FR_OK) {
        console_print_newline();
        console_printf(0, s_error_code, result);
    }
    console_print_newline();

    if (result != FR_OK) {
        return false;
    }
done:
    nile_spi_set_control(NILE_SPI_CLOCK_FAST | NILE_SPI_DEV_TF);
    return true;
}

bool op_tf_card_test(void) {
    bool result = op_tf_card_init(true);
    
    // Maybe return information about the card?

    return result;
}

static const char __wf_rom path_tftest_bin[] = "/tftest.bin";

#define TF_TEST_MAX_SIZE 16384
#define TF_TEST_BUFFER ((uint8_t*) 0x5000)

#define TEST_BUFFER_COMPARE 0
#define TEST_BUFFER_WRITE   1
#define TEST_BUFFER_ERASE   2

static bool tf_card_test_buffer(uint8_t mode, uint16_t max_size) {
    for (uint16_t i = 0; i < max_size; i += 256) {
        for (int j = 0; j < 256; j++) {
            uint8_t expected_value = j + (i >> 8);
            if (TEST_BUFFER_WRITE) {
                TF_TEST_BUFFER[i + j] = expected_value;
            } else if (TEST_BUFFER_ERASE) {
                TF_TEST_BUFFER[i + j] = expected_value ^ 0xFF;
            } else if (TF_TEST_BUFFER[i + j] != expected_value) {
                return false;
            }
        }
    }
    return true;
}

static FRESULT tf_card_test_open(FIL *f) {
    FRESULT result;
    strcpy(TF_TEST_BUFFER, path_tftest_bin);

    result = f_open(f, (const char*) TF_TEST_BUFFER, FA_OPEN_ALWAYS | FA_READ | FA_WRITE);
    if (result != FR_OK) return result;
    
    if (f_size(f) != TF_TEST_MAX_SIZE) {
        result = f_lseek(f, 0);
        if (result != FR_OK) return result;
        result = f_truncate(f);
        if (result != FR_OK) return result;

        tf_card_test_buffer(TEST_BUFFER_WRITE, TF_TEST_MAX_SIZE);
        result = f_write(f, TF_TEST_BUFFER, TF_TEST_MAX_SIZE, NULL);
        if (result != FR_OK) return result;
    }

    return FR_OK;
}

static bool tf_card_handle_error(FRESULT result) {
    if (result == FR_OK) return true;

    console_print_status(false);
    console_print_newline();
    console_printf(0, s_error_code, result);
    return false;
}

bool op_tf_card_benchmark_read(void) {
    FIL file;
    uint16_t br;

    console_print_header(s_benchmark_card_read);

    if (!ws_system_color_active()) {
        console_print(0, s_model_unsupported);
        return false;
    }

    if (!op_tf_card_init(true)) return false;
    console_print(0, s_benchmark_preparing_test_file);
    if (!tf_card_handle_error(tf_card_test_open(&file))) return false;
    console_print_status(true);
    console_print_newline();
    console_print_newline();

    for (uint16_t len = 512; len <= TF_TEST_MAX_SIZE; len <<= 1) {
        tf_card_test_buffer(TEST_BUFFER_ERASE, len);
        if (!tf_card_handle_error(f_lseek(&file, 0))) return false;
        console_printf(0, s_benchmark_reading_bytes, len);

        outportw(IO_HBLANK_TIMER, 65535);
        outportw(IO_TIMER_CTRL, HBLANK_TIMER_ENABLE | HBLANK_TIMER_ONESHOT);
        if (!tf_card_handle_error(f_read(&file, TF_TEST_BUFFER, len, &br))) return false;

        uint16_t hblanks = 65535 - inportw(IO_HBLANK_COUNTER);
        outportw(IO_TIMER_CTRL, 0);

        if (!tf_card_test_buffer(TEST_BUFFER_COMPARE, len)) {
            console_print_status(false);
            console_print_newline();
            console_printf(0, s_benchmark_data_read_mismatch);
            return false;
        }

        console_print_status(true);
        uint16_t bytes_msec = len / ((hblanks + 11) / 12);
        // bytes/msec are approximately equal to kbytes/sec
        console_printf(0, s_benchmark_hblanks, hblanks, bytes_msec);
        console_print_newline();
    }

    return true;
}

bool op_tf_card_benchmark_write(void) {
    FIL file;
    uint16_t br;

    console_print_header(s_benchmark_card_write);

    if (!ws_system_color_active()) {
        console_print(0, s_model_unsupported);
        return false;
    }

    if (!op_tf_card_init(true)) return false;
    console_print(0, s_benchmark_preparing_test_file);
    if (!tf_card_handle_error(tf_card_test_open(&file))) return false;
    console_print_status(true);
    console_print_newline();
    console_print_newline();

    tf_card_test_buffer(TEST_BUFFER_WRITE, TF_TEST_MAX_SIZE);
    for (uint16_t len = 512; len <= TF_TEST_MAX_SIZE; len <<= 1) {
        if (!tf_card_handle_error(f_lseek(&file, 0))) return false;
        console_printf(0, s_benchmark_writing_bytes, len);

        outportw(IO_HBLANK_TIMER, 65535);
        outportw(IO_TIMER_CTRL, HBLANK_TIMER_ENABLE | HBLANK_TIMER_ONESHOT);
        if (!tf_card_handle_error(f_write(&file, TF_TEST_BUFFER, len, &br))) return false;

        uint16_t hblanks = 65535 - inportw(IO_HBLANK_COUNTER);
        outportw(IO_TIMER_CTRL, 0);
        
        console_print_status(true);
        uint16_t bytes_msec = len / ((hblanks + 11) / 12);
        // bytes/msec are approximately equal to kbytes/sec
        console_printf(0, s_benchmark_hblanks, hblanks, bytes_msec);
        console_print_newline();
    }

    return true;
}
