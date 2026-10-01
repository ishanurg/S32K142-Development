/*
 * i2c.c
 *
 *  Created on: 20-Sep-2026
 *      Author: ishan
 */


#include "i2c.h"
#include "clock.h"
#include "gpio.h"

#define WIRE_BUF_SIZE 32
#define I2C_TIMEOUT   500000

static uint8_t tx_buf[WIRE_BUF_SIZE];
static uint8_t tx_len = 0;
static uint8_t target_addr = 0;

static uint8_t rx_buf[WIRE_BUF_SIZE];
static uint8_t rx_head = 0;
static uint8_t rx_tail = 0;

// Internal helper to ensure there is room in the hardware Transmit FIFO
static uint8_t wait_tx_ready(void) {
    uint32_t timeout = I2C_TIMEOUT;
    while (((IP_LPI2C0->MSR & LPI2C_MSR_TDF_MASK) == 0) && timeout) {
        if (IP_LPI2C0->MSR & LPI2C_MSR_NDF_MASK) return 2; // NACK
        timeout--;
    }
    return (timeout == 0) ? 1 : 0; // 1 = Timeout
}

// Internal helper to ensure there is data in the hardware Receive FIFO
static uint8_t wait_rx_ready(void) {
    uint32_t timeout = I2C_TIMEOUT;
    while (((IP_LPI2C0->MSR & LPI2C_MSR_RDF_MASK) == 0) && timeout) {
        if (IP_LPI2C0->MSR & LPI2C_MSR_NDF_MASK) return 2; // NACK
        timeout--;
    }
    return (timeout == 0) ? 1 : 0;
}

static void force_stop_and_clear(void) {
    IP_LPI2C0->MSR = LPI2C_MSR_NDF_MASK | LPI2C_MSR_EPF_MASK | LPI2C_MSR_ALF_MASK;
    IP_LPI2C0->MTDR = LPI2C_MTDR_CMD(2) | LPI2C_MTDR_DATA(0);
}

void Wire_Begin(void) {
    Clock_EnablePort(PORT_B);
    Clock_EnablePeripheral(PERIPH_LPI2C0);

    pinMux(PORT_B, 6, 2);
    pinMux(PORT_B, 7, 2);

    IP_LPI2C0->MCR &= ~LPI2C_MCR_MEN_MASK;
    IP_LPI2C0->MCR |= LPI2C_MCR_RST_MASK;
    IP_LPI2C0->MCR &= ~LPI2C_MCR_RST_MASK;

    Wire_SetClock(100000); // 100kHz Standard Mode

    IP_LPI2C0->MCR |= LPI2C_MCR_MEN_MASK;
}

void Wire_SetClock(uint32_t frequency) {
    bool was_enabled = (IP_LPI2C0->MCR & LPI2C_MCR_MEN_MASK) != 0;
    IP_LPI2C0->MCR &= ~LPI2C_MCR_MEN_MASK;

    uint32_t core_clk = Clock_GetSystemFreq();
    uint32_t divider = core_clk / frequency;
    uint32_t half_div = (divider - 2) / 2;

    IP_LPI2C0->MCCR0 = LPI2C_MCCR0_CLKHI(half_div) |
                       LPI2C_MCCR0_CLKLO(half_div) |
                       LPI2C_MCCR0_DATAVD(half_div / 2) |
                       LPI2C_MCCR0_SETHOLD(half_div / 2);

    if (was_enabled) IP_LPI2C0->MCR |= LPI2C_MCR_MEN_MASK;
}

void Wire_BeginTransmission(uint8_t address) {
    target_addr = address;
    tx_len = 0;
}

size_t Wire_Write(uint8_t data) {
    if (tx_len >= WIRE_BUF_SIZE) return 0;
    tx_buf[tx_len++] = data;
    return 1;
}

