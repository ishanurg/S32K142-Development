/*
 * gpio.h
 *
 *  Created on: 19-Sep-2026
 *      Author: ishan
 */

#ifndef GPIO_H
#define GPIO_H

#include "S32K142.h"
#include <stdint.h>

// ---------------------------------------------------------
// Arduino-Style Vocabulary
// ---------------------------------------------------------
typedef enum {
    PORT_A = 0,
    PORT_B = 1,
    PORT_C = 2,
    PORT_D = 3,
    PORT_E = 4
} Port_t;

typedef enum {
    INPUT = 0,
    OUTPUT = 1
} PinMode;

typedef enum {
    LOW = 0,
    HIGH = 1
} PinState;

// ---------------------------------------------------------
// RGB LED Definitions
// ---------------------------------------------------------
#define RGB_PORT PORT_D
#define LED_BLUE_PIN  0
#define LED_RED_PIN   15
#define LED_GREEN_PIN 16

typedef enum {
    COLOR_OFF,
    COLOR_RED,
    COLOR_GREEN,
    COLOR_BLUE,
    COLOR_YELLOW,
    COLOR_CYAN,
    COLOR_MAGENTA,
    COLOR_WHITE
} LedColor;

// ---------------------------------------------------------
// Function Prototypes
// ---------------------------------------------------------

// Advanced pin routing for hardware peripherals (UART, PWM, SPI, etc.)
void pinMux(Port_t port, uint8_t pin, uint8_t mux_val);

// Core Arduino-Style GPIO Functions
void pinMode(Port_t port, uint8_t pin, PinMode mode);
void digitalWrite(Port_t port, uint8_t pin, PinState state);
void digitalToggle(Port_t port, uint8_t pin);
PinState digitalRead(Port_t port, uint8_t pin);

// RGB LED Specific Functions
void RGB_Init(void);
void RGB_SetColor(LedColor color);

#endif /* GPIO_H */
