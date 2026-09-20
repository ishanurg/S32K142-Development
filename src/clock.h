/*
 * clock.h
 *
 *  Created on: 19-Sep-2026
 *      Author: ishan
 */

#ifndef CLOCK_H_
#define CLOCK_H_

#include "S32K142.h"
#include <stdint.h>

#include "gpio.h"


typedef enum {

    SYS_CLK_48MHZ_FIRC,


    SYS_CLK_80MHZ_SPLL
} SystemClockSpeed_t;


typedef enum {
    PERIPH_LPUART0,
    PERIPH_LPUART1,
    PERIPH_LPSPI0,
    PERIPH_LPSPI1,
    PERIPH_LPI2C0,
    PERIPH_ADC0,
    PERIPH_ADC1,
    PERIPH_FTM0,
    PERIPH_FTM1,
    PERIPH_FTM2,
    PERIPH_FTM3,
    PERIPH_FLEXCAN0,
    PERIPH_FLEXCAN1
} Peripheral_t;



void Clock_InitSystem(SystemClockSpeed_t speed);


void Clock_EnablePort(Port_t port);


void Clock_EnablePeripheral(Peripheral_t peri);


uint32_t Clock_GetSystemFreq(void);

#endif /* CLOCK_H_ */
