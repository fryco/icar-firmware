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

struct FIRMWARE_UPGRADE {
	//for upgrade firmware
	unsigned char err_no;//indicate error number
	unsigned char q_idx; //the point for upgrade command queue
};


/* 说明 :
 * firmware限制在60KB以内，升级时先保存在 page68~127 里
 × page67 记录对应的CRC值
 * 0~3   Bytes: 对应page68的CRC值， 4~7  Bytes: CRC值取反
 * 8~11  Bytes: 对应page69的CRC值，12~15 Bytes: CRC值取反
 * 16~19 Bytes: 对应page70的CRC值，20~23 Bytes: CRC值取反
 * 24~27 Bytes: 对应page71的CRC值，28~31 Bytes: CRC值取反
 * ...
 */

//#define FLASH_UPGRADE_BASE				0x08010C00	//Page67
#define FLASH_UPGRADE_BASE				0	//Page67, simu first
#define NEW_FW_REV						0	//4 bytes for rev. 4 B for !rev., start addr
#define NEW_FW_SIZE						8	//4 bytes for size 4 B for !size, start addr
#define BLK_CRC_DAT						16	//4 bytes for CRC 4 B for !CRC, start addr


//Error define for upgrade firmware
#define ERR_UPGRADE_HAVE_NEW_FW			1	//STM32 have new firmware, no need upgrade
#define ERR_UPGRADE_SIZE_LARGE			2	//firmware size too large, > 60KB
#define ERR_UPGRADE_UP_NEWER			3	//Upgrading a newer firmware
#define ERR_UPGRADE_NO_INFO				4	//No upgrade info in flash
#define ERR_UPGRADE_BLK_CRC				5	//Block CRC error
#define ERR_UPGRADE_NO_MATCH			6	//Upgrading FW Rev no match

/* Includes ------------------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */
void flash_program_one_page(void);
void init_flash_map( void ); //for dev. only, will be removed
unsigned char flash_upgrade_ask( unsigned char * ) ;
unsigned char flash_upgrade_rec( unsigned char *, unsigned char * ) ;
#endif /* __APP_FLASH_H */
