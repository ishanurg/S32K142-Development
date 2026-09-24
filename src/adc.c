/*
 * adc.c
 *
 *  Created on: 24-Sep-2026
 *      Author: User
 */

#include "adc.h"
#include "clock.h"
#include "dma.h"

static uint8_t get_adc_channel(Port_t port, uint8_t pin) {
    if (port == PORT_A) {
        if (pin == 0) return 0;
        if (pin == 1) return 1;
        if (pin == 6) return 2;
        if (pin == 7) return 3;
    }
    else if (port == PORT_B) {
        if (pin == 0) return 4;
        if (pin == 1) return 5;
        if (pin == 2) return 6;
        if (pin == 3) return 7;
        if (pin == 13) return 8;
    }
    else if (port == PORT_C) {
        if (pin == 1) return 9;
        if (pin == 2) return 10;
        if (pin == 3) return 11;
        if (pin == 14) return 12; // PIN_POT[cite: 5]
        if (pin == 15) return 13;
        if (pin == 16) return 14;
        if (pin == 17) return 15;
    }
    return 0xFF;
}

void ADC_Begin(void) {
    // FIXED: Use the central clock library to guarantee safe power delivery
    Clock_EnablePeripheral(PERIPH_ADC0);

    // FIXED: Route to the guaranteed Bus clock (inherited from PCC)
    analogReadClock(ADC_CLK_BUS, ADC_DIV_2);

    analogReadResolution(ADC_RES_12BIT);
    analogReadAveraging(ADC_AVG_4);
    analogReadSampleTime(12);

    // Ensure DMA triggering is globally disabled during boot
    IP_ADC0->SC2 = 0x00000000;
}

void analogReadResolution(ADC_Resolution_t res) {
    uint32_t cfg1 = IP_ADC0->CFG1;
    cfg1 &= ~ADC_CFG1_MODE_MASK;
    cfg1 |= ADC_CFG1_MODE(res);
    IP_ADC0->CFG1 = cfg1;
}

void analogReadClock(ADC_ClockSource_t clk, ADC_ClockDivider_t div) {
    uint32_t cfg1 = IP_ADC0->CFG1;
    cfg1 &= ~(ADC_CFG1_ADICLK_MASK | ADC_CFG1_ADIV_MASK);
    cfg1 |= ADC_CFG1_ADICLK(clk) | ADC_CFG1_ADIV(div);
    IP_ADC0->CFG1 = cfg1;
}

void analogReadAveraging(ADC_Averaging_t avg) {
    if (avg == ADC_AVG_NONE) {
        IP_ADC0->SC3 &= ~ADC_SC3_AVGE_MASK;
    } else {
        IP_ADC0->SC3 |= ADC_SC3_AVGE_MASK;
        uint32_t avgs_val = (avg == ADC_AVG_4) ? 0 : (avg == ADC_AVG_8) ? 1 : (avg == ADC_AVG_16) ? 2 : 3;
        uint32_t sc3 = IP_ADC0->SC3;
        sc3 &= ~ADC_SC3_AVGS_MASK;
        sc3 |= ADC_SC3_AVGS(avgs_val);
        IP_ADC0->SC3 = sc3;
    }
}

void analogReadSampleTime(uint8_t cycles) {
    uint32_t cfg2 = IP_ADC0->CFG2;
    cfg2 &= ~ADC_CFG2_SMPLTS_MASK;
    cfg2 |= ADC_CFG2_SMPLTS(cycles);
    IP_ADC0->CFG2 = cfg2;
}

uint16_t analogRead(Port_t port, uint8_t pin) {
    uint8_t channel = get_adc_channel(port, pin);
    if (channel == 0xFF) return 0xFFFF;

    pinMux(port, pin, 0);
    IP_ADC0->SC1[0] = ADC_SC1_ADCH(channel);

    // FIXED: Added a hardware timeout so the CPU never permanently freezes
    uint32_t timeout = 500000;
    while (((IP_ADC0->SC1[0] & ADC_SC1_COCO_MASK) == 0) && timeout) {
        timeout--;
    }

    // If the timeout hit 0, the ADC hardware is failing. Return max value as an error code.
    if (timeout == 0) return 65535;

    return (uint16_t)(IP_ADC0->R[0]);
}

void analogRead_Start_DMA(Port_t port, uint8_t pin, volatile uint32_t *result_var, uint8_t dma_channel) {
    uint8_t channel = get_adc_channel(port, pin);
    if (channel == 0xFF) return;

    pinMux(port, pin, 0);

    IP_ADC0->SC3 &= ~ADC_SC3_ADCO_MASK;
    IP_ADC0->SC2 &= ~ADC_SC2_DMAEN_MASK;

    // 32-bit DMA transfer matches the physical system bus width of the ADC register
    DMA_ConfigChannel(dma_channel, DMA_REQ_ADC0, (uint32_t)&IP_ADC0->R[0], (uint32_t)result_var, 4, DMA_SIZE_32BIT, 0, 0);

    IP_ADC0->SC2 |= ADC_SC2_DMAEN_MASK;
    IP_ADC0->SC1[0] = ADC_SC1_ADCH(channel);
}
