/*
 * S32K142_uart.c
 *
 *  Created on: 18-Sep-2026
 *      Author: ishan
 */
//

#include "uart.h"
#include "clock.h"
#include "gpio.h"
#include "dma.h" // Added DMA Support

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
    Clock_EnablePort(PORT_C);
    Clock_EnablePeripheral(PERIPH_LPUART1);

    pinMux(PORT_C, 6, 2);
    pinMux(PORT_C, 7, 2);

    IP_LPUART1->CTRL &= ~(LPUART_CTRL_TE_MASK | LPUART_CTRL_RE_MASK);

    uint32_t current_sys_clk = Clock_GetSystemFreq();
    uint32_t sbr = current_sys_clk / (baudrate * 16);

    // Apply Baud Rate and Enable DMA Hardware Flags globally[cite: 12]
    IP_LPUART1->BAUD = LPUART_BAUD_SBR(sbr) | LPUART_BAUD_OSR(15) | LPUART_BAUD_TDMAE_MASK | LPUART_BAUD_RDMAE_MASK;

    IP_LPUART1->CTRL |= LPUART_CTRL_RIE_MASK;
    NVIC_ISER1 |= (1 << 1);

    IP_LPUART1->CTRL |= LPUART_CTRL_TE_MASK | LPUART_CTRL_RE_MASK;
}

__INTERRUPT_IRQ void LPUART1_RxTx_IRQHandler(void) {
    if (IP_LPUART1->STAT & LPUART_STAT_RDRF_MASK) {
        char c = (char)IP_LPUART1->DATA;
        uint16_t next_head = (rx_head + 1) % RX_BUFFER_SIZE;
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
    if (rx_head == rx_tail) return -1;
    char c = rx_buffer[rx_tail];
    rx_tail = (rx_tail + 1) % RX_BUFFER_SIZE;
    return c;
}

void UART_ReadString(char *buffer, uint32_t limit) {
    uint32_t index = 0;
    while (index < (limit - 1)) {
        if (UART_Available()) {
            char c = (char)UART_Read();
            if (c == '\n' || c == '\r') break;
            buffer[index++] = c;
        }
    }
    buffer[index] = '\0';
}

void UART_Write(char c) {
    while ((IP_LPUART1->STAT & LPUART_STAT_TDRE_MASK) == 0);
    IP_LPUART1->DATA = c;
}

void UART_Print(const char *str) {
    while (*str != '\0') UART_Write(*str++);
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

    if (num == 0) { UART_Write('0'); return; }
    if (num < 0) { isNegative = true; num = -num; }
    while (num > 0) { buffer[i++] = (num % 10) + '0'; num /= 10; }
    if (isNegative) { buffer[i++] = '-'; }
    while (i > 0) { UART_Write(buffer[--i]); }
}

void UART_PrintFloat(float num, uint8_t decimal_places) {
    if (num < 0.0) { UART_Write('-'); num = -num; }
    int32_t int_part = (int32_t)num;
    float remainder = num - (float)int_part;
    UART_PrintInt(int_part);

    if (decimal_places > 0) UART_Write('.');
    while (decimal_places > 0) {
        remainder *= 10.0;
        int32_t digit = (int32_t)remainder;
        UART_Write(digit + '0');
        remainder -= (float)digit;
        decimal_places--;
    }
}

void UART_Print_DMA(const char *str, uint16_t length, uint8_t dma_channel) {
    // Note: LPUART_BAUD_TDMAE_MASK is globally enabled in UART_Begin.
    // Transmits buffer completely in the background.
    DMA_ConfigChannel(dma_channel, DMA_REQ_LPUART1_TX, (uint32_t)str, (uint32_t)&IP_LPUART1->DATA, length, DMA_SIZE_8BIT, 1, 0);
}
