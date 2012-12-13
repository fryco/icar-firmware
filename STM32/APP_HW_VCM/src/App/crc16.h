/**
  ******************************************************************************
  * SVN revision information:
  * @file    $URL: https://icar-firmware.googlecode.com/svn/app_v07/src_comm/App/drv_rtc.h $ 
  * @version $Rev: 200 $
  * @author  $Author: cn0086.info@gmail.com $
  * @date    $Date: 2012-05-08 12:16:04 +0800 (周二, 2012-05-08) $
  * @brief   This is CRC16 algorithm
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __CRC16_H
#define __CRC16_H

/* Exported macro ------------------------------------------------------------*/

/* Includes ------------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */
unsigned int crc16tablefast (unsigned char* p, unsigned long len);
//void seconds_to_datetime (unsigned int counts, struct DATE_TIME *datetime);
#endif /* __CRC16_H */
