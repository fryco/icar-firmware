#include "main.h"


/* Private define ------------------------------------------------------------*/
/* Define the STM32F10x FLASH Page Size depending on the used STM32 device */
#if defined (STM32F10X_HD) || defined (STM32F10X_HD_VL) || defined (STM32F10X_CL) || defined (STM32F10X_XL)
  #define FLASH_PAGE_SIZE    ((uint16_t)0x800) //2KB
#else
  #define FLASH_PAGE_SIZE    ((uint16_t)0x400) //1KB
#endif

#define FLASH_BOOT_ADDR  ((uint32_t)0x08000000)
#define FLASH_BOOT_SIZE  ((uint32_t)0x1000) //4KB, page0~3

#define FLASH_APP_ADDR  ((uint32_t)0x08001000)
#define FLASH_APP_SIZE  ((uint32_t)0xF000) //60KB, page4~63

#define FLASH_CFG_ADDR  ((uint32_t)0x08010000)
#define FLASH_CFG_SIZE  ((uint32_t)0x800)  //2KB,  page64~65

#define FLASH_IDX_ADDR  ((uint32_t)0x08010800)
#define FLASH_IDX_SIZE  ((uint32_t)0x400)  //1KB,  page66

#define FLASH_CRC_ADDR  ((uint32_t)0x08010C00)
#define FLASH_CRC_SIZE  ((uint32_t)0x400)  //1KB,  page67

#define FLASH_DAT_ADDR  ((uint32_t)0x08011000)
#define FLASH_DAT_SIZE  ((uint32_t)0xF000) //60KB, page68~127

void flash_program_one_page( )
{
	FLASH_Status FLASHStatus = FLASH_COMPLETE;

	prompt("CRC_value = %X\r\n", CRC_GetCRC( ));
	prompt("CRC_value = %X\r\n", CRC_CalcCRC(0x5A5AA5A5));

	/* Unlock the Flash Bank1 Program Erase controller */
	FLASH_UnlockBank1();

	/* Clear All pending flags */
	FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);
	
	/* Erase the FLASH pages */
	FLASHStatus = FLASH_ErasePage(FLASH_IDX_ADDR);
	if ( FLASHStatus == FLASH_COMPLETE ) {
		prompt("FLASH_ErasePage :%X success.\r\n",FLASH_IDX_ADDR);
	}
	else {
		prompt("FLASH_ErasePage :%X failure: %d\r\n",FLASH_IDX_ADDR,FLASHStatus);
	}

	/* Program Flash Bank1 */
	FLASHStatus = FLASH_ProgramWord(FLASH_IDX_ADDR,0x5A5AA5A5);
	if ( FLASHStatus == FLASH_COMPLETE ) {
		prompt("FLASH_ProgramWord :%X success.\r\n",FLASH_IDX_ADDR);
	}
	else {
		prompt("FLASH_ErasePage :%X failure: %d\r\n",FLASH_IDX_ADDR,FLASHStatus);
	}

	FLASH_LockBank1();
}
