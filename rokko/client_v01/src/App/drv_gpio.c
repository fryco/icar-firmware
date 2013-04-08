#include "main.h"

GPIO_TypeDef* GPIO_PORT[LEDn] ={pin9_GPIO_PORT,pin14_GPIO_PORT,\
								pin15_GPIO_PORT,pin33_GPIO_PORT,\
								pin40_GPIO_PORT,pin51_GPIO_PORT,\
								pin62_GPIO_PORT};
const uint16_t GPIO_PIN[LEDn] ={pin9_PIN,pin14_PIN,pin15_PIN,\
								pin33_PIN,pin40_PIN,pin51_PIN,\
								pin62_PIN};
const uint32_t GPIO_CLK[LEDn] ={pin9_GPIO_CLK,pin14_GPIO_CLK,\
								pin15_GPIO_CLK,pin33_GPIO_CLK,\
								pin40_GPIO_CLK,pin51_GPIO_CLK,\
								pin62_GPIO_CLK};
GPIOMode_TypeDef GPIO_MODE[LEDn] = {pin9_GPIO_MODE,pin14_GPIO_MODE,\
								pin15_GPIO_MODE,pin33_GPIO_MODE,\
								pin40_GPIO_MODE,pin51_GPIO_MODE,\
								pin62_GPIO_MODE};
 
void led_on(led_typedef led)
{
  GPIO_PORT[led]->BRR = GPIO_PIN[led];
}

void led_off(led_typedef led)
{
  GPIO_PORT[led]->BSRR = GPIO_PIN[led];
}

void led_toggle(led_typedef led)
{
  GPIO_PORT[led]->ODR ^= GPIO_PIN[led];
}

void led_init_all( void )
{
	GPIO_InitTypeDef  GPIO_InitStructure;
	unsigned char led = 0 ;
	
	for ( led = 0 ; led < LEDn ; led++ ) {	
		/* Enable the GPIO_LED Clock */
		RCC_APB2PeriphClockCmd(GPIO_CLK[led], ENABLE);
		
		/* Configure the GPIO_LED pin */
		GPIO_InitStructure.GPIO_Pin = GPIO_PIN[led];
		
		GPIO_InitStructure.GPIO_Mode = GPIO_MODE[led];
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
		GPIO_PORT[led]->BSRR = GPIO_PIN[led];
		GPIO_Init(GPIO_PORT[led], &GPIO_InitStructure);
	}

	GSM_SW_OFF; GSM_PM_OFF; CAN_PM_OFF;
	J1850_PM_OFF; PWR_LED_OFF; SG_LED_OFF;

}

void gpio_init( void ) 
{
  GPIO_InitTypeDef GPIO_InitStructure;
  
  /* Configure all unused GPIO port pins in Analog Input mode (floating input
     trigger OFF), this will reduce the power consumption and increase the device
     immunity against EMI/EMC *************************************************/
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB |
                         RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD |
                         RCC_APB2Periph_GPIOE, ENABLE);

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_All;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  GPIO_Init(GPIOB, &GPIO_InitStructure);
  GPIO_Init(GPIOC, &GPIO_InitStructure);
  GPIO_Init(GPIOD, &GPIO_InitStructure);
  GPIO_Init(GPIOE, &GPIO_InitStructure);

  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB |
                         RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD |
                         RCC_APB2Periph_GPIOE, DISABLE);  
}

