/*
 * spi.c
 *
 *  Created on: 20-Sep-2026
 *      Author: ishan
 */


#include "spi.h"
#include "clock.h"
#include "gpio.h"
#include "dma.h" // Added DMA Support

void SPI_Begin(SPI_Module_t spi_num, SPI_Speed_t speed) {
    LPSPI_Type *spi_reg;

    if (spi_num == SPI_0) {
        spi_reg = IP_LPSPI0;
        Clock_EnablePort(PORT_B);
        Clock_EnablePeripheral(PERIPH_LPSPI0);

        pinMux(PORT_B, 2, 3); // SCK
        pinMux(PORT_B, 3, 3); // SIN
        pinMux(PORT_B, 4, 3); // SOUT
        pinMux(PORT_B, 5, 3); // PCS0
    }
    else {
        spi_reg = IP_LPSPI1;
        Clock_EnablePort(PORT_B);
        Clock_EnablePeripheral(PERIPH_LPSPI1);

        pinMux(PORT_B, 14, 3); // SCK
        pinMux(PORT_B, 15, 3); // SIN
        pinMux(PORT_B, 16, 3); // SOUT
        pinMux(PORT_B, 17, 3); // PCS
    }

    spi_reg->CR &= ~LPSPI_CR_MEN_MASK;
    spi_reg->CR |= LPSPI_CR_RST_MASK;
    spi_reg->CR &= ~LPSPI_CR_RST_MASK;
    spi_reg->CFGR1 = LPSPI_CFGR1_MASTER(1);

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

    spi_reg->TCR = LPSPI_TCR_CPOL(0) | LPSPI_TCR_CPHA(0) | LPSPI_TCR_PRESCALE(0) | LPSPI_TCR_PCS(0) | LPSPI_TCR_FRAMESZ(7);

    // Enable Module
    spi_reg->CR |= LPSPI_CR_MEN_MASK;
}

uint8_t SPI_Transfer(SPI_Module_t spi_num, uint8_t data) {
    LPSPI_Type *spi_reg = (spi_num == SPI_0) ? IP_LPSPI0 : IP_LPSPI1;

    while((spi_reg->SR & LPSPI_SR_TDF_MASK) == 0);
    spi_reg->SR |= LPSPI_SR_RDF_MASK;
    spi_reg->TDR = data;

    while((spi_reg->SR & LPSPI_SR_RDF_MASK) == 0);
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

void SPI_TransferBulk_DMA(SPI_Module_t spi_num, uint8_t *tx_data, uint8_t *rx_data, uint16_t length, uint8_t tx_dma_ch, uint8_t rx_dma_ch) {
    LPSPI_Type *spi_reg = (spi_num == SPI_0) ? IP_LPSPI0 : IP_LPSPI1;
    uint8_t tx_req = (spi_num == SPI_0) ? DMA_REQ_LPSPI0_TX : DMA_REQ_LPSPI1_TX;
    uint8_t rx_req = (spi_num == SPI_0) ? DMA_REQ_LPSPI0_RX : DMA_REQ_LPSPI1_RX;

    // Reset FIFOs before DMA transfer
    spi_reg->CR |= LPSPI_CR_RTF_MASK | LPSPI_CR_RRF_MASK;

    // Setup RX Channel (Priority: Must be armed first)
    if (rx_data) {
        DMA_ConfigChannel(rx_dma_ch, rx_req, (uint32_t)&spi_reg->RDR, (uint32_t)rx_data, length, DMA_SIZE_8BIT, 0, 1);
    }

    // Setup TX Channel
    if (tx_data) {
        DMA_ConfigChannel(tx_dma_ch, tx_req, (uint32_t)tx_data, (uint32_t)&spi_reg->TDR, length, DMA_SIZE_8BIT, 1, 0);
    }

    // Enable LPSPI Hardware DMA Requests[cite: 11]
    spi_reg->DER = LPSPI_DER_TDDE_MASK | LPSPI_DER_RDDE_MASK;
}
