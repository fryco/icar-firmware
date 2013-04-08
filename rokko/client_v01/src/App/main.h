/**
  ******************************************************************************
  * SVN revision information:
  * @file    $URL: svn://svn.cn0086.info/icar/firmware/APP_HW_v01/src/App/main.h $ 
  * @version $Rev: 102 $
  * @author  $Author: cn0086.info $
  * @date    $Date: 2013-02-16 17:55:12 +0800 (鍛ㄥ叚, 2013-02-16) $
  * @brief   This file contains all necessary *.h and global define.
  ******************************************************************************
  */ 

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

/* Includes ------------------------------------------------------------------*/
#include "config.h"
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
#include "app_obd.h"
#include "drv_uart.h"
#include "drv_gpio.h"
#include "drv_adc.h"
#include "drv_rtc.h"
#include "drv_can.h"
#include "drv_iwdg.h"
#include "drv_mg323.h"
#include "drv_flash.h"
#include "LJH.h"
#include "CRC_8.h"
#include "crc16.h"
#include "prot.h"


/* Exported types ------------------------------------------------------------*/
//For taskmanger

/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/


/* Exported functions ------------------------------------------------------- */


/* Exported types ------------------------------------------------------------*/
struct WARN_MSG {
	unsigned char queue_idx;//send queue index
	unsigned int msg;//file name(1 Byte), msg(1 Byte), line(2 B)
};

struct RECORD_STRUCTURE {//check below for record index define
	unsigned char req;//+1 for each require
	unsigned short val;//value
};

struct RECORD_STRUCTURE_NEW {//check below for record index define
	unsigned int time;//UTC
	unsigned int val;//low  8  bits: record item index, 
					 //high 24 bits: value, max: 0xFF FF FF = 16777216
};


struct ICAR_DEVICE {
	unsigned char hw_rev ;//iCar hardware revision, up to 255
	u16 fw_rev ;//iCar firmware revision, up to 9999
	unsigned char debug ;//debug flag
	unsigned int login_timer;
	unsigned char *sn ;//serial number
	unsigned char need_sn;//server need SN
	bool event;//server need SN

	unsigned char err_q_idx[MAX_ERR_MSG];
	struct WARN_MSG warn[MAX_WARN_MSG];
	
	struct RTC_STATUS stm32_rtc;
	struct ADC_STATUS stm32_adc;

	struct UART_TX stm32_u1_tx;
	struct UART_RX stm32_u1_rx;

	struct UART_TX stm32_u2_tx;
	struct UART_RX stm32_u2_rx;

	struct UART_TX stm32_u3_tx;
	struct UART_RX stm32_u3_rx;

	struct UART_TX stm32_data_tx;
	struct UART_RX stm32_data_rx;

	struct GSM_STATUS mg323;

	struct FIRMWARE_UPGRADE upgrade;
	struct PARA_UPDATE update;
	struct PARA_METERS para;
	
	//struct RECORD_STRUCTURE rec[MAX_RECORD_IDX];//will be remove later
	

	bool record_lock;
	unsigned char record_cnt;//record count, < MAX_RECORD_CNT
	struct RECORD_STRUCTURE_NEW rec_new[MAX_RECORD_CNT];
	
//VWP IO定义
	GPIO_TypeDef *vpw_tx_io;
	u16 vpw_tx_pin;
	GPIO_TypeDef *vpw_rx_io;
	u16 vpw_rx_pin;
//PWM IO定义
	GPIO_TypeDef *pwm_tx_io;
	u16 pwm_tx_pin;
	GPIO_TypeDef *pwm_rx_io;
	u16 pwm_rx_pin;

	struct OBD_DAT obd;
};

//record index define
#define REC_IDX_ADC1				10 //for ADC1
#define REC_IDX_ADC2				20 //for ADC2
#define REC_IDX_ADC3				30 //for ADC3
#define REC_IDX_ADC4				40 //for ADC4
#define REC_IDX_ADC5				50 //for ADC5
#define REC_IDX_MCU					60 //for MCU temperature
#define REC_IDX_GSM					90 //for GSM signal

#endif /* __MAIN_H */
