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

// Arduino TwoWire standard API mapped for S32K142
void Wire_Begin(void);
void Wire_SetClock(uint32_t frequency);

void Wire_BeginTransmission(uint8_t address);
size_t Wire_Write(uint8_t data);


uint8_t Wire_EndTransmission(bool sendStop);


uint8_t Wire_RequestFrom(uint8_t address, uint8_t quantity, bool sendStop);

int Wire_Available(void);
int Wire_Read(void);

#endif /* I2C_H */
