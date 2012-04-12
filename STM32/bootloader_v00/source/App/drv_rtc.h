/**
  ******************************************************************************
  * SVN revision information:
  * @file    $URL: https://icar-firmware.googlecode.com/svn/STM32/v040/source/App/drv_rtc.h $ 
  * @version $Rev: 65 $
  * @author  $Author: cn0086.info@gmail.com $
  * @date    $Date: 2012-02-08 15:08:27 +0800 (周三, 2012-02-08) $
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
//void seconds_to_datetime (unsigned int counts, struct DATE_TIME *datetime);
#endif /* __DRV_RTC_H */
