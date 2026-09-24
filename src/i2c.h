/*
 * i2c.h
 *
 *  Created on: 20-Sep-2026
 *      Author: ishan
 */


#ifndef I2C_H
#define I2C_H

#include "S32K142.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Initializes I2C on PTB6 (SDA) and PTB7 (SCL) at 100kHz
void Wire_Begin(void);

// Change clock speed (e.g., 100000 for 100kHz, 400000 for 400kHz)
void Wire_SetClock(uint32_t frequency);

// Prepares to send data to a specific slave device
void Wire_BeginTransmission(uint8_t address);

// Adds a byte to the transmit buffer
size_t Wire_Write(uint8_t data);

// Executes the transmission.
// Returns: 0 = Success, 2 = NACK received, 4 = Bus Timeout/Error
uint8_t Wire_EndTransmission(void);

// Requests bytes from a slave device
uint8_t Wire_RequestFrom(uint8_t address, uint8_t quantity);

// Checks how many bytes are available to read
int Wire_Available(void);

// Reads a single byte from the receive buffer
int Wire_Read(void);

void Wire_Transmit_DMA(uint8_t address, uint8_t *tx_data, uint16_t length, uint8_t tx_dma_ch );
#endif /* I2C_H */
