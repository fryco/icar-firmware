/**
  ******************************************************************************
  * SVN revision information:
  * @file    $URL$ 
  * @version $Rev$
  * @author  $Author$
  * @date    $Date$
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
#endif /* __DRV_CAN_H */
