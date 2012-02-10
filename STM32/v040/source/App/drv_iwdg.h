/**
  ******************************************************************************
  * SVN revision information:
  * @file    $URL$ 
  * @version $Rev$
  * @author  $Author$
  * @date    $Date$
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
