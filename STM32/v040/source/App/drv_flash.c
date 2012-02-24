#include "main.h"

extern struct ICAR_DEVICE my_icar;

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

//Return 0: ok, others error.
unsigned char flash_update( unsigned char *buf, unsigned char *buf_start) 
{
	u16 buf_index, buf_len, fw_rev, fw_size ;

	//C9 57 D5 00 xx yy data
	//xx: data len, 
	//yy: KB sequence, 00: data is latest firmware revision(u16) + size(u16)
	//                 01: 1st KB of FW, 02: 2nd KB, 03: 3rd KB

	buf_len = buf[3] << 8 | buf[4];

	if ( my_icar.debug ) {
		prompt("Len= %d : ",buf_len);
	}

	if ( buf_len < 5 ) { //err, Min.: 00 + rev(2Bytes) + size(2B) = 5 Bytes
		printf("Length error!\r\n");
		return 1 ;
	}

	//C9 20 D5 00 05 00 FF FF FF FF 39
	fw_rev = buf[6] << 8 | buf[7];
	fw_size= buf[8] << 8 | buf[9];
	if ( my_icar.debug ) {
		printf("fw_rev: %d fw_size: %d current fw: %d\r\n", fw_rev,fw_size,my_icar.fw_rev);
	}

	if ( fw_rev <=  my_icar.fw_rev ) {//firmware old
		prompt("Error, firmware : %d is older then current: %d\r\n",fw_rev,my_icar.fw_rev);
		return 2;
	}

	if ( fw_size > 60*1024 ) {//must < 60KB
		prompt("Error, firmware size: %d Bytes> 60KB\r\n",fw_size);
		return 3;
	}

	for ( buf_index = 0 ; buf_index < buf_len+6 ; buf_index++ ) {
		if ( (buf+buf_index) < buf_start+GSM_BUF_LENGTH ) {
			printf("%02X ",*(buf+buf_index));
		}
		else {//data in begin of buffer
			printf("%02X ",*(buf+buf_index-GSM_BUF_LENGTH));
		}
	}
	printf("\r\n");

	return 0;
}

