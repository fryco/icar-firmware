/**
  ******************************************************************************
  * SVN revision information:
  * @file    $URL: https://icar-firmware.googlecode.com/svn/STM32/fw_v06/source/App/drv_gpio.h $ 
  * @version $Rev: 158 $
  * @author  $Author: cn0086.info@gmail.com $
  * @date    $Date: 2012-04-16 15:34:25 +0800 (周一, 2012-04-16) $
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
  OBD_UNKNOW = 0, //Green LED, OBD unknow
  OBD_KWP = 1, //Green LED, KWP protocol
  OBD_CAN20 = 2, //Green LED
  OBD_CAN10 = 3, //Green LED
  GSM_PM = 4, //GSM power control
  GPS_PM = 5, //GPS module power control
  SD_PM  = 6  //SD card power control
} led_typedef;

/* Exported constants --------------------------------------------------------*/

#define LEDn                             7

#define OBD_UNKNOW_PIN                   GPIO_Pin_8
#define OBD_UNKNOW_GPIO_PORT             GPIOB
#define OBD_UNKNOW_GPIO_CLK              RCC_APB2Periph_GPIOB
#define OBD_UNKNOW_GPIO_MODE             GPIO_Mode_Out_OD

#define OBD_KWP_PIN                      GPIO_Pin_9
#define OBD_KWP_GPIO_PORT                GPIOB
#define OBD_KWP_GPIO_CLK                 RCC_APB2Periph_GPIOB
#define OBD_KWP_GPIO_MODE                GPIO_Mode_Out_OD

#define OBD_CAN20_PIN                    GPIO_Pin_0
#define OBD_CAN20_GPIO_PORT              GPIOA
#define OBD_CAN20_GPIO_CLK               RCC_APB2Periph_GPIOA
#define OBD_CAN20_GPIO_MODE              GPIO_Mode_Out_OD

#define OBD_CAN10_PIN                    GPIO_Pin_1
#define OBD_CAN10_GPIO_PORT              GPIOA
#define OBD_CAN10_GPIO_CLK               RCC_APB2Periph_GPIOA
#define OBD_CAN10_GPIO_MODE              GPIO_Mode_Out_OD


#define GSM_PM_PIN                       GPIO_Pin_6
#define GSM_PM_GPIO_PORT                 GPIOC
#define GSM_PM_GPIO_CLK                  RCC_APB2Periph_GPIOC
#define GSM_PM_GPIO_MODE                 GPIO_Mode_Out_PP

#define GPS_PM_PIN                       GPIO_Pin_8  
#define GPS_PM_GPIO_PORT                 GPIOA
#define GPS_PM_GPIO_CLK                  RCC_APB2Periph_GPIOA
#define GPS_PM_GPIO_MODE                 GPIO_Mode_Out_OD

#define SD_PM_PIN                        GPIO_Pin_7
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

#define POWER_LED                        OBD_UNKNOW
#define ALARM_LED              	         OBD_KWP
#define ONLINE_LED                       OBD_CAN10
#define RELAY_LED              	         OBD_CAN20


/* Exported functions ------------------------------------------------------- */

void led_init_all( void );
void led_on(led_typedef led);
void led_off(led_typedef led);
void led_toggle(led_typedef led);

void gpio_init( void ) ;

#endif /* __DRV_GPIO_H */
