/**
 * Copyright (c) 2024 Adrian Siekierka
 *
 * Nileswan MCU is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * Nileswan MCU is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with Nileswan MCU. If not, see <https://www.gnu.org/licenses/>.
 */

#include "mcu.h"
#include <stm32u073xx.h>
#include <stm32u0xx_ll_rcc.h>
#include <stm32u0xx_ll_rtc.h>
#include "rtc.h"

static uint8_t rtc_curr_cmd;

bool rtc_is_processing_command(void) {
    return rtc_curr_cmd != 0;
}

int rtc_start_command_rx(uint8_t cmd) {
    rtc_curr_cmd = cmd;

    switch (cmd & 0xF) {
    case 2: /* Status */
        return 1;
    case 4: /* Date/Time */
        return 7;
    case 6: /* Time */
        return 3;
    case 8: /* Alarm */
        return 2;
    case 10: /* ?? */
        return 2;
    default:
        return 0;
    }
}

static void rtc_write_start(void) {
    LL_RTC_DisableWriteProtection(RTC);
}

static void rtc_write_end(void) {
    LL_RTC_EnableWriteProtection(RTC);
}

static void rtc_init_start(void) {
    LL_RTC_EnableInitMode(RTC);
    while (!LL_RTC_IsActiveFlag_INIT(RTC));
}

static void rtc_init_end(void) {
    LL_RTC_DisableInitMode(RTC);
}

static inline uint8_t rtc_hour_mcu_to_rtc(uint32_t tr) {
    if (RTC->CR & RTC_CR_FMT) {
        uint8_t result = ((tr >> 16) & 0x3F) | ((tr >> 15) & 0x80);
        // convert 12 -> 00 for 12-hour reads
        if ((result & 0x3F) == 0x12) {
            result &= 0x80;
        }
        return result;
    } else {
        uint8_t result = ((tr >> 16) & 0x3F);
        // set AM/PM bit for 24-hour reads
        if (result >= 0x12) {
            result |= 0x80;
        }
        return result;
    }
}


void RTC_TAMP_IRQHandler(void) {
    LL_RTC_ClearFlag_ALRA(RTC);
    if (TAMP->BKP0R == S3511A_INTAE) {
        uint32_t tr = RTC->TR;
        uint32_t value = (tr & 0x7F00) | rtc_hour_mcu_to_rtc(tr);
        if (!((TAMP->BKP1R ^ value) & 0xBF7F)) {
            mcu_fpga_irq_set();
        } else {
            mcu_fpga_irq_clear();
        }
    } else switch (TAMP->BKP0R & ~S3511A_INTAE) {
    case S3511A_INTFE: {
        uint32_t tr = RTC->TR;
        uint32_t ssr = RTC->SSR;
        uint32_t value = ((ssr ^ 0x7FFF) & 0x7FFF) | ((tr & 0x1) << 15);
        if (TAMP->BKP1R & value) {
            mcu_fpga_irq_set();
        } else {
            mcu_fpga_irq_clear();
        }
    } break;
    case S3511A_INTME: {
        uint32_t tr = RTC->TR;
        uint32_t ssr = RTC->SSR;
        uint32_t value = ((tr & 0x7F) == 0x00) && ((ssr & 0x7FFF) >= (32768 - 327));
        if (value) {
            mcu_fpga_irq_set();
        } else {
            mcu_fpga_irq_clear();
        }
    } break;
    case S3511A_INTME | S3511A_INTFE: {
        uint32_t tr = RTC->TR;
        uint32_t value = (tr & 0x7F) < 0x30;
        if (value) {
            mcu_fpga_irq_set();
        } else {
            mcu_fpga_irq_clear();
        }
    } break;
    default: {
        mcu_fpga_irq_clear();
    } break;
    }
}

static void rtc_apply_alarm_mode(uint32_t alarm_mode) {
    TAMP->BKP0R = alarm_mode;
    RTC->CR &= ~(RTC_CR_ALRAE | RTC_CR_ALRAIE);
    if (alarm_mode) {
        if (TAMP->BKP0R == S3511A_INTAE) {
            // Alarm mode: one IRQ a minute
            RTC->ALRMAR = RTC_ALRMAR_MSK4 | RTC_ALRMAR_MSK3 | RTC_ALRMAR_MSK2;
            RTC->ALRMASSR = 0;
        } else switch (TAMP->BKP0R & ~S3511A_INTAE) {
        case S3511A_INTFE: {
            // Frequency mode: one IRQ every 16384 Hz
            // FIXME: 32768 Hz support.
            // FIXME: Could be power-optimized more by dynamically changing RTC->ALRMAR.
            RTC->ALRMAR = RTC_ALRMAR_MSK4 | RTC_ALRMAR_MSK3 | RTC_ALRMAR_MSK2 | RTC_ALRMAR_MSK1;
            RTC->ALRMASSR = (1 << 24) | 1;
        } break;
        case S3511A_INTME: {
            // Per-minute edge: one IRQ every ~1ms
            // FIXME: Could be power-optimized more by dynamically changing RTC->ALRMAR.
            RTC->ALRMAR = RTC_ALRMAR_MSK4 | RTC_ALRMAR_MSK3 | RTC_ALRMAR_MSK2 | RTC_ALRMAR_MSK1;
            RTC->ALRMASSR = (5 << 24) | 31;
        } break;
        case S3511A_INTME | S3511A_INTFE: {
            // Per-minute steady: one IRQ every second
            // FIXME: Could be power-optimized more by dynamically changing RTC->ALRMAR.
            RTC->ALRMAR = RTC_ALRMAR_MSK4 | RTC_ALRMAR_MSK3 | RTC_ALRMAR_MSK2 | RTC_ALRMAR_MSK1;
            RTC->ALRMASSR = 0;
        } break;
        }
        RTC->CR |= RTC_CR_ALRAE | RTC_CR_ALRAIE;
        RTC_TAMP_IRQHandler();
    } else {
        mcu_fpga_irq_clear();
    }
}

