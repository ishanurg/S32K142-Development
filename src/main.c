#include "S32K142.h"
#include "clock.h"
#include "gpio.h"
#include "uart.h"
#include "adc.h"
#define VREF_VOLTAGE 5.0f

extern void delay(uint32_t ms);



int main(void) {
    // 1. Initialize the system clock to 48MHz
    Clock_InitSystem(SYS_CLK_48MHZ_FIRC);

    // 2. Power up Port C (Required to access the Potentiometer pin PTC14)
    Clock_EnablePort(PORT_C);

    // 3. Initialize UART at 115200 baud rate for the serial monitor
    UART_Begin(115200);
    delay(100);

    // 4. Initialize the ADC in standard blocking mode
    ADC_Begin();



    while(1) {
        // Read the 12-bit analog value from the on-board potentiometer (0 - 4095)
        uint16_t pot_value = analogRead(PIN_POT);

        float voltage = ((float)pot_value / 4095)* VREF_VOLTAGE ;

        // Print the value to the UART terminal
        UART_Print("Raw Value: ");
        UART_PrintInt(pot_value);
        UART_Print(",");
        UART_Print("Voltage Value: ");
        UART_PrintFloat(voltage, 2);
        UART_Println("");

        // Wait 100 milliseconds before the next reading (10 readings per second)
        delay(250);
    }
}
