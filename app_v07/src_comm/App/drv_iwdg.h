/**
  ******************************************************************************
  * SVN revision information:
  * @file    $URL: https://icar-firmware.googlecode.com/svn/STM32/fw_v06/source/App/drv_iwdg.h $ 
  * @version $Rev: 158 $
  * @author  $Author: cn0086.info@gmail.com $
  * @date    $Date: 2012-04-16 15:34:25 +0800 (周一, 2012-04-16) $
  * @brief   This file is for independent watchdog define in STM32
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __DRV_IWDG_H
#define __DRV_IWDG_H

/* Exported macro ------------------------------------------------------------*/

/* Includes ------------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */
//void RTC_Configuration(void);
void iwdg_init(void);
void show_rst_flag( void );
void show_err_log( void );

#endif /* __DRV_IWDG_H */