void mcu_rtc_init(void) {
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_RTCAPB);

    LL_RCC_LSI_Enable();
    while (!LL_RCC_LSI_IsReady());

    if (rtc_is_configured()) {
        rtc_write_start();
        rtc_apply_alarm_mode(TAMP->BKP0R);
        rtc_write_end();
    }

    NVIC_SetPriority(RTC_TAMP_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 80, 0));
    NVIC_EnableIRQ(RTC_TAMP_IRQn);
}

bool rtc_is_configured(void) {
    return LL_RCC_GetRTCClockSource() != LL_RCC_RTC_CLKSOURCE_NONE;
}

void rtc_reset(void) {
    LL_RCC_ForceBackupDomainReset();
    LL_RCC_ReleaseBackupDomainReset();

    LL_RCC_SetRTCClockSource(LL_RCC_LSI_IsReady() ? LL_RCC_RTC_CLKSOURCE_LSI : LL_RCC_RTC_CLKSOURCE_LSE);
    LL_RCC_EnableRTC();

    rtc_write_start();
    rtc_init_start();
    RTC->CR = RTC_CR_FMT | RTC_CR_BYPSHAD;
    RTC->CALR |= RTC_CALR_LPCAL;
    RTC->DR = 0x00E101;
    RTC->TR = 0x120000;
    RTC->PRER = 0x007FFF;
    TAMP->BKP1R = 0;
    rtc_apply_alarm_mode(0);
    rtc_init_end();
    rtc_write_end();
}

void rtc_write_status(uint8_t value) {
    uint32_t cr = RTC->CR;

    uint32_t old_cr = cr;
    cr = (cr & ~RTC_CR_FMT) | ((value & S3511A_1224) ? 0 : RTC_CR_FMT);

    uint32_t alarm_mode = (value & (S3511A_INTAE | S3511A_INTFE | S3511A_INTME));
    bool alarm_mode_changed = TAMP->BKP0R != alarm_mode;
    bool init_required = (old_cr & 0xFF) != (cr & 0xFF);

    rtc_write_start();
    if (init_required) rtc_init_start();
    RTC->CR = cr;
    uint32_t tr = RTC->TR;

    // Perform 12<->24-hour time conversion
    if ((old_cr & RTC_CR_FMT) && !(cr & RTC_CR_FMT)) {
        // 12-hour -> 24-hour 
        if ((tr & 0x3F0000) == 0x120000) {
            // 12:00 AM, 12:00 PM
            tr &= ~0x3F0000;
            if (tr & RTC_TR_PM) {
                tr |= 0x120000;
            }
        } else if (tr & RTC_TR_PM) {
            // 1:00 - 11:00 PM
            if ((tr & 0x3F0000) == 0x080000 || (tr & 0x3F0000) == 0x090000) {
                tr += 0x180000;
            } else {
                tr += 0x120000;
            }
        }
        tr &= ~RTC_TR_PM;
        RTC->TR = tr;
    } else if (!(old_cr & RTC_CR_FMT) && (cr & RTC_CR_FMT)) {
        // 24-hour -> 12-hour
        tr &= ~RTC_TR_PM;
        if ((tr & 0x3F0000) == 0x000000) {
            // 0:00
            tr |= 0x120000;
        } else if ((tr & 0x3F0000) == 0x120000) {
            // 12:00
            tr |= RTC_TR_PM;
        } else if ((tr & 0x3F0000) > 0x120000) {
            // 13:00 - 23:00
            if ((tr & 0x3F0000) == 0x200000 || (tr & 0x3F0000) == 0x210000) {
                tr -= 0x180000;
            } else {
                tr -= 0x120000;
            }
            tr |= RTC_TR_PM;
        }
        RTC->TR = tr;
    }

    if (alarm_mode_changed) {
        rtc_apply_alarm_mode(alarm_mode);
    }

    if (init_required) rtc_init_end();
    rtc_write_end();

}

