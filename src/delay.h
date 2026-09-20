/*
 * delay.h
 *
 *  Created on: 19-Sep-2026
 *      Author: ishan
 */


#ifndef DELAY_H_
#define DELAY_H_
#include "S32K142.h"
#include <stdint.h>

#define SYSTICK_CTRL (*(volatile uint32_t*)0xE000E010)
#define SYSTICK_LOAD (*(volatile uint32_t*)0xE000E014)
#define SYSTICK_VAL  (*(volatile uint32_t*)0xE000E018)

void delay(uint32_t ms);


#endif /* DELAY_H_ */
