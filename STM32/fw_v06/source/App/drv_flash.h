/**
  ******************************************************************************
  * SVN revision information:
  * @file    $URL$ 
  * @version $Rev$
  * @author  $Author$
  * @date    $Date$
  * @brief   This is for STM32 internal flash
  ******************************************************************************
  */ 

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __APP_FLASH_H
#define __APP_FLASH_H

/* Private define ------------------------------------------------------------*/

struct FIRMWARE_UPGRADE {
	//for upgrade firmware
	unsigned char err_no;//indicate error number
	unsigned char q_idx; //the point for upgrade command queue
	unsigned int prog_fail_addr;//flash address for prog failure
	bool new_fw_ready ;
	u16 page_size; //my_icar.upgrade.page_size
	unsigned int base; //my_icar.upgrade.base
};


/* 说明 :
 * firmware限制在60KB以内，升级时先保存在 page68~127 里
 × page67 记录对应的CRC值, 偏移量从BLK_CRC_DAT 开始
 * 0~3   Bytes: 对应page68的CRC值， 4~7  Bytes: CRC值取反
 * 8~11  Bytes: 对应page69的CRC值，12~15 Bytes: CRC值取反
 * 16~19 Bytes: 对应page70的CRC值，20~23 Bytes: CRC值取反
 * 24~27 Bytes: 对应page71的CRC值，28~31 Bytes: CRC值取反
 * ...
 */

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
#define ERR_UPGRADE_SUCCESSFUL			1	//firmware upgrade successful
#define ERR_UPGRADE_HAS_LATEST_FW		2	//STM32 has latest firmware, no need upgrade
#define ERR_UPGRADE_SIZE_LARGE			3	//firmware size too large, > 60KB
#define ERR_UPGRADE_UP_NEWER			4	//Upgrading a newer firmware
#define ERR_UPGRADE_NO_INFO				5	//No upgrade info in flash
#define ERR_UPGRADE_BLK_CRC				6	//Block CRC error
#define ERR_UPGRADE_NO_MATCH			7	//Upgrading FW Rev no match
#define ERR_UPGRADE_PROG_FAIL			8	//prog flash failure
#define ERR_UPGRADE_STRING_LEN			9	//Upgrade string length un-correct

/* Includes ------------------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */
//void flash_program_one_page(void);
unsigned char flash_prog_u16( uint32_t addr, uint16_t data);
unsigned char flash_upgrade_ask( unsigned char * ) ;
unsigned char flash_upgrade_rec( unsigned char *, unsigned char * ) ;
#endif /* __APP_FLASH_H */
