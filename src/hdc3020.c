/*
 * hdc3020.c
 *
 *  Created on: 07-Oct-2026
 *      Author: User
 */


#include "hdc3020.h"
#include "lpi2c.h"
#include <stdint.h>

bool HDC3020_TriggerMeasurement(void) {
    uint8_t cmd[2] = {0x24, 0x00};
    return LPI2C0_transmit(HDC3020_I2C_ADDR, cmd, 2);
}

bool HDC3020_ReadData(double *out_temp, double *out_rh) {
    uint8_t rx[6] = {0};

    if (!LPI2C0_receive(HDC3020_I2C_ADDR, rx, 6)) {
        return false;
    }

    uint16_t t_raw = (rx[0] << 8) | rx[1];
    uint16_t h_raw = (rx[3] << 8) | rx[4];

    *out_temp = (((double)t_raw / 65535.0) * 175.0) - 45.0;
    *out_rh   = (((double)h_raw / 65535.0) * 100.0);

    return true;
}
