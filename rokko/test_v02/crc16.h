/**
  ******************************************************************************
  * SVN revision information:
  * @file    $URL: svn://svn.cn0086.info/icar/internal/rokko/crc16.h $ 
  * @version $Rev: 38 $
  * @author  $Author: cn0086.info $
  * @date    $Date: 2013-01-06 09:43:04 +0800 (Sun, 06 Jan 2013) $
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
