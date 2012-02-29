/**
  ******************************************************************************
  * SVN revision information:
  * @file    $URL: https://icar-firmware.googlecode.com/svn/STM32/v040/source/App/drv_adc.h $ 
  * @version $Rev: 65 $
  * @author  $Author: cn0086.info@gmail.com $
  * @date    $Date: 2012-02-08 15:08:27 +0800 (周三, 2012-02-08) $
  * @brief   This is for STM32 internal flash
  ******************************************************************************
  */ 

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __APP_FLASH_H
#define __APP_FLASH_H

/* 说明 :
 * firmware限制在60KB以内，升级时先保存在 page68~127 里
 × page67 记录对应的CRC值
 * 0~3   Bytes: 对应page68的CRC值， 4~7  Bytes: CRC值取反
 * 8~11  Bytes: 对应page69的CRC值，12~15 Bytes: CRC值取反
 * 16~19 Bytes: 对应page70的CRC值，20~23 Bytes: CRC值取反
 * 24~27 Bytes: 对应page71的CRC值，28~31 Bytes: CRC值取反
 * ...
 */

#define FLASH_UPGRADE_BASE				0x08010C00	//Page67

/* Includes ------------------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */
void flash_program_one_page(void);
unsigned char flash_upgrade( unsigned char *, unsigned char * ) ;
#endif /* __APP_FLASH_H */