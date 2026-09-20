#include "S32K142.h"
#include "clock.h"
#include "gpio.h"
#include "S32K142_uart.h"
#include "spi.h"

// External delay assuming you kept your SysTick delay implementation
extern void delay(uint32_t ms);

int main(void) {
    Clock_InitSystem(SYS_CLK_48MHZ_FIRC);

    // Init UART for Serial Monitor (115200 Baud)
    UART_Begin(115200);
    delay(100);

    UART_Println("======================================");
    UART_Println("LPSPI0 Physical Loopback Test Starting");
    UART_Println("Ensure jumper connects PTB15 to PTB16");
    UART_Println("======================================");

    // Initialize SPI0 at 1MHz
    SPI_Begin(SPI_1, SPI_SPEED_1MHZ);

    uint8_t counter = 0;

    while(1) {
        UART_Print("Sending byte: ");
        UART_PrintInt(counter);

        // Send byte over MOSI, immediately read what comes back on MISO
        uint8_t received = SPI_Transfer(SPI_1, counter);

        UART_Print(" | Received: ");
        UART_PrintInt(received);

        if (received == counter) {
            UART_Println(" [PASS]");
        } else {
            UART_Println(" [FAIL - Check Jumper Wire]");
        }

        counter++;
        delay(1000); // 1 second delay between tests
    }
}
