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
  LED1_G = 0, //Green LED
  LED1_R = 1, //Red   LED
  LED2_G = 2, //Green LED
  LED2_R = 3, //Red   LED
  GSM_PM = 4, //GSM power control
  GPS_PM = 5, //GPS module power control
  SD_PM  = 6  //SD card power control
} led_typedef;

/* Exported constants --------------------------------------------------------*/

#define LEDn                             7

#define LED1_G_PIN                       GPIO_Pin_8
#define LED1_G_GPIO_PORT                 GPIOC
#define LED1_G_GPIO_CLK                  RCC_APB2Periph_GPIOC
#define LED1_G_GPIO_MODE                 GPIO_Mode_Out_OD

#define LED1_R_PIN                       GPIO_Pin_9
#define LED1_R_GPIO_PORT                 GPIOC
#define LED1_R_GPIO_CLK                  RCC_APB2Periph_GPIOC
#define LED1_R_GPIO_MODE                 GPIO_Mode_Out_OD

#define LED2_G_PIN                       GPIO_Pin_2
#define LED2_G_GPIO_PORT                 GPIOC
#define LED2_G_GPIO_CLK                  RCC_APB2Periph_GPIOC
#define LED2_G_GPIO_MODE                 GPIO_Mode_Out_OD

#define LED2_R_PIN                       GPIO_Pin_3
#define LED2_R_GPIO_PORT                 GPIOC
#define LED2_R_GPIO_CLK                  RCC_APB2Periph_GPIOC
#define LED2_R_GPIO_MODE                 GPIO_Mode_Out_OD


#define GSM_PM_PIN                       GPIO_Pin_9
#define GSM_PM_GPIO_PORT                 GPIOB
#define GSM_PM_GPIO_CLK                  RCC_APB2Periph_GPIOB
#define GSM_PM_GPIO_MODE                 GPIO_Mode_Out_PP

#define GPS_PM_PIN                       GPIO_Pin_11  
#define GPS_PM_GPIO_PORT                 GPIOA
#define GPS_PM_GPIO_CLK                  RCC_APB2Periph_GPIOA
#define GPS_PM_GPIO_MODE                 GPIO_Mode_Out_OD

#define SD_PM_PIN                        GPIO_Pin_6
#define SD_PM_GPIO_PORT                  GPIOC
#define SD_PM_GPIO_CLK                   RCC_APB2Periph_GPIOC  
#define SD_PM_GPIO_MODE                  GPIO_Mode_Out_OD

/* Exported macro ------------------------------------------------------------*/
#define GSM_PM_OFF                       led_on(GSM_PM)
#define GSM_PM_ON                        led_off(GSM_PM)

#define GPS_PM_OFF                       led_off(GPS_PM)
#define GPS_PM_ON                        led_on(GPS_PM)

#define SD_PM_OFF                        led_off(SD_PM)
#define SD_PM_ON                         led_on(SD_PM)

#define POWER_LED                        LED1_G
#define ONLINE_LED                       LED1_R
#define RELAY_LED              	         LED2_G
#define ALARM_LED              	         LED2_R


/* Exported functions ------------------------------------------------------- */

void led_init_all( void );
void led_on(led_typedef led);
void led_off(led_typedef led);
void led_toggle(led_typedef led);

void gpio_init( void ) ;

#endif /* __DRV_GPIO_H */
