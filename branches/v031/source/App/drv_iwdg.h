/**
  ******************************************************************************
  * @file    source/App/drv_iwdg.h
  * @author  cn0086@139.com
  * @version V00
  * @date    2011/11/5 18:52:14
  * @brief   This file contains global function
  ******************************************************************************
  * @history v00: 2011/11/5, draft, by cn0086@139.com
  * @        v01: 
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
