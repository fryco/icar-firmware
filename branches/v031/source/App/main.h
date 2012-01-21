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
#include "app_taskmanager.h"
#include "app_gsm.h"
#include "drv_uart.h"
#include "drv_gpio.h"
#include "drv_adc.h"
#include "drv_rtc.h"
#include "drv_iwdg.h"
#include "drv_mg323.h"

/* Exported types ------------------------------------------------------------*/
//For taskmanger

/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/

#define DEBUG_GSM

#define AT_TIMEOUT				1000
#define AT_CMD_LENGTH			64 //for GSM command
//#define RTC_UPDATE_PERIOD		1*60*60 //1 Hour
#define RTC_UPDATE_PERIOD		1*1*60 //1 mins
//#define RTC_UPDATE_PERIOD		1*1*2 //2 secs

//For GSM <==> Server protocol
#define GSM_HEAD				0xC9
#define GSM_CMD_RECORD			0x52 //'R', record
#define GSM_CMD_TIME			0x54 //'T', time


#define	prompt(x, args...)	printf("[%d,%02d%%]> ",OSTime/100,OSCPUUsage);printf(x,	##args);

/* Exported functions ------------------------------------------------------- */

/* Exported types ------------------------------------------------------------*/
struct ICAR_DEVICE {
	unsigned int login_timer;
	unsigned char *sn ;//serial number
	struct RTC_STATUS stm32_rtc;
	struct ADC_STATUS stm32_adc;

	struct UART_TX stm32_u1_tx;
	struct UART_RX stm32_u1_rx;

	struct UART_TX stm32_u2_tx;
	struct UART_RX stm32_u2_rx;

	struct UART_TX stm32_u3_tx;
	struct UART_RX stm32_u3_rx;

	struct GSM_STATUS mg323;

};

#endif /* __MAIN_H */
