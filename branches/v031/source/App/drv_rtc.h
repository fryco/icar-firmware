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
struct RTC_STATUS {
	unsigned int update_timer ;//record update rtc time, re-update after 1 hour
	unsigned int prescaler ;  //++ if rtc slow, -- if rtc fast

};

struct DATE_TIME
{
    unsigned int year;
    unsigned int month;
    unsigned int day;
    unsigned int hour;
    unsigned int minute;
    unsigned int second;
};

/* Exported constants --------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */
void rtc_init(void);
void RTC_show_time( unsigned int );
void RTC_update_calibrate( unsigned char *, unsigned char * ) ;
//void seconds_to_datetime (unsigned int counts, struct DATE_TIME *datetime);
#endif /* __DRV_RTC_H */