uint8_t rtc_read_status(void) {
    uint8_t result = 0x00;
    if (!rtc_is_configured()) {
        rtc_reset();
        result |= S3511A_POWER_LOST;
    }
    
    uint32_t cr = RTC->CR;
    return result
        | ((cr & RTC_CR_FMT) ? 0 : S3511A_1224)
        | (TAMP->BKP0R & (S3511A_INTAE | S3511A_INTFE | S3511A_INTME));
}

void rtc_write_datetime(uint8_t *buffer, bool date) {
    rtc_write_start();
    rtc_init_start();

    if (date) {
        uint32_t dr = 0;
        uint8_t dow = (buffer[3] & 7);
        dr |= buffer[0] << 16;
        dr |= (buffer[1] & 0x1F) << 8;
        dr |= buffer[2] & 0x3F;
        dr |= (dow == 0 ? 7 : dow) << 13;
        RTC->DR = dr;
        buffer += 4;
    }

    if (RTC->CR & RTC_CR_FMT) {
        // convert 00 -> 12 for 12-hour writes
        if ((buffer[0] & 0x1F) == 0x00) {
            buffer[0] |= 0x12;
        } else if ((buffer[0] & 0x1F) >= 0x12 || (buffer[0] & 0x0F) > 0x9) {
            buffer[0] &= ~0x1F;
        }
    } else {
        if ((buffer[0] & 0x3F) >= 0x24 || (buffer[0] & 0x0F) > 0x9) {
            buffer[0] &= ~0x3F;
        }
    }
    if ((buffer[1] & 0x7F) >= 0x60 || (buffer[1] & 0x0F) > 0x9) {
        buffer[1] = 0x00;
    }
    // FIXME: The S-3511A will allow a seconds value between 0x60 .. 0x7F
    // (or one with the last digit between 0xA .. 0xF) for one second,
    // then skip to the next value. The MCU RTC will, instead, happily
    // tick all the way to 0x7F. Setting 0x59 means the value becomes
    // correct for these edge cases after one second.
    if ((buffer[2] & 0x7F) >= 0x60 || (buffer[2] & 0x0F) > 0x9) {
        buffer[2] = 0x59;
    }
    
    uint32_t tr = 0;
    tr |= (buffer[0] & 0x3F) << 16;
    tr |= (buffer[0] & 0x80) << 15;
    tr |= (buffer[1] & 0x7F) << 8;
    tr |= (buffer[2] & 0x7F);
    RTC->TR = tr;

    RTC_TAMP_IRQHandler();
    rtc_init_end();
    rtc_write_end();
}

void rtc_read_datetime(uint8_t *buffer, bool date) {
    volatile uint32_t tr = RTC->TR, tr2;
    volatile uint32_t dr = RTC->DR, dr2;

    do {
        tr2 = tr;
        dr2 = dr;
        tr = RTC->TR;
        dr = RTC->DR;
    } while  (tr != tr2 || dr != dr2);

    if (date) {
        uint8_t dow = (dr >> 13) & 0x7;
        buffer[0] = dr >> 16;
        buffer[1] = (dr >> 8) & 0x1F;
        buffer[2] = dr & 0x3F;
        buffer[3] = (dow == 7 ? 0 : dow);
        buffer += 4;
    }
    buffer[0] = rtc_hour_mcu_to_rtc(tr);
    buffer[1] = (tr >> 8) & 0x7F;
    buffer[2] = tr & 0x7F;
}

static inline void rtc_write_alarm(uint32_t value) {
    rtc_write_start();
    TAMP->BKP1R = value;
    RTC_TAMP_IRQHandler();
    rtc_write_end();
}

int rtc_finish_command_rx(uint8_t *rx, uint8_t *tx) {
    uint8_t cmd = rtc_curr_cmd;
    rtc_curr_cmd = 0;
    switch (cmd & 0xF) {
    case 0:
    case 1: /* Reset */
        rtc_reset();
        return 0;
    case 2: /* Status */
        rtc_write_status(rx[0]);
        return 0;
    case 3:
        tx[0] = rtc_read_status();
        return 1;
    case 4: /* Date/Time */
        rtc_write_datetime(rx, true);
        return 0;
    case 5:
        rtc_read_datetime(tx, true);
        return 7;
    case 6: /* Time */
        rtc_write_datetime(rx, false);
        return 0;
    case 7:
        rtc_read_datetime(tx, false);
        return 3;
    case 8: /* Alarm */
        rtc_write_alarm(*((uint16_t*) rx));
        return 0;
    case 9:
        *((uint16_t*) tx) = 0xFFFF;
        rtc_write_alarm(*((uint16_t*) tx));
        return 2;
    case 10: /* ?? */
        return 0;
    case 11:
        *((uint16_t*) tx) = 0xFFFF;
        return 2;
    default:
        return 0;
    }
    return 0;
}
