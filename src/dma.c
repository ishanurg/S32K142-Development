/*
 * dma.c
 *
 *  Created on: 24-Sep-2026
 *      Author: User
 */
#include "dma.h"

void DMA_Init(void) {
	IP_PCC->PCCn[PCC_DMAMUX_INDEX] |= PCC_PCCn_CGC_MASK;

    IP_DMA->CR = 0;
}

void DMA_ConfigChannel(uint8_t channel, uint8_t mux_source, uint32_t src_addr, uint32_t dest_addr, uint16_t total_bytes, DMA_TransferSize_t size, uint8_t src_increment, uint8_t dest_increment) {
    IP_DMAMUX->CHCFG[channel] = 0;

    IP_DMA->CERR = DMA_CERR_CERR(channel);
    IP_DMA->CERQ = DMA_CERQ_CERQ(channel);
    IP_DMA->CDNE = DMA_CDNE_CDNE(channel);

    IP_DMA->TCD[channel].SADDR = src_addr;
    IP_DMA->TCD[channel].DADDR = dest_addr;
    IP_DMA->TCD[channel].ATTR = DMA_TCD_ATTR_SSIZE(size) | DMA_TCD_ATTR_DSIZE(size);
    IP_DMA->TCD[channel].SOFF = src_increment;
    IP_DMA->TCD[channel].DOFF = dest_increment;

    uint32_t bytes_per_request = (1 << size);
    IP_DMA->TCD[channel].NBYTES.MLNO = bytes_per_request;

    uint16_t iterations = total_bytes / bytes_per_request;
    IP_DMA->TCD[channel].CITER.ELINKNO = DMA_TCD_CITER_ELINKNO_CITER(iterations);
    IP_DMA->TCD[channel].BITER.ELINKNO = DMA_TCD_BITER_ELINKNO_BITER(iterations);
    IP_DMA->TCD[channel].SLAST = 0;
    IP_DMA->TCD[channel].DLASTSGA = 0;
    IP_DMA->TCD[channel].CSR = 0;

    IP_DMAMUX->CHCFG[channel] = DMAMUX_CHCFG_ENBL_MASK | DMAMUX_CHCFG_SOURCE(mux_source);
    IP_DMA->SERQ = DMA_SERQ_SERQ(channel);
}