uint8_t Wire_EndTransmission(bool sendStop) {
    uint8_t status;

    // Clear old flags
    IP_LPI2C0->MSR = LPI2C_MSR_NDF_MASK | LPI2C_MSR_EPF_MASK | LPI2C_MSR_ALF_MASK | LPI2C_MSR_SDF_MASK;

    // 1. Generate START and send target address
    status = wait_tx_ready();
    if (status != 0) { force_stop_and_clear(); return (status == 2) ? 2 : 4; }
    IP_LPI2C0->MTDR = LPI2C_MTDR_CMD(4) | LPI2C_MTDR_DATA(target_addr << 1);

    // 2. Queue all payload bytes into the hardware FIFO
    for (uint8_t i = 0; i < tx_len; i++) {
        status = wait_tx_ready();
        if (status != 0) { force_stop_and_clear(); return (status == 2) ? 2 : 4; }
        IP_LPI2C0->MTDR = LPI2C_MTDR_CMD(0) | LPI2C_MTDR_DATA(tx_buf[i]);
    }

    // 3. Repeated Start Logic
    if (sendStop) {
        // Normal Mode: Queue a STOP command
        status = wait_tx_ready();
        if (status != 0) { force_stop_and_clear(); return (status == 2) ? 2 : 4; }
        IP_LPI2C0->MTDR = LPI2C_MTDR_CMD(2) | LPI2C_MTDR_DATA(0);

        // Wait for the hardware to detect the Stop condition on the physical bus
        uint32_t timeout = I2C_TIMEOUT;
        while (((IP_LPI2C0->MSR & LPI2C_MSR_SDF_MASK) == 0) && timeout) {
            if (IP_LPI2C0->MSR & LPI2C_MSR_NDF_MASK) { force_stop_and_clear(); return 2; }
            timeout--;
        }
        if (timeout == 0) return 4;
        IP_LPI2C0->MSR = LPI2C_MSR_SDF_MASK;
    }
    else {
        // Repeated Start Mode: Do NOT send a stop.
        // Just wait for the hardware TX FIFO to empty so we know the data actually left the chip.
        uint32_t timeout = I2C_TIMEOUT;
        while (((IP_LPI2C0->MFSR & LPI2C_MFSR_TXCOUNT_MASK) != 0) && timeout) {
            if (IP_LPI2C0->MSR & LPI2C_MSR_NDF_MASK) { force_stop_and_clear(); return 2; }
            timeout--;
        }
        if (timeout == 0) return 4;
    }

    return 0; // Success
}

uint8_t Wire_RequestFrom(uint8_t address, uint8_t quantity, bool sendStop) {
    if (quantity == 0 || quantity > WIRE_BUF_SIZE) return 0;
    uint8_t status;

    IP_LPI2C0->MSR = LPI2C_MSR_NDF_MASK | LPI2C_MSR_EPF_MASK | LPI2C_MSR_ALF_MASK | LPI2C_MSR_SDF_MASK;

    // 1. Generate START and send target address (Read bit set)
    status = wait_tx_ready();
    if (status != 0) { force_stop_and_clear(); return 0; }
    IP_LPI2C0->MTDR = LPI2C_MTDR_CMD(4) | LPI2C_MTDR_DATA((address << 1) | 1);

    // 2. Command hardware to capture N bytes
    status = wait_tx_ready();
    if (status != 0) { force_stop_and_clear(); return 0; }
    IP_LPI2C0->MTDR = LPI2C_MTDR_CMD(1) | LPI2C_MTDR_DATA(quantity - 1);

    // 3. Queue the STOP command if requested
    if (sendStop) {
        status = wait_tx_ready();
        if (status != 0) { force_stop_and_clear(); return 0; }
        IP_LPI2C0->MTDR = LPI2C_MTDR_CMD(2) | LPI2C_MTDR_DATA(0);
    }

    rx_head = 0;
    rx_tail = 0;

    // 4. Read incoming bytes from the hardware FIFO
    for (uint8_t i = 0; i < quantity; i++) {
        status = wait_rx_ready();
        if (status != 0) { force_stop_and_clear(); break; }
        rx_buf[rx_head++] = (uint8_t)(IP_LPI2C0->MRDR & LPI2C_MRDR_DATA_MASK);
    }

    // 5. Wait for the STOP condition to clear the bus
    if (sendStop) {
        uint32_t timeout = I2C_TIMEOUT;
        while (((IP_LPI2C0->MSR & LPI2C_MSR_SDF_MASK) == 0) && timeout) timeout--;
        IP_LPI2C0->MSR = LPI2C_MSR_SDF_MASK;
    }

    return rx_head;
}

int Wire_Available(void) {
    return (rx_head - rx_tail);
}

int Wire_Read(void) {
    if (rx_head == rx_tail) return -1;
    return rx_buf[rx_tail++];
}
