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

/* Private define ------------------------------------------------------------*/
/* Define the STM32F10x FLASH Page Size depending on the used STM32 device */
#if defined (STM32F10X_HD) || defined (STM32F10X_HD_VL) || defined (STM32F10X_CL) || defined (STM32F10X_XL)
  #define FLASH_PAGE_SIZE    ((uint16_t)0x800) //2KB
#else
  #define FLASH_PAGE_SIZE    ((uint16_t)0x400) //1KB
#endif

struct FIRMWARE_UPGRADE {
	//for upgrade firmware
	unsigned char err_no;//indicate error number
	unsigned char q_idx; //the point for upgrade command queue
	unsigned int prog_fail_addr;//flash address for prog failure
	bool new_fw_ready ;
};

//#define APPLICATION_ADDRESS    (uint32_t)0x08001000 //4KB
#define APPLICATION_ADDRESS    (uint32_t)0x08003000 //12KB

/* 说明 :
 * firmware限制在60KB以内，升级时先保存在 page68~127 里
 × page67 记录对应的CRC值, 偏移量从BLK_CRC_DAT 开始
 * 0~3   Bytes: 对应page68的CRC值， 4~7  Bytes: CRC值取反
 * 8~11  Bytes: 对应page69的CRC值，12~15 Bytes: CRC值取反
 * 16~19 Bytes: 对应page70的CRC值，20~23 Bytes: CRC值取反
 * 24~27 Bytes: 对应page71的CRC值，28~31 Bytes: CRC值取反
 * ...
 */

#define FLASH_UPGRADE_BASE_F			0x08010C00	//Page67
//FW info and FW data read as: *(vu16*)
#define NEW_FW_REV						0	//4 bytes for rev. 4 B for !rev., start addr
#define NEW_FW_SIZE						8	//4 bytes for size 4 B for !size, start addr
#define FW_READY_ADD					16	//4 bytes for FLAG: AA55A5A5 4 B for ~
//CRC read as: *(vu32*)
#define FW_CRC_DAT						24	//4 bytes for CRC, 4 B for !CRC, start addr
#define BLK_CRC_DAT						32	//4 bytes for CRC, 4 B for !CRC, start addr

#define FW_READY_FLAG					0xAA55A5A5

//Error define for upgrade firmware
#define ERR_UPGRADE_NO_ERR				0	//No ERR
#define ERR_UPGRADE_HAVE_NEW_FW			1	//STM32 have new firmware, no need upgrade
#define ERR_UPGRADE_SIZE_LARGE			2	//firmware size too large, > 60KB
#define ERR_UPGRADE_UP_NEWER			3	//Upgrading a newer firmware
#define ERR_UPGRADE_NO_INFO				4	//No upgrade info in flash
#define ERR_UPGRADE_BLK_CRC				5	//Block CRC error
#define ERR_UPGRADE_NO_MATCH			6	//Upgrading FW Rev no match
#define ERR_UPGRADE_PROG_FAIL			7	//prog flash failure

/* Includes ------------------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */
//void flash_program_one_page(void);
unsigned char flash_prog_u16( uint32_t addr, uint16_t data);
unsigned char flash_upgrade( void ) ;

#endif /* __APP_FLASH_H */
