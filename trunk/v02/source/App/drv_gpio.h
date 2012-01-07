/**
  ******************************************************************************
  * @file    source/App/drv_gpio.h 
  * @author  cn0086@139.com
  * @version V00
  * @date    2011/10/27 9:54:35
  * @brief   This file contains global function
  ******************************************************************************
  * @history v00: 2011/10/27, draft, by cn0086@139.com
  * @        v01:
  */ 

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __DRV_GPIO_H
#define __DRV_GPIO_H

/* Includes ------------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/

typedef enum 
{
  LED1   = 0, //Green LED
  GSM_PM = 1, //GSM power control
  GPS_PM = 2, //GPS module power control
  SD_PM  = 3  //SD card power control
} led_typedef;

/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/

#define LEDn                             4

#define LED1_PIN                         GPIO_Pin_8
#define LED1_GPIO_PORT                   GPIOC
#define LED1_GPIO_CLK                    RCC_APB2Periph_GPIOC
#define LED1_GPIO_MODE                   GPIO_Mode_Out_PP

#define GSM_PM_PIN                       GPIO_Pin_9
#define GSM_PM_GPIO_PORT                 GPIOB
#define GSM_PM_GPIO_CLK                  RCC_APB2Periph_GPIOB
#define GSM_PM_GPIO_MODE                 GPIO_Mode_Out_PP

#define GPS_PM_PIN                       GPIO_Pin_8  
#define GPS_PM_GPIO_PORT                 GPIOA
#define GPS_PM_GPIO_CLK                  RCC_APB2Periph_GPIOA
#define GPS_PM_GPIO_MODE                 GPIO_Mode_Out_OD

#define SD_PM_PIN                        GPIO_Pin_6
#define SD_PM_GPIO_PORT                  GPIOC
#define SD_PM_GPIO_CLK                   RCC_APB2Periph_GPIOC  
#define SD_PM_GPIO_MODE                  GPIO_Mode_Out_OD

#define GSM_PM_OFF                       led_off(GSM_PM)
#define GSM_PM_ON                        led_on(GSM_PM)

#define GPS_PM_OFF                       led_on(GPS_PM)
#define GPS_PM_ON                        led_off(GPS_PM)

#define SD_PM_OFF                        led_on(SD_PM)
#define SD_PM_ON                         led_off(SD_PM)

/* Exported functions ------------------------------------------------------- */

void led_init(led_typedef led);
void led_on(led_typedef led);
void led_off(led_typedef led);
void led_toggle(led_typedef led);

void gpio_init( void ) ;

#endif /* __DRV_GPIO_H */
