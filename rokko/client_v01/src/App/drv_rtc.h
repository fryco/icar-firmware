/**
  ******************************************************************************
  * SVN revision information:
  * @file    $URL: svn://svn.cn0086.info/icar/firmware/APP_HW_v01/src/App/drv_rtc.h $ 
  * @version $Rev: 73 $
  * @author  $Author: cn0086.info $
  * @date    $Date: 2013-01-14 22:25:20 +0800 (周一, 2013-01-14) $
  * @brief   This is realtime clock driver in STM32
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __DRV_RTC_H
#define __DRV_RTC_H

/* Exported macro ------------------------------------------------------------*/

/* Includes ------------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
struct RTC_STATUS {
	//unsigned char update_count ;
	u32 update_timer ;//re-update if > RTC_UPDATE_PERIOD
	u16 prescaler ;  //++ if rtc slow, -- if rtc fast
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
