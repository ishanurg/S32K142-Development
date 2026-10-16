/*
 * delay.c
 *
 *  Created on: 19-Sep-2026
 *      Author: ishan
 */

#include "S32K142.h"
#include "delay.h"

void delay(uint32_t ms) {
    SYSTICK_LOAD = 48000 - 1;
    SYSTICK_VAL = 0;
    SYSTICK_CTRL = 5;

    for (uint32_t i = 0; i < ms; i++) {

        while ((SYSTICK_CTRL & (1 << 16)) == 0);
    }
    SYSTICK_CTRL = 0;
}
