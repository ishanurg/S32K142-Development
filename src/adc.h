/*
 * adc.h
 *
 *  Created on: 24-Sep-2026
 *      Author: User
 */

#ifndef ADC_H
#define ADC_H

#include "S32K142.h"
#include "gpio.h"
#include <stdint.h>
#include <stdbool.h>

#define PIN_A0   PORT_B, 0
#define PIN_A1   PORT_B, 1
#define PIN_A2   PORT_B, 2
#define PIN_A3   PORT_B, 3
#define PIN_A4   PORT_C, 1
#define PIN_A5   PORT_C, 2
#define PIN_POT  PORT_C, 14  // EVB On-Board Potentiometer

typedef enum { ADC_RES_8BIT = 0, ADC_RES_12BIT = 1, ADC_RES_10BIT = 2 } ADC_Resolution_t;
typedef enum { ADC_AVG_NONE = 0, ADC_AVG_4 = 4, ADC_AVG_8 = 8, ADC_AVG_16 = 16, ADC_AVG_32 = 32 } ADC_Averaging_t;
typedef enum { ADC_CLK_BUS = 0, ADC_CLK_ALT2 = 1, ADC_CLK_ALT3 = 2, ADC_CLK_ALT4 = 3 } ADC_ClockSource_t;
typedef enum { ADC_DIV_1 = 0, ADC_DIV_2 = 1, ADC_DIV_4 = 2, ADC_DIV_8 = 3 } ADC_ClockDivider_t;

void ADC_Begin(void);
void analogReadResolution(ADC_Resolution_t res);
void analogReadAveraging(ADC_Averaging_t avg);
void analogReadSampleTime(uint8_t cycles);
void analogReadClock(ADC_ClockSource_t clk, ADC_ClockDivider_t div);

uint16_t analogRead(Port_t port, uint8_t pin);

// UPDATED: Now uses a volatile uint32_t pointer for full register extraction
void analogRead_Start_DMA(Port_t port, uint8_t pin, volatile uint32_t *result_var, uint8_t dma_channel);

#endif /* ADC_H */
