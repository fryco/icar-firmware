/**
  ******************************************************************************
  * @file    source/App/drv_rtc.h
  * @author  cn0086@139.com
  * @version V01
  * @date    2011/11/5 16:52:14
  * @brief   This file contains global function
  ******************************************************************************
  * @history v00: 2011/11/5 16:52:19, draft, by cn0086@139.com
  * @        v01: 
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __DRV_RTC_H
#define __DRV_RTC_H

/* Exported macro ------------------------------------------------------------*/

/* Includes ------------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
struct rtc_status {
	unsigned int update_time ;//record update rtc time, re-update after 1 hour
	bool updating ;
	unsigned int prescaler ;  //++ if rtc slow, -- if rtc fast

};

/* Exported constants --------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */
void rtc_init(void);
#endif /* __DRV_RTC_H */
