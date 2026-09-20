/*
 * clock.c
 *
 *  Created on: 19-Sep-2026
 *      Author: ishn
 */

#include "clock.h"


static uint32_t current_system_freq = 48000000;


void Clock_InitSystem(SystemClockSpeed_t speed) {
    if (speed == SYS_CLK_48MHZ_FIRC) {
        // 1. Ensure Fast Internal Reference Clock (FIRC) is enabled
        IP_SCG->FIRCCSR |= SCG_FIRCCSR_FIRCEN_MASK;

        // 2. Wait for FIRC to be valid
        while(!(IP_SCG->FIRCCSR & SCG_FIRCCSR_FIRCVLD_MASK));

        // 3. Set FIRC Divider 2 to 'Divide by 1' (48 MHz) for Peripherals
        // 0b001 = Divide by 1
        IP_SCG->FIRCDIV = SCG_FIRCDIV_FIRCDIV2(1);

        // 4. Switch the entire System Clock to use FIRC (48 MHz)
        // SCS=3 (FIRC), DIVCORE=0 (Div1=48MHz), DIVBUS=1 (Div2=24MHz), DIVSLOW=1 (Div2=24MHz)
        IP_SCG->RCCR = SCG_RCCR_SCS(3) | SCG_RCCR_DIVCORE(0) | SCG_RCCR_DIVBUS(1) | SCG_RCCR_DIVSLOW(1);

        // 5. Wait for the clock to actually switch over
        while(((IP_SCG->CSR & SCG_CSR_SCS_MASK) >> SCG_CSR_SCS_SHIFT) != 3);

        current_system_freq = 48000000;
    }
    else if (speed == SYS_CLK_80MHZ_SPLL) {

        Clock_InitSystem(SYS_CLK_48MHZ_FIRC);
    }
}


void Clock_EnablePort(Port_t port) {
    // Ports do not have a clock source selection (PCS), only an ON/OFF switch (CGC)
    switch(port) {
        case PORT_A: IP_PCC->PCCn[PCC_PORTA_INDEX] |= PCC_PCCn_CGC_MASK; break;
        case PORT_B: IP_PCC->PCCn[PCC_PORTB_INDEX] |= PCC_PCCn_CGC_MASK; break;
        case PORT_C: IP_PCC->PCCn[PCC_PORTC_INDEX] |= PCC_PCCn_CGC_MASK; break;
        case PORT_D: IP_PCC->PCCn[PCC_PORTD_INDEX] |= PCC_PCCn_CGC_MASK; break;
        case PORT_E: IP_PCC->PCCn[PCC_PORTE_INDEX] |= PCC_PCCn_CGC_MASK; break;
    }
}


void Clock_EnablePeripheral(Peripheral_t peri) {
    uint32_t pcc_index = 0;
    uint8_t requires_pcs = 1;


    switch(peri) {
        case PERIPH_LPUART0: pcc_index = PCC_LPUART0_INDEX; break;
        case PERIPH_LPUART1: pcc_index = PCC_LPUART1_INDEX; break;
        case PERIPH_LPSPI0:  pcc_index = PCC_LPSPI0_INDEX; break;
        case PERIPH_LPSPI1:  pcc_index = PCC_LPSPI1_INDEX; break;
        case PERIPH_LPI2C0:  pcc_index = PCC_LPI2C0_INDEX; break;
        case PERIPH_ADC0:    pcc_index = PCC_ADC0_INDEX; break;
        case PERIPH_ADC1:    pcc_index = PCC_ADC1_INDEX; break;
        case PERIPH_FTM0:    pcc_index = PCC_FTM0_INDEX; break;
        case PERIPH_FTM1:    pcc_index = PCC_FTM1_INDEX; break;
        case PERIPH_FTM2:    pcc_index = PCC_FTM2_INDEX; break;
        case PERIPH_FTM3:    pcc_index = PCC_FTM3_INDEX; break;
        case PERIPH_FLEXCAN0:pcc_index = PCC_FlexCAN0_INDEX; requires_pcs = 0; break;
        case PERIPH_FLEXCAN1:pcc_index = PCC_FlexCAN1_INDEX; requires_pcs = 0; break;
        default: return;
    }


    if (requires_pcs) {

        IP_PCC->PCCn[pcc_index] &= ~PCC_PCCn_CGC_MASK;


        IP_PCC->PCCn[pcc_index] = PCC_PCCn_PCS(3) | PCC_PCCn_CGC_MASK;
    } else {

        IP_PCC->PCCn[pcc_index] |= PCC_PCCn_CGC_MASK;
    }
}


uint32_t Clock_GetSystemFreq(void) {
    return current_system_freq;
}
