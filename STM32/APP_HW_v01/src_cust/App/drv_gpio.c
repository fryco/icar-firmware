#include "main.h"

GPIO_TypeDef* GPIO_PORT[LEDn] ={LED1_G_GPIO_PORT,LED1_R_GPIO_PORT,\
								LED2_G_GPIO_PORT,LED2_R_GPIO_PORT,\
								GSM_PM_GPIO_PORT,GPS_PM_GPIO_PORT,\
								SD_PM_GPIO_PORT};
const uint16_t GPIO_PIN[LEDn] ={LED1_G_PIN,LED1_R_PIN,LED2_G_PIN,\
								LED2_R_PIN,GSM_PM_PIN,GPS_PM_PIN,\
								SD_PM_PIN};
const uint32_t GPIO_CLK[LEDn] ={LED1_G_GPIO_CLK,LED1_R_GPIO_CLK,\
								LED2_G_GPIO_CLK,LED2_R_GPIO_CLK,\
								GSM_PM_GPIO_CLK,GPS_PM_GPIO_CLK,\
								SD_PM_GPIO_CLK};
GPIOMode_TypeDef GPIO_MODE[LEDn] = {LED1_G_GPIO_MODE,LED1_R_GPIO_MODE,\
								LED2_G_GPIO_MODE,LED2_R_GPIO_MODE,\
								GSM_PM_GPIO_MODE,GPS_PM_GPIO_MODE,\
								SD_PM_GPIO_MODE};
 
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

	led_off(LED1_G);
	led_off(LED1_R);
	led_off(LED2_G);
	led_off(LED2_R);
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

