/*
 * hdc3020.h
 *
 *  Created on: 07-Oct-2026
 *      Author: User
 */

#ifndef HDC3020_H_
#define HDC3020_H_


#include <stdbool.h>

#define HDC3020_I2C_ADDR 0x44


bool HDC3020_TriggerMeasurement(void);


bool HDC3020_ReadData(double *out_temp, double *out_rh);

#endif /* HDC3020_H_ */
