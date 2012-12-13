/**
  ******************************************************************************
  * SVN revision information:
  * @file    $URL: https://icar-firmware.googlecode.com/svn/bootloader_v02/src_comm/App/main.h $ 
  * @version $Rev: 173 $
  * @author  $Author: cn0086.info@gmail.com $
  * @date    $Date: 2012-05-08 10:27:51 +0800 (周二, 2012-05-08) $
  * @brief   This file contains all necessary *.h and global define.
  ******************************************************************************
  */ 

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

/* Includes ------------------------------------------------------------------*/
//#include <stdio.h>
//#include <string.h>
#include <stdbool.h>
#include "stm32f10x.h"
#include "drv_gpio.h"
#include "drv_flash.h"

/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */
void puthex(unsigned char c);
void putstring(unsigned char  *puts);
/* Exported types ------------------------------------------------------------*/

#endif /* __MAIN_H */
