/*
 * spi.h
 *
 *  Created on: 20-Sep-2026
 *      Author: ishan
 */

#ifndef SPI_H_
#define SPI_H_

#include "S32K142.h"
#include <stdint.h>

typedef enum {
    SPI_0 = 0,
    SPI_1 = 1
} SPI_Module_t;

typedef enum {
    SPI_SPEED_1MHZ,
    SPI_SPEED_4MHZ,
    SPI_SPEED_8MHZ
} SPI_Speed_t;


void SPI_Begin(SPI_Module_t spi_num, SPI_Speed_t speed);


uint8_t SPI_Transfer(SPI_Module_t spi_num, uint8_t data);


void SPI_TransferBulk(SPI_Module_t spi_num, uint8_t *tx_data, uint8_t *rx_data, uint32_t length);

#endif /* SPI_H_ */
