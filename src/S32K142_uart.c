/*
 * S32K142_uart.c
 *
 *  Created on: 18-Sep-2026
 *      Author: ishan
 */

#include "S32K142_uart.h"
#include "S32K142_uart.h"
#include "clock.h"   // Brings in Clock_EnablePort and Clock_GetSystemFreq
#include "gpio.h"    // Brings in pinMux

#define RX_BUFFER_SIZE 64
volatile char rx_buffer[RX_BUFFER_SIZE];
volatile uint16_t rx_head = 0;
volatile uint16_t rx_tail = 0;

#define NVIC_ISER1 (*(volatile uint32_t*)0xE000E104)

#if defined (__ghs__)
    #define __INTERRUPT_IRQ  __interrupt
#elif defined (__ICCARM__)
    #define __INTERRUPT_IRQ  __irq
#elif defined (__GNUC__)
    #define __INTERRUPT_IRQ  __attribute__ ((interrupt ("IRQ")))
#else
    #define __INTERRUPT_IRQ
#endif

void UART_Begin(uint32_t baudrate) {
    // 1. Safely turn on power to Port C and LPUART1 using the Clock library
    Clock_EnablePort(PORT_C);
    Clock_EnablePeripheral(PERIPH_LPUART1);

    // 2. Configure Pin Multiplexing for PTC6 (RX) and PTC7 (TX) using GPIO library
    // MUX 2 tells the pin to connect to the UART hardware
    pinMux(PORT_C, 6, 2);
    pinMux(PORT_C, 7, 2);

    // 3. Disable TX & RX while configuring
    IP_LPUART1->CTRL &= ~(LPUART_CTRL_TE_MASK | LPUART_CTRL_RE_MASK);

    // 4. Calculate Baud Rate dynamically based on actual system clock
    uint32_t current_sys_clk = Clock_GetSystemFreq();
    uint32_t sbr = current_sys_clk / (baudrate * 16);

    IP_LPUART1->BAUD = LPUART_BAUD_SBR(sbr) | LPUART_BAUD_OSR(15);

    // 5. Enable Receive Interrupt (RIE) so the hardware alerts us of new data
    IP_LPUART1->CTRL |= LPUART_CTRL_RIE_MASK;

    // 6. Enable LPUART1 Interrupts in the Core NVIC
    // LPUART1_RxTx_IRQn is 33. It lives in ISER1, bit 1 (33 % 32 = 1)
    NVIC_ISER1 |= (1 << 1);

    // 7. Unpause TX & RX
    IP_LPUART1->CTRL |= LPUART_CTRL_TE_MASK | LPUART_CTRL_RE_MASK;
}

__INTERRUPT_IRQ void LPUART1_RxTx_IRQHandler(void) {
    // Check if the interrupt was caused by a received character (RDRF flag)
    if (IP_LPUART1->STAT & LPUART_STAT_RDRF_MASK) {
        char c = (char)IP_LPUART1->DATA; // Reading clears the flag

        uint16_t next_head = (rx_head + 1) % RX_BUFFER_SIZE;
        // If buffer isn't full, store it
        if (next_head != rx_tail) {
            rx_buffer[rx_head] = c;
            rx_head = next_head;
        }
    }
}

bool UART_Available(void) {
    return (rx_head != rx_tail);
}

int UART_Read(void) {
    if (rx_head == rx_tail) {
        return -1;
    }

    char c = rx_buffer[rx_tail];
    rx_tail = (rx_tail + 1) % RX_BUFFER_SIZE;
    return c;
}

void UART_ReadString(char *buffer, uint32_t limit) {
    uint32_t index = 0;
    while (index < (limit - 1)) {
        if (UART_Available()) {
            char c = (char)UART_Read();
            if (c == '\n' || c == '\r') {
                break; // Stop reading on enter key
            }
            buffer[index++] = c;
        }
    }
    buffer[index] = '\0'; // Null-terminate
}

void UART_Write(char c) {
    // Wait until the Transmit Data Register is Empty
    while ((IP_LPUART1->STAT & LPUART_STAT_TDRE_MASK) == 0);
    IP_LPUART1->DATA = c;
}

void UART_Print(const char *str) {
    while (*str != '\0') {
        UART_Write(*str++);
    }
}

void UART_Println(const char *str) {
    UART_Print(str);
    UART_Write('\r');
    UART_Write('\n');
}

void UART_PrintInt(int32_t num) {
    char buffer[12];
    int i = 0;
    bool isNegative = false;

    if (num == 0) {
        UART_Write('0');
        return;
    }

    if (num < 0) {
        isNegative = true;
        num = -num;
    }

    while (num > 0) {
        buffer[i++] = (num % 10) + '0';
        num /= 10;
    }

    if (isNegative) {
        buffer[i++] = '-';
    }

    while (i > 0) {
        UART_Write(buffer[--i]);
    }
}

void UART_PrintFloat(float num, uint8_t decimal_places) {
    if (num < 0.0) {
        UART_Write('-');
        num = -num;
    }

    int32_t int_part = (int32_t)num;
    float remainder = num - (float)int_part;
    UART_PrintInt(int_part);

    if (decimal_places > 0) {
        UART_Write('.');
    }

    while (decimal_places > 0) {
        remainder *= 10.0;
        int32_t digit = (int32_t)remainder;
        UART_Write(digit + '0');
        remainder -= (float)digit;
        decimal_places--;
    }
}
