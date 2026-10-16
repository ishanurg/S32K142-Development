/*
 * inf.h
 *
 *  Created on: 07-Oct-2026
 *      Author: User
 */

#ifndef INF_H_
#define INF_H_

#include <stdint.h>
#include <stdbool.h>

#define INF_I2C_ADDR 0x36

bool INF_TriggerConc(void);
bool INF_ReadConcData(int16_t *out_raw);
bool INF_TriggerTemp(void);
bool INF_ReadTempData(int8_t *out_temp);
typedef struct {
    uint8_t status;
    uint32_t sensor_id;
    uint16_t prod_code;
    uint16_t fw_rev;
    uint8_t manufacturer;
} InfineonInfo_t;

/* Hijacks SCL to wake the internal ASIC */
void INF_WakeSequence(void);


void INF_InitLeaky(void);


bool INF_ReadRawConc(int16_t *out_raw);


void INF_ProcessLeaky(int16_t raw_conc, double *out_leaky1, double *out_leaky2);

/* Sends 0xA9 command, waits 2ms, returns 8-bit ASIC temperature */
bool INF_ReadTemp(int8_t *out_temp);

/* Wakes sensor and sends 0xC2 command, retrieves 12 bytes of sensor info */
bool INF_ReadID(InfineonInfo_t *info);

#endif /* INF_H_ */
