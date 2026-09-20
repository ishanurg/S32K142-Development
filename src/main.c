#include "S32K142.h"
#include "S32K142_uart.h"
#include "gpio.h"
#include "delay.h"


void Clocks_Init(void) {
    IP_SCG->FIRCDIV = SCG_FIRCDIV_FIRCDIV2(1);
}


int main(void){
	Clocks_Init();
	RGB_Init();
//	pinMode(PORT_A, 2, OUTPUT);
 for (int i =0; i <5000; i++)
	{
	         RGB_SetColor(COLOR_BLUE);
	         delay(500);

	         RGB_SetColor(COLOR_OFF);
	         delay(500);
//	         digitalToggle(PORT_A, 2);


	}
return 0;


}
