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
#include <stm32u0xx_ll_rcc.h>
#include "rtc.h"

static uint8_t rtc_curr_cmd;

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

void mcu_rtc_init(void) {
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_RTCAPB);

    LL_RCC_LSI_Enable();
    while (!LL_RCC_LSI_IsReady());

    if (LL_RCC_GetRTCClockSource() == LL_RCC_RTC_CLKSOURCE_NONE) {
        rtc_reset();
    }

    // Enable RTC alarm interrupt
    LL_EXTI_EnableRisingTrig_0_31(LL_EXTI_LINE_17);
    LL_EXTI_EnableFallingTrig_0_31(LL_EXTI_LINE_17);
    LL_EXTI_EnableIT_0_31(LL_EXTI_LINE_17);
    NVIC_SetPriority(RTC_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 0, 0));
    NVIC_EnableIRQ(RTC_IRQn);
}

bool rtc_is_configured(void) {
    return LL_RCC_GetRTCClockSource() != LL_RCC_RTC_CLKSOURCE_NONE;
}

void rtc_reset(void) {
    LL_RCC_ForceBackupDomainReset();
    LL_RCC_ReleaseBackupDomainReset();

    LL_RCC_SetRTCClockSource(LL_RCC_RTC_CLKSOURCE_LSI);
    LL_RCC_EnableRTC();

    rtc_write_start();
    rtc_init_start();
    RTC->CR = RTC_CR_FMT;
    RTC->ALRMAR = RTC_ALRMAR_MSK1 | RTC_ALRMAR_MSK4;
    rtc_init_end();
    rtc_write_end();
}

void rtc_write_status(uint8_t value) {
    uint32_t cr = RTC->CR;
    uint32_t old_cr = cr;
    cr = (cr & ~(RTC_CR_FMT | RTC_CR_ALRAE | RTC_CR_ALRAIE)) | ((value & S3511A_1224) ? 0 : RTC_CR_FMT);
    if ((value & (S3511A_INTAE | S3511A_INTFE | S3511A_INTME)) == S3511A_INTAE) {
        // Alarm mode
        cr |= RTC_CR_ALRAE | RTC_CR_ALRAIE;
    } else {
        // TODO: INTFE, INTME
    }

    bool init_required = (old_cr & 0xFF) != (cr & 0xFF);

    rtc_write_start();
    if (init_required) rtc_init_start();
    RTC->CR = cr;
    if (init_required) rtc_init_end();
    rtc_write_end();
}

uint8_t rtc_read_status(void) {
    uint8_t result = 0x00;
    if (!rtc_is_configured()) {
        rtc_reset();
        result |= S3511A_POWER_LOST;
    }

    // TODO: INTME, INTFE
    
    uint32_t cr = RTC->CR;
    return result
        | ((cr & RTC_CR_FMT) ? 0 : S3511A_1224)
        | ((cr & RTC_CR_ALRAE) ? S3511A_INTAE : 0);
}

void rtc_write_datetime(const uint8_t *buffer, bool date) {
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
    uint32_t tr = 0;
    tr |= (buffer[4] & 0x1F) << 16;
    tr |= (buffer[4] & 0x80) << 15;
    tr |= (buffer[5] & 0x7F) << 8;
    tr |= (buffer[6] & 0x7F);
    RTC->TR = tr;

    rtc_init_end();
    rtc_write_end();
}

void rtc_read_datetime(uint8_t *buffer, bool date) {
    uint32_t tr = RTC->TR;
    if (date) {
        uint32_t dr = RTC->DR;
        uint8_t dow = (dr >> 13) & 0x7;
        buffer[0] = dr >> 16;
        buffer[1] = (dr >> 8) & 0x1F;
        buffer[2] = dr;
        buffer[3] = (dow == 7 ? 0 : dow);
        buffer += 4;
    }
    buffer[4] = ((tr >> 16) & 0x3F) | ((tr >> 15) & 0x80);
    buffer[5] = tr >> 8;
    buffer[6] = tr;
}

void rtc_write_alarm(uint8_t hour, uint8_t minute) {
    rtc_write_start();
    rtc_init_start();

    // TODO: Binary mode (U0)
    RTC->ALRMAR = RTC_ALRMAR_MSK1 | RTC_ALRMAR_MSK4
        | ((hour & 0x3F) << 16) | ((hour & 0x80) << 15)
        | ((minute & 0x7F) << 8);

    rtc_init_end();
    rtc_write_end();
}

int rtc_finish_command_rx(uint8_t *rx, uint8_t *tx) {
    switch (rtc_curr_cmd & 0xF) {
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
        rtc_write_alarm(rx[0], rx[1]);
        return 0;
    case 9:
        tx[0] = 0xFF;
        tx[1] = 0xFF;
        rtc_write_alarm(tx[0], tx[1]);
        return 2;
    case 10: /* ?? */
        return 0;
    case 11:
        tx[0] = 0xFF;
        tx[1] = 0xFF;
        return 2;
    default:
        return 0;
    }
    return 0;
}
