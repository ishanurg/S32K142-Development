/*
 * lpi2c.c
 *
 *  Created on: 07-Oct-2026
 *      Author: User
 */


#include "lpi2c.h"
#include "S32K142.h"

void delay_ms(uint32_t ms) {
    for(volatile uint32_t i = 0; i < (ms * 8000); i++) { __asm volatile("nop"); }
}

void delay_us(uint32_t us) {
    for(volatile uint32_t i = 0; i < (us * 8); i++) { __asm volatile("nop"); }
}

void PORT_init(void) {
    IP_PCC->PCCn[PCC_PORTA_INDEX] |= PCC_PCCn_CGC_MASK;
    /* Mux PTA2 (SDA) and PTA3 (SCL) to ALT3 for LPI2C0 */
    IP_PORTA->PCR[2] = PORT_PCR_MUX(3) | PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;
    IP_PORTA->PCR[3] = PORT_PCR_MUX(3) | PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;
}

void LPI2C0_init(void) {
    IP_SCG->FIRCDIV |= SCG_FIRCDIV_FIRCDIV2(1);
    IP_PCC->PCCn[PCC_LPI2C0_INDEX] &= ~PCC_PCCn_CGC_MASK;
    IP_PCC->PCCn[PCC_LPI2C0_INDEX] = PCC_PCCn_PCS(3);
    IP_PCC->PCCn[PCC_LPI2C0_INDEX] |= PCC_PCCn_CGC_MASK;

    IP_LPI2C0->MCR = LPI2C_MCR_RST_MASK;
    IP_LPI2C0->MCR = 0x00000000;
    IP_LPI2C0->MCFGR1 = LPI2C_MCFGR1_PRESCALE(0);
    IP_LPI2C0->MCCR0 = LPI2C_MCCR0_CLKHI(236) | LPI2C_MCCR0_CLKLO(240) |
                       LPI2C_MCCR0_SETHOLD(119) | LPI2C_MCCR0_DATAVD(20);
    IP_LPI2C0->MCR |= LPI2C_MCR_MEN_MASK;
}

bool LPI2C0_transmit(uint16_t saddr, uint8_t *tx_buff, uint32_t tx_num) {
    uint32_t i, timeout;
    IP_LPI2C0->MSR = 0x7F00;
    IP_LPI2C0->MTDR = (4 << 8) | (saddr << 1) | 0x00;
    for (i = 0; i < tx_num; i++) {
        timeout = 100000;
        while(((IP_LPI2C0->MSR & LPI2C_MSR_TDF_MASK) == 0) && --timeout);
        if(!timeout) return false;
        IP_LPI2C0->MTDR = (0 << 8) | tx_buff[i];
    }
    IP_LPI2C0->MTDR = (2 << 8) | 0;
    timeout = 100000;
    while(((IP_LPI2C0->MSR & LPI2C_MSR_SDF_MASK) == 0) && --timeout);
    IP_LPI2C0->MSR |= LPI2C_MSR_SDF_MASK;
    return (timeout > 0);
}

bool LPI2C0_receive(uint16_t saddr, uint8_t *rx_buff, uint32_t rx_num) {
    uint32_t i, timeout;
    IP_LPI2C0->MSR = 0x7F00;
    IP_LPI2C0->MTDR = (4 << 8) | (saddr << 1) | 0x01;
    IP_LPI2C0->MTDR = (1 << 8) | (rx_num - 1);
    for (i = 0; i < rx_num; i++) {
        timeout = 100000;
        while(((IP_LPI2C0->MSR & LPI2C_MSR_RDF_MASK) == 0) && --timeout);
        if(!timeout) return false;
        rx_buff[i] = (uint8_t)(IP_LPI2C0->MRDR & 0xFF);
    }
    IP_LPI2C0->MTDR = (2 << 8) | 0;
    timeout = 100000;
    while(((IP_LPI2C0->MSR & LPI2C_MSR_SDF_MASK) == 0) && --timeout);
    IP_LPI2C0->MSR |= LPI2C_MSR_SDF_MASK;
    return (timeout > 0);
}
