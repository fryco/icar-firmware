/**
  ******************************************************************************
  * SVN revision information:
  * @file    $URL: svn://svn.cn0086.info/icar/internal/APP_HW_v00/src/App/crc16.h $ 
  * @version $Rev: 10 $
  * @author  $Author: cn0086.info $
  * @date    $Date: 2012-12-14 02:25:56 +0800 (周五, 2012-12-14) $
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
