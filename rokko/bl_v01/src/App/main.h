/**
  ******************************************************************************
  * SVN revision information:
  * @file    $URL: svn://svn.cn0086.info/icar/firmware/BL_HW_v01/src/App/main.h $ 
  * @version $Rev: 79 $
  * @author  $Author: cn0086.info $
  * @date    $Date: 2013-01-14 22:30:53 +0800 (周一, 2013-01-14) $
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
