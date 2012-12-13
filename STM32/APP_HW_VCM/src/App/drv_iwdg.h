/**
  ******************************************************************************
  * SVN revision information:
  * @file    $URL: https://icar-firmware.googlecode.com/svn/app_v07/src_comm/App/drv_iwdg.h $ 
  * @version $Rev: 200 $
  * @author  $Author: cn0086.info@gmail.com $
  * @date    $Date: 2012-05-08 12:16:04 +0800 (周二, 2012-05-08) $
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
