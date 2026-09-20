/*
 * spi.c
 *
 *  Created on: 20-Sep-2026
 *      Author: ishan
 */


#include "spi.h"
#include "clock.h"
#include "gpio.h"

void SPI_Begin(SPI_Module_t spi_num, SPI_Speed_t speed) {
    LPSPI_Type *spi_reg;

    // -------------------------------------------------------------
    // 1. Route Pins and Enable Clocks
    // -------------------------------------------------------------
    if (spi_num == SPI_0) {
        spi_reg = IP_LPSPI0;

        Clock_EnablePort(PORT_B);
        Clock_EnablePeripheral(PERIPH_LPSPI0);

        // CORRECTED: Route LPSPI0 to PTB2, PTB3, PTB4, PTB5 (ALT 3)
        pinMux(PORT_B, 2, 3); // SCK
        pinMux(PORT_B, 3, 3); // SIN (MISO)
        pinMux(PORT_B, 4, 3); // SOUT (MOSI)
        pinMux(PORT_B, 5, 3); // PCS0 (CS)
    }
    else {
        spi_reg = IP_LPSPI1;

        Clock_EnablePort(PORT_B);
        Clock_EnablePeripheral(PERIPH_LPSPI1);

        // CORRECTED: Route LPSPI1 to PTB14, PTB15, PTB16, PTB17 (ALT 3)
        pinMux(PORT_B, 14, 3); // SCK
        pinMux(PORT_B, 15, 3); // SIN (MISO)
        pinMux(PORT_B, 16, 3); // SOUT (MOSI)
        pinMux(PORT_B, 17, 3); // PCS (CS)
    }

    // -------------------------------------------------------------
    // 2. Configure the LPSPI Module
    // -------------------------------------------------------------

    // Disable module to modify registers
    spi_reg->CR &= ~LPSPI_CR_MEN_MASK;

    // Reset module
    spi_reg->CR |= LPSPI_CR_RST_MASK;
    spi_reg->CR &= ~LPSPI_CR_RST_MASK;

    // Set Master Mode
    spi_reg->CFGR1 = LPSPI_CFGR1_MASTER(1);

    // Set Clock Speed (Assuming 48MHz FIRC Source)
    switch(speed) {
        case SPI_SPEED_1MHZ:
            spi_reg->CCR = LPSPI_CCR_SCKDIV(46) | LPSPI_CCR_DBT(46) | LPSPI_CCR_PCSSCK(46) | LPSPI_CCR_SCKPCS(46);
            break;
        case SPI_SPEED_4MHZ:
            spi_reg->CCR = LPSPI_CCR_SCKDIV(10) | LPSPI_CCR_DBT(10) | LPSPI_CCR_PCSSCK(10) | LPSPI_CCR_SCKPCS(10);
            break;
        case SPI_SPEED_8MHZ:
            spi_reg->CCR = LPSPI_CCR_SCKDIV(4) | LPSPI_CCR_DBT(4) | LPSPI_CCR_PCSSCK(4) | LPSPI_CCR_SCKPCS(4);
            break;
    }

    // Configure Transmit Command Register (TCR)
    // CPOL = 0, CPHA = 0, 8-bit frame (FRAMESZ = 7), PCS0
    spi_reg->TCR = LPSPI_TCR_CPOL(0) | LPSPI_TCR_CPHA(0) | LPSPI_TCR_PRESCALE(0) | LPSPI_TCR_PCS(0) | LPSPI_TCR_FRAMESZ(7);

    // Enable Module
    spi_reg->CR |= LPSPI_CR_MEN_MASK;
}

uint8_t SPI_Transfer(SPI_Module_t spi_num, uint8_t data) {
    LPSPI_Type *spi_reg = (spi_num == SPI_0) ? IP_LPSPI0 : IP_LPSPI1;

    // Wait until Transmit Data Flag (TDF) is empty
    while((spi_reg->SR & LPSPI_SR_TDF_MASK) == 0);

    // Clear receive flag and push data to TX FIFO
    spi_reg->SR |= LPSPI_SR_RDF_MASK;
    spi_reg->TDR = data;

    // Wait until Receive Data Flag (RDF) is full
    while((spi_reg->SR & LPSPI_SR_RDF_MASK) == 0);

    // Read RX FIFO
    return (uint8_t)(spi_reg->RDR & 0xFF);
}

void SPI_TransferBulk(SPI_Module_t spi_num, uint8_t *tx_data, uint8_t *rx_data, uint32_t length) {
    for (uint32_t i = 0; i < length; i++) {
        uint8_t incoming = SPI_Transfer(spi_num, tx_data ? tx_data[i] : 0xFF);
        if (rx_data) {
            rx_data[i] = incoming;
        }
    }
}
