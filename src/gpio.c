/*
 * gpio.c
 *
 *  Created on: 19-Sep-2026
 *      Author: ishan
 */

#include "gpio.h"

// ---------------------------------------------------------
// INTERNAL HELPER FUNCTION
// Notice we removed the pcc_idx completely. Power management
// is now handled entirely by your new clock.c library!
// ---------------------------------------------------------
static void Get_Port_Pointers(Port_t port, PORT_Type **port_reg, GPIO_Type **gpio_reg) {
    switch (port) {
        case PORT_A: *port_reg = IP_PORTA; *gpio_reg = IP_PTA; break;
        case PORT_B: *port_reg = IP_PORTB; *gpio_reg = IP_PTB; break;
        case PORT_C: *port_reg = IP_PORTC; *gpio_reg = IP_PTC; break;
        case PORT_D: *port_reg = IP_PORTD; *gpio_reg = IP_PTD; break;
        case PORT_E: *port_reg = IP_PORTE; *gpio_reg = IP_PTE; break;
        default:     *port_reg = IP_PORTA; *gpio_reg = IP_PTA; break;
    }
}

// ---------------------------------------------------------
// CORE GPIO FUNCTIONS
// ---------------------------------------------------------

// New function to connect a pin to hardware like UART or PWM
void pinMux(Port_t port, uint8_t pin, uint8_t mux_val) {
    PORT_Type *port_reg;
    GPIO_Type *gpio_reg;
    Get_Port_Pointers(port, &port_reg, &gpio_reg);

    port_reg->PCR[pin] = PORT_PCR_MUX(mux_val);
}

void pinMode(Port_t port, uint8_t pin, PinMode mode) {
    PORT_Type *port_reg;
    GPIO_Type *gpio_reg;
    Get_Port_Pointers(port, &port_reg, &gpio_reg);

    // Automatically set MUX to 1 (Standard GPIO)
    pinMux(port, pin, 1);

    if (mode == OUTPUT) {
        gpio_reg->PDDR |= (1 << pin);
    } else {
        gpio_reg->PDDR &= ~(1 << pin);
    }
}

void digitalWrite(Port_t port, uint8_t pin, PinState state) {
    PORT_Type *port_reg;
    GPIO_Type *gpio_reg;
    Get_Port_Pointers(port, &port_reg, &gpio_reg);

    if (state == HIGH) {
        gpio_reg->PSOR = (1 << pin);
    } else {
        gpio_reg->PCOR = (1 << pin);
    }
}

void digitalToggle(Port_t port, uint8_t pin) {
    PORT_Type *port_reg;
    GPIO_Type *gpio_reg;
    Get_Port_Pointers(port, &port_reg, &gpio_reg);

    gpio_reg->PTOR = (1 << pin);
}

PinState digitalRead(Port_t port, uint8_t pin) {
    PORT_Type *port_reg;
    GPIO_Type *gpio_reg;
    Get_Port_Pointers(port, &port_reg, &gpio_reg);

    if ((gpio_reg->PDIR & (1 << pin)) != 0) {
        return HIGH;
    }
    return LOW;
}

// ---------------------------------------------------------
// RGB LED LOGIC (Active-High Hardware)
// ---------------------------------------------------------

void RGB_Init(void) {
    digitalWrite(RGB_PORT, LED_RED_PIN, LOW);
    digitalWrite(RGB_PORT, LED_GREEN_PIN, LOW);
    digitalWrite(RGB_PORT, LED_BLUE_PIN, LOW);

    pinMode(RGB_PORT, LED_RED_PIN, OUTPUT);
    pinMode(RGB_PORT, LED_GREEN_PIN, OUTPUT);
    pinMode(RGB_PORT, LED_BLUE_PIN, OUTPUT);
}

void RGB_SetColor(LedColor color) {
    digitalWrite(RGB_PORT, LED_RED_PIN, LOW);
    digitalWrite(RGB_PORT, LED_GREEN_PIN, LOW);
    digitalWrite(RGB_PORT, LED_BLUE_PIN, LOW);

    switch (color) {
        case COLOR_RED:     digitalWrite(RGB_PORT, LED_RED_PIN, HIGH); break;
        case COLOR_GREEN:   digitalWrite(RGB_PORT, LED_GREEN_PIN, HIGH); break;
        case COLOR_BLUE:    digitalWrite(RGB_PORT, LED_BLUE_PIN, HIGH); break;
        case COLOR_YELLOW:  digitalWrite(RGB_PORT, LED_RED_PIN, HIGH); digitalWrite(RGB_PORT, LED_GREEN_PIN, HIGH); break;
        case COLOR_CYAN:    digitalWrite(RGB_PORT, LED_GREEN_PIN, HIGH); digitalWrite(RGB_PORT, LED_BLUE_PIN, HIGH); break;
        case COLOR_MAGENTA: digitalWrite(RGB_PORT, LED_RED_PIN, HIGH); digitalWrite(RGB_PORT, LED_BLUE_PIN, HIGH); break;
        case COLOR_WHITE:   digitalWrite(RGB_PORT, LED_RED_PIN, HIGH); digitalWrite(RGB_PORT, LED_GREEN_PIN, HIGH); digitalWrite(RGB_PORT, LED_BLUE_PIN, HIGH); break;
        case COLOR_OFF:
        default: break;
    }
}
