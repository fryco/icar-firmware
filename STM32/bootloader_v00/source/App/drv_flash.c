#include "main.h"

extern struct ICAR_DEVICE my_icar;

//0: ok, others: error
unsigned char flash_upgrade(  ) 
{
	unsigned char i , blk_cnt;
	u16 var_u16, fw_size;

	FLASH_Status FLASHStatus = FLASH_COMPLETE;

	//Calc. block:
	fw_size = *(vu16*)(FLASH_UPGRADE_BASE_F+NEW_FW_SIZE) ;
	var_u16 = *(vu16*)(FLASH_UPGRADE_BASE_F+NEW_FW_REV) ;

	if((fw_size%1024) > 0 ){
		blk_cnt = (fw_size >> 10) + 1;
	}
	else{
		blk_cnt = (fw_size >> 10);
	}

	printf("New firmware: %d, size: %d  block cnt: %d ready.\r\n\r\n",\
			var_u16, fw_size, blk_cnt);

	/* Allow access to BKP Domain */
	PWR_BackupAccessCmd(ENABLE);

	//save to BK reg, main app upload this event to server
	//BKP_DR6, new fw rev
	//BKP_DR7, new fw size
    BKP_WriteBackupRegister(BKP_DR6, var_u16);//fw rev
    BKP_WriteBackupRegister(BKP_DR7, fw_size);//fw size

	/* Unlock the Flash Bank1 Program Erase controller */
	FLASH_UnlockBank1();

	// erase configuration block data
	/* Clear All pending flags */
	FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);
	
	//Erase the FLASH pages 
	FLASHStatus = FLASH_ErasePage(FLASH_UPGRADE_BASE_F);

	//copy each block
	for ( i = 0 ; i < blk_cnt ; i++ ) {

		printf("Erase blk %02d, move %08X ==> %08X\r\n", i ,\
			FLASH_UPGRADE_BASE_F+FLASH_PAGE_SIZE*(i+1),\
			APPLICATION_ADDRESS+i*FLASH_PAGE_SIZE);

		/* Clear All pending flags */
		FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);
		
		//Erase the FLASH pages 
		FLASHStatus = FLASH_ErasePage(APPLICATION_ADDRESS+i*FLASH_PAGE_SIZE);

		if ( FLASHStatus != FLASH_COMPLETE ) {
			FLASH_LockBank1();
			return 1 ;
		}

		for ( var_u16 = 0 ; var_u16 < FLASH_PAGE_SIZE ; var_u16 = var_u16+2 ) {

			/* Clear All pending flags */
			FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);
		
			FLASHStatus = FLASH_ProgramHalfWord(\
							APPLICATION_ADDRESS+i*FLASH_PAGE_SIZE+var_u16,\
							*(vu16*)(FLASH_UPGRADE_BASE_F+FLASH_PAGE_SIZE*(i+1)+var_u16));
			if ( FLASHStatus != FLASH_COMPLETE ) {
				FLASH_LockBank1();
				return 1 ;
			}
			//printf("%04X ",*(vu16*)(APPLICATION_ADDRESS+i*FLASH_PAGE_SIZE+var_u16));
		}

		// erase OTA data
		/* Clear All pending flags */
		FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);
		
		//Erase the FLASH pages 
		FLASHStatus = FLASH_ErasePage(FLASH_UPGRADE_BASE_F+FLASH_PAGE_SIZE*(i+1));
	}

	//BKP_DR1, ERR index: 	15~12:MCU reset 
	//						11~8:upgrade fw success flag
	//						7~4:GPRS disconnect reason
	//						3~0:GSM module poweroff reason
	var_u16 = (BKP_ReadBackupRegister(BKP_DR1))&0xF0FF;
	var_u16 = var_u16 | (0xA<<8) ;
    BKP_WriteBackupRegister(BKP_DR1, var_u16);

	FLASH_LockBank1();
	return 0 ;
}
