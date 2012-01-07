/**
  ******************************************************************************
  * @file    source/App/main.h 
  * @author  cn0086@139.com
  * @version V00
  * @date    2011/10/27 9:54:35
  * @brief   This file contains all necessary *.h and global define.
  ******************************************************************************
  * @history v00: 2011/10/27, draft, by cn0086@139.com
  * @        v01:
  */ 

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include <os_cfg.h>
#include <ucos_ii.h>
#include <cpu.h>
#include <string.h>
#include <stdbool.h>
#include "stm32f10x.h"
#include "app_cfg.h"
#include "drv_uart.h"
#include "drv_gpio.h"

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/

//#define GSM
#define DEBUG_GSM
#define GSM_TIMEOUT		5000

#define AT_CMD_LENGTH 64 //for GSM command

#define	prompt(x, args...)	printf("[%d,%02d%%]> ",OSTime/1000,OSCPUUsage);printf(x,	##args);

/* Exported functions ------------------------------------------------------- */

#endif /* __MAIN_H */
