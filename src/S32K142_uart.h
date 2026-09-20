/*
 * S32K142_uart.h
 *
 *  Created on: 18-Sep-2026
 *      Author: ishan
 */

#ifndef S32K142_UART_H_
#define S32K142_UART_H_
#include "S32K142.h"
#include <stdint.h>
#include <stdbool.h>

void UART_Begin(uint32_t baudrate);

// Check if data is available in the background ring buffer
bool UART_Available(void);

// Read a single character from the buffer. Returns -1 if empty.
int UART_Read(void);

// Read an entire string until a newline ('\n') or carriage return ('\r') is received
void UART_ReadString(char *buffer, uint32_t limit);

// Write a single character
void UART_Write(char c);

// Print a string
void UART_Print(const char *str);

// Print a string with a newline at the end
void UART_Println(const char *str);

// Print a whole number
void UART_PrintInt(int32_t num);

// Print a decimal number with a specific number of decimal places
void UART_PrintFloat(float num, uint8_t decimal_places);


#endif /* S32K142_UART_H_ */
