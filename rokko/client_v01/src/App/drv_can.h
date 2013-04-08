/**
  ******************************************************************************
  * SVN revision information:
  * @file    $URL: svn://svn.cn0086.info/icar/firmware/APP_HW_v01/src/App/drv_can.h $ 
  * @version $Rev: 73 $
  * @author  $Author: cn0086.info $
  * @date    $Date: 2013-01-14 22:25:20 +0800 (周一, 2013-01-14) $
  * @brief   This is realtime clock driver in STM32
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __DRV_CAN_H
#define __DRV_CAN_H

/* Exported macro ------------------------------------------------------------*/

/* Includes ------------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */
//void can_init(void);
u8 can_add_filter( can_std_typedef, u32 );
void can_rec_all_id( bool );
void can_init( can_speed_typedef ,  can_std_typedef  );
bool can_send( u32 can_id, frame_typedef frame_typ, u8 dat_len, u8 * dat );
bool can_enquire_support_pid( void );
//u8 can_read_pid( u8 );
#endif /* __DRV_CAN_H */
