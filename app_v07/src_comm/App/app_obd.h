/**
  ******************************************************************************
  * SVN revision information:
  * @file    $URL$ 
  * @version $Rev$
  * @author  $Author$
  * @date    $Date$
  ******************************************************************************
**/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __APP_OBD_H
#define __APP_OBD_H

/* Includes ------------------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/
struct OBD_DAT {
	unsigned char can_tx_cnt;//can tx count
};

/* Exported constants --------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */

void  app_task_obd (void *p_arg);

#endif /* __APP_OBD_H */
