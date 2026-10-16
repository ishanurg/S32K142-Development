/*
 * lpi2c.h
 *
 *  Created on: 07-Oct-2026
 *      Author: User
 */

#ifndef LPI2C_H_
#define LPI2C_H_

#include <stdint.h>
#include <stdbool.h>

void delay_ms(uint32_t ms);
void delay_us(uint32_t us);

void PORT_init(void);
void LPI2C0_init(void);
bool LPI2C0_transmit(uint16_t saddr, uint8_t *tx_buff, uint32_t tx_num);
bool LPI2C0_receive(uint16_t saddr, uint8_t *rx_buff, uint32_t rx_num);

#endif /* LPI2C_H_ */
