#include "main.h"

extern unsigned int page_size; 
extern unsigned int upgrade_base; 

//0: ok, others: error
unsigned char flash_upgrade(  ) 
{
	unsigned char i , blk_cnt;
	u16 var_u16, fw_size;

	FLASH_Status FLASHStatus = FLASH_COMPLETE;

	//Calc. block:
	fw_size = *(vu16*)(upgrade_base+NEW_FW_SIZE) ;
	var_u16 = *(vu16*)(upgrade_base+NEW_FW_REV) ;

	if((fw_size%1024) > 0 ){
		blk_cnt = (fw_size >> 10) + 1;
	}
	else{
		blk_cnt = (fw_size >> 10);
	}

	putstring("New firmware: "), puthex(var_u16>>8), puthex(var_u16&0xFF);
	putstring(" size: "), puthex(fw_size>>8), puthex(fw_size&0xFF);
	putstring(" block cnt: "), puthex(blk_cnt), putstring("\r\n");

	/* Unlock the Flash Bank1 Program Erase controller */
	FLASH_UnlockBank1();

	//copy each block
	for ( i = 0 ; i < blk_cnt ; i++ ) {

		putstring("Erase blk: "), puthex( i ), putstring("\r\n");
		/* Clear All pending flags */
		FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);
		
		//Erase the FLASH pages 
		FLASHStatus = FLASH_ErasePage(APPLICATION_ADDRESS+i*page_size);

		if ( FLASHStatus != FLASH_COMPLETE ) {
			FLASH_LockBank1();
			return 1 ;
		}

		for ( var_u16 = 0 ; var_u16 < page_size ; var_u16 = var_u16+2 ) {

			/* Clear All pending flags */
			FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);
			/*		
			if ( page_size == 0x400 ) {//1KB blk
				FLASHStatus = FLASH_ProgramHalfWord(\
								APPLICATION_ADDRESS+i*page_size+var_u16,\
								*(vu16*)(upgrade_base+page_size*(i+1)+var_u16));
			}
			else { //2KB blk
				FLASHStatus = FLASH_ProgramHalfWord(\
								APPLICATION_ADDRESS+i*page_size+var_u16,\
								*(vu16*)(upgrade_base+0x400+page_size*i+var_u16));
			} */

			FLASHStatus = FLASH_ProgramHalfWord(\
						APPLICATION_ADDRESS+i*page_size+var_u16,\
						*(vu16*)(upgrade_base+0x400+page_size*i+var_u16));

			if ( FLASHStatus != FLASH_COMPLETE ) {
				FLASH_LockBank1();
				return 1 ;
			}
			//printf("%04X ",*(vu16*)(APPLICATION_ADDRESS+i*page_size+var_u16));
		}
	}

	// erase configuration block data
	/* Clear All pending flags */
	FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);
	
	//Erase the FLASH pages, test
	FLASHStatus = FLASH_ErasePage(upgrade_base);

	FLASH_LockBank1();
	return 0 ;
}
