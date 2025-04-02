#include "flash_fsm.h"
#include <nile.h>
#include <nile/hardware.h>
#include <ws/hardware.h>
#include "console.h"
#include "strings.h"

#define SRAM_BYTE ((volatile uint8_t __far*) MK_FP(0x1000, 0x55AA))

bool test_flash_fsm(void) {
    bool result = false;
    console_print_header(s_flash_fsm_test);

    uint8_t prev_sys_ctrl2 = inportb(IO_SYSTEM_CTRL2);
    outportb(IO_SYSTEM_CTRL2, prev_sys_ctrl2 & ~SYSTEM_CTRL2_SRAM_WAIT);
    outportb(IO_CART_FLASH, CART_FLASH_ENABLE);
    outportw(IO_BANK_2003_RAM, 0x10);

    *SRAM_BYTE = 0x11;

    outportb(IO_NILE_EMU_CNT, NILE_EMU_FLASH_FSM);

    // Test #1: Simple write (blocked)
    {
        console_printf(0, s_flash_fsm_test_no, 1);
        *SRAM_BYTE = 0x00;
        if (!console_print_status(*SRAM_BYTE == 0x11)) goto done;
        console_print_newline();    
    }

    // Test #2: Slow flash
    {
        console_printf(0, s_flash_fsm_test_no, 2);
        *SRAM_BYTE = 0xAA;
        *SRAM_BYTE = 0x55;
        *SRAM_BYTE = 0xA0;
        *SRAM_BYTE = 0x22;
        if (!console_print_status(*SRAM_BYTE == 0x22)) goto done;
        console_print_newline();    
    }

    // Test #3: Fast flash
    {
        console_printf(0, s_flash_fsm_test_no, 3);
        *SRAM_BYTE = 0xAA;
        *SRAM_BYTE = 0x55;
        *SRAM_BYTE = 0x20;
        *SRAM_BYTE = 0xA0;
        *SRAM_BYTE = 0x33;
        *SRAM_BYTE = 0x90;
        if (!console_print_status(*SRAM_BYTE == 0x33)) goto done;
        console_print_newline();    
    }

    // Test #4, #5, #6: "Erase"
    {
        console_printf(0, s_flash_fsm_test_no, 4);
        *SRAM_BYTE = 0xAA;
        *SRAM_BYTE = 0x55;
        *SRAM_BYTE = 0x80;
        *SRAM_BYTE = 0xAA;
        *SRAM_BYTE = 0x55;
        *SRAM_BYTE = 0x30;
        if (!console_print_status(*SRAM_BYTE == 0xFF)) goto done;
        console_print_newline();    
        console_printf(0, s_flash_fsm_test_no, 5);
        *SRAM_BYTE = 0xAA;
        *SRAM_BYTE = 0x55;
        *SRAM_BYTE = 0x80;
        *SRAM_BYTE = 0xAA;
        *SRAM_BYTE = 0x55;
        *SRAM_BYTE = 0x10;
        if (!console_print_status(*SRAM_BYTE == 0xFF)) goto done;
        console_print_newline();     
        console_printf(0, s_flash_fsm_test_no, 6);
        *SRAM_BYTE = 0xAA;
        *SRAM_BYTE = 0x55;
        *SRAM_BYTE = 0x90; 
        if (!console_print_status(*SRAM_BYTE == 0x33)) goto done;
        console_print_newline();     
    }

    result = true;
done:
    if (!result) {
        console_printf(0, s_flash_fsm_last_byte, *SRAM_BYTE);
    }
    outportb(IO_CART_FLASH, CART_FLASH_DISABLE);
    outportb(IO_NILE_EMU_CNT, 0);
    outportb(IO_SYSTEM_CTRL2, prev_sys_ctrl2);

    return result;
}
