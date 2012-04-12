#include "main.h"

extern struct ICAR_DEVICE my_icar;

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

//return 0 : OK, else error
unsigned char flash_erase( uint32_t addr )
{
	FLASH_Status FLASHStatus = FLASH_COMPLETE;

	/* Unlock the Flash Bank1 Program Erase controller */
	FLASH_UnlockBank1();

	/* Clear All pending flags */
	FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);
	
	//Erase the FLASH pages 
	FLASHStatus = FLASH_ErasePage(addr);
	FLASH_LockBank1();

	if ( FLASHStatus == FLASH_COMPLETE ) {
		printf("FLASH_ErasePage :%08X success.\r\n",addr);
		return 0 ;
	}
	else {
		printf("FLASH_ErasePage :%08X failure: %d\r\n",addr,FLASHStatus);
		printf("Check %s:%d\r\n",__FILE__,__LINE__);
		return 1 ;
	}
}

//return 0 : OK, else return failure 
unsigned char flash_prog_u16( uint32_t addr, uint16_t data)
{
	FLASH_Status FLASHStatus = FLASH_COMPLETE;
	unsigned char retry = MAX_PROG_TRY ;

	/* Unlock the Flash Bank1 Program Erase controller */
	FLASH_UnlockBank1();

	/* Clear All pending flags */
	FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);
	
	/* Program Flash Bank1 */
	while ( retry-- ) {
		FLASHStatus = FLASH_ProgramHalfWord(addr, data);
		if ( *(vu16*)(addr) == data ) {//prog success
			FLASH_LockBank1();
			if ( my_icar.debug > 1) {
				printf("Prog %04X @ %08X, read back: %04X\r\n",data,addr,\
					*(vu16*)(addr));
			}
			return 0 ;
		}
		else {
			printf("Prog %04X @ %08X failure! read back: %04X\r\n",\
					data,addr,*(vu16*)(addr));
		}
	}

	//have try "retry" timer, still failure, return this address
	FLASH_LockBank1();

	printf("Prog add :%08X failure: %d\r\n",addr,FLASHStatus);
	printf("Check %s:%d\r\n",__FILE__,__LINE__);
	my_icar.upgrade.prog_fail_addr = addr ;
	return 1 ;
}


