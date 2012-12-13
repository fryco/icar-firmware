#include "main.h"

GPIO_TypeDef* GPIO_PORT[LEDn] ={OBD_UNKNOW_GPIO_PORT,OBD_KWP_GPIO_PORT,\
								OBD_CAN20_GPIO_PORT,OBD_CAN10_GPIO_PORT,\
								GSM_PM_GPIO_PORT,GPS_PM_GPIO_PORT,\
								CAN_PM_GPIO_PORT};
const uint16_t GPIO_PIN[LEDn] ={OBD_UNKNOW_PIN,OBD_KWP_PIN,OBD_CAN20_PIN,\
								OBD_CAN10_PIN,GSM_PM_PIN,GPS_PM_PIN,\
								CAN_PM_PIN};
const uint32_t GPIO_CLK[LEDn] ={OBD_UNKNOW_GPIO_CLK,OBD_KWP_GPIO_CLK,\
								OBD_CAN20_GPIO_CLK,OBD_CAN10_GPIO_CLK,\
								GSM_PM_GPIO_CLK,GPS_PM_GPIO_CLK,\
								CAN_PM_GPIO_CLK};
GPIOMode_TypeDef GPIO_MODE[LEDn] = {OBD_UNKNOW_GPIO_MODE,OBD_KWP_GPIO_MODE,\
								OBD_CAN20_GPIO_MODE,OBD_CAN10_GPIO_MODE,\
								GSM_PM_GPIO_MODE,GPS_PM_GPIO_MODE,\
								CAN_PM_GPIO_MODE};
 
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

	led_off(OBD_UNKNOW);
	led_off(OBD_KWP);
	led_off(OBD_CAN20);
	led_off(OBD_CAN10);
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

