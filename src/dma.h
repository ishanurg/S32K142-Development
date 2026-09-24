/*
 * dma.h
 *
 *  Created on: 24-Sep-2026
 *      Author: User
 */

#ifndef DMA_H
#define DMA_H

#include "S32K142.h"
#include <stdint.h>

#define DMA_REQ_LPSPI0_RX  14
#define DMA_REQ_LPSPI0_TX  15
#define DMA_REQ_LPSPI1_RX  32
#define DMA_REQ_LPSPI1_TX  33
#define DMA_REQ_LPUART1_RX 34
#define DMA_REQ_LPUART1_TX 35
#define DMA_REQ_ADC0       42
#define DMA_REQ_LPI2C0_RX  43
#define DMA_REQ_LPI2C0_TX  44

typedef enum {
    DMA_SIZE_8BIT  = 0,
    DMA_SIZE_16BIT = 1,
    DMA_SIZE_32BIT = 2
} DMA_TransferSize_t;

void DMA_Init(void);
void DMA_ConfigChannel(uint8_t channel, uint8_t mux_source, uint32_t src_addr, uint32_t dest_addr, uint16_t total_bytes, DMA_TransferSize_t size, uint8_t src_increment, uint8_t dest_increment);

#endif /* DMA_H */
