/**
  ******************************************************************************
  * SVN revision information:
  * @file    $URL$ 
  * @version $Rev$
  * @author  $Author$
  * @date    $Date$
  * @brief   This file is for general purpose input/output define in STM32
  ******************************************************************************
  */ 

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __DRV_GPIO_H
#define __DRV_GPIO_H

/* Includes ------------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/

typedef enum 
{
	pin9 = 0,	//GSM module switch (ON_OFF) control
	pin14 = 1,	//GSM board LED1,0:ON, 1:OFF, for power indicate
	pin15 = 2,	//GSM board LED2,0:ON, 1:OFF, for signal indicate
	pin33 = 3,	//GSM power enable, 0:OFF, 1:ON
	pin40 = 4,	//CAN PM, OD, 0:ON, 1:OFFindicate
	pin51 = 5,	//CAN switch, no use current
	pin62  = 6	//J1850 PM, OD, 0:ON, 1:OFF
} led_typedef;

/* Exported constants --------------------------------------------------------*/

#define LEDn							7

#define pin9_PIN						GPIO_Pin_1
#define pin9_GPIO_PORT					GPIOC
#define pin9_GPIO_CLK					RCC_APB2Periph_GPIOC
#define pin9_GPIO_MODE					GPIO_Mode_Out_PP

#define pin14_PIN						GPIO_Pin_0
#define pin14_GPIO_PORT					GPIOA
#define pin14_GPIO_CLK					RCC_APB2Periph_GPIOA
#define pin14_GPIO_MODE					GPIO_Mode_Out_OD

#define pin15_PIN						GPIO_Pin_1
#define pin15_GPIO_PORT					GPIOA
#define pin15_GPIO_CLK					RCC_APB2Periph_GPIOA
#define pin15_GPIO_MODE					GPIO_Mode_Out_OD

#define pin33_PIN						GPIO_Pin_12
#define pin33_GPIO_PORT					GPIOB
#define pin33_GPIO_CLK					RCC_APB2Periph_GPIOB
#define pin33_GPIO_MODE					GPIO_Mode_Out_PP

#define pin40_PIN						GPIO_Pin_9
#define pin40_GPIO_PORT					GPIOC
#define pin40_GPIO_CLK					RCC_APB2Periph_GPIOC
#define pin40_GPIO_MODE					GPIO_Mode_Out_OD

#define pin51_PIN						GPIO_Pin_10  
#define pin51_GPIO_PORT					GPIOC
#define pin51_GPIO_CLK					RCC_APB2Periph_GPIOC
#define pin51_GPIO_MODE					GPIO_Mode_Out_OD

#define pin62_PIN						GPIO_Pin_9
#define pin62_GPIO_PORT					GPIOB
#define pin62_GPIO_CLK					RCC_APB2Periph_GPIOB  
#define pin62_GPIO_MODE					GPIO_Mode_Out_OD

/* Exported macro ------------------------------------------------------------*/
#define GSM_SW_OFF						led_on(pin9)
#define GSM_SW_ON						led_off(pin9)

#define GSM_PM							pin33
#define GSM_PM_OFF						led_on(pin33)
#define GSM_PM_ON						led_off(pin33)

#define CAN_PM_OFF						led_off(pin40)
#define CAN_PM_ON						led_on(pin40)

#define J1850_PM_OFF					led_off(pin62)
#define J1850_PM_ON						led_on(pin62)

#define PWR_LED							pin14
#define PWR_LED_OFF						led_off(pin14)
#define PWR_LED_ON						led_on(pin14)

#define SG_LED_OFF						led_off(pin15)
#define SG_LED_ON						led_on(pin15)

/* Exported functions ------------------------------------------------------- */

void led_init_all( void );
void led_on(led_typedef led);
void led_off(led_typedef led);
void led_toggle(led_typedef led);

void gpio_init( void ) ;

#endif /* __DRV_GPIO_H */
