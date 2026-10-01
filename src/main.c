#include "S32K142.h"
#include "clock.h"
#include "uart.h"

extern void delay(uint32_t ms);

// External declarations mapped to our complete bme680_spi.c library
extern void LPSPI0_Init(void);
extern void bme680_init_sensor(void);
extern void bme680_get_data(double *temperature, double *pressure, double *humidity, double *gas_res);

int main(void) {
    Clock_InitSystem(SYS_CLK_48MHZ_FIRC);
    UART_Begin(115200);
    LPSPI0_Init();

    UART_Println("\r\n======================================");
    UART_Println("BME680 Full Sensor Initialization...");

    // Boot the sensor and extract all factory T, P, H, and Gas data
    bme680_init_sensor();

    UART_Println("Calibration Loaded. Entering Main Loop.");
    UART_Println("======================================");

    double temp_c, press_hpa, humidity_rh, gas_ohms;

    while(1) {
        // Trigger measurement, heat gas layer, and calculate final values
        bme680_get_data(&temp_c, &press_hpa, &humidity_rh, &gas_ohms);

        UART_Print("Temp: ");
        UART_PrintFloat((float)temp_c, 2);
        UART_Print(" C | ");

        UART_Print("Hum: ");
        UART_PrintFloat((float)humidity_rh, 2);
        UART_Print(" % | ");

        UART_Print("Press: ");
        UART_PrintFloat((float)press_hpa, 2);
        UART_Print(" hPa | ");

        UART_Print("Gas Res: ");
        if (gas_ohms > 0) {
            UART_PrintFloat((float)(gas_ohms / 1000.0), 2); // Print as kOhms
            UART_Println(" kOhm");
        } else {
            UART_Println("Heating...");
        }

        // Zanduino-style loop delay
        delay(1000);
    }
}
