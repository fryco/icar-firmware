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
		prompt("FLASH_ErasePage :%08X success.\r\n",addr);
		return 0 ;
	}
	else {
		prompt("FLASH_ErasePage :%08X failure: %d\r\n",addr,FLASHStatus);
		prompt("Check %s:%d\r\n",__FILE__,__LINE__);
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
				prompt("Prog %04X @ %08X, read back: %04X\r\n",data,addr,\
					*(vu16*)(addr));
			}
			return 0 ;
		}
		else {
			prompt("Prog %04X @ %08X failure! read back: %04X\r\n",\
					data,addr,*(vu16*)(addr));
		}
	}

	//have try "retry" timer, still failure, return this address
	FLASH_LockBank1();

	prompt("Prog add :%08X failure: %d\r\n",addr,FLASHStatus);
	prompt("Check %s:%d\r\n",__FILE__,__LINE__);
	my_icar.upgrade.prog_fail_addr = addr ;
	return 1 ;
}

//0: ok, others: error
unsigned char flash_upgrade_ask( unsigned char *buf) 
{//Ask upgrade data ...

	unsigned char blk_cnt;
	u16 var_u16, fw_size;
	unsigned int var_u32, crc_dat ;

	//TBD: Check error flag, feedback to server if error
	//set buf[5] > 0xF0 if error, then report to server for failure detail.

	//check upgrade process status
	if ( *(vu16*)(FLASH_UPGRADE_BASE_F+NEW_FW_REV) == 0xFFFF ) {//empty
		prompt("No upgrade data, can be used.\r\n");
	}
	else {//Upgrading...
		prompt("In upgrade %d ==> %d process...\r\n", my_icar.fw_rev,\
				*(vu16*)(FLASH_UPGRADE_BASE_F+NEW_FW_REV));

		//Ask firmware data
		//Calc. block:
		fw_size = *(vu16*)(FLASH_UPGRADE_BASE_F+NEW_FW_SIZE) ;
		prompt("Firmware size: %X\t", fw_size);

		if((fw_size%1024) > 0 ){
			blk_cnt = (fw_size >> 10) + 1;
		}
		else{
			blk_cnt = (fw_size >> 10);
		}
		printf("Block count: %d\r\n",blk_cnt);

		//Check blk crc empty? ask data if empty
		//C9 97 55 00 xx yy 
		//xx: data len, 
		//yy: KB sequence, 01: ask 1st KB of FW, 02: 2nd KB, 03: 3rd KB

		for ( buf[5] = 1 ; buf[5] <= blk_cnt ; buf[5]++ ) {
			crc_dat = *(vu32*)(FLASH_UPGRADE_BASE_F+BLK_CRC_DAT+buf[5]*8) ;
			printf(".");
			//prompt("Block %d CRC: %X @ %08X\t", buf[5],crc_dat,FLASH_UPGRADE_BASE_F+BLK_CRC_DAT+buf[5]*8);
			if ( crc_dat == 0xFFFFFFFF ) { //empty
				//Check ~CRC again
				crc_dat = *(vu32*)(FLASH_UPGRADE_BASE_F+BLK_CRC_DAT+buf[5]*8+4) ;
				if ( crc_dat == 0 ) {//the CRC just 0xFFFFFFFF
					;//prompt("Block %d CRC: %X @ %08X\t", buf[5],crc_dat,FLASH_UPGRADE_BASE_F+BLK_CRC_DAT+buf[5]*8);
				}
				else { 
					if ( crc_dat == 0xFFFFFFFF ) { //empty
						printf("\r\n");
						prompt("Block %d CRC: %X @ %08X\t", buf[5],crc_dat,FLASH_UPGRADE_BASE_F+BLK_CRC_DAT+buf[5]*8);
						printf("Ask BLK %d data...\r\n",buf[5]);

						//Send : BLK index + new firmware revision
						//C9 00 55 00 03 01 00 5E
						buf[3] = 0;//length high
						buf[4] = 3;//length low
						
						buf[6] = (*(vu16*)(FLASH_UPGRADE_BASE_F+NEW_FW_REV))>>8&0xFF;  //fw rev. high
						buf[7] = (*(vu16*)(FLASH_UPGRADE_BASE_F+NEW_FW_REV))&0xFF;//fw rev. low
						return 0 ;
					}
					else {//Not FF, not 0, the flash failure?
						printf("\r\nFlash failure, check: %s: %d\r\n",__FILE__, __LINE__);
					}
				}
			}
			else {
				if ( my_icar.debug > 1) {
					prompt("Block %d CRC: %X @ %08X\t", buf[5],crc_dat,\
						FLASH_UPGRADE_BASE_F+BLK_CRC_DAT+buf[5]*8);
					printf("Have data, no need ask.\r\n");
				}
			}
		}//end block check
		// Have download all firmware from server, now verify it
		printf("\r\n");
		prompt("Got all fw data! check: %s: %d\r\n",__FILE__, __LINE__);

		// Check whether had verified?
		if ( (*(vu32*)(FLASH_UPGRADE_BASE_F+FW_READY_ADD) == 0xFFFFFFFF) &&\
			(*(vu32*)(FLASH_UPGRADE_BASE_F+FW_READY_ADD+4) == 0xFFFFFFFF) ) {

			//empty, check CRC...

			/* Reset CRC generator */
			CRC->CR = CRC_CR_RESET;
	
			//Calc CRC for local fw in flash
			for ( var_u16 = 0 ; var_u16 < fw_size ; var_u16 = var_u16+4 ) {
				var_u32 = (*(vu8*)(FLASH_UPGRADE_BASE_F+FLASH_PAGE_SIZE+var_u16))<<24 | \
					(*(vu8*)(FLASH_UPGRADE_BASE_F+FLASH_PAGE_SIZE+var_u16+1))<<16 | \
					(*(vu8*)(FLASH_UPGRADE_BASE_F+FLASH_PAGE_SIZE+var_u16+2))<<8 | \
					(*(vu8*)(FLASH_UPGRADE_BASE_F+FLASH_PAGE_SIZE+var_u16+3));
	
				CRC->DR = var_u32 ;
			}

			var_u32 = CRC->DR ;
			prompt("Calc %d Bytes, CRC: %08X\r\n",var_u16, var_u32);

			if ( var_u32 == *(vu32*)(FLASH_UPGRADE_BASE_F+FW_CRC_DAT) ) {
	
				//correct, prog the ready flag to flash
				var_u32 = flash_prog_u16(FLASH_UPGRADE_BASE_F+FW_READY_ADD,FW_READY_FLAG&0xFFFF);
				if ( var_u32 ) return ERR_UPGRADE_PROG_FAIL; //prog failure
	
				var_u32 = flash_prog_u16(FLASH_UPGRADE_BASE_F+FW_READY_ADD+2,(FW_READY_FLAG>>16)&0xFFFF);
				if ( var_u32 ) return ERR_UPGRADE_PROG_FAIL; //prog failure
	
				var_u32 = flash_prog_u16(FLASH_UPGRADE_BASE_F+FW_READY_ADD+4,~(FW_READY_FLAG&0xFFFF));
				if ( var_u32 ) return ERR_UPGRADE_PROG_FAIL; //prog failure
	
				var_u32 = flash_prog_u16(FLASH_UPGRADE_BASE_F+FW_READY_ADD+6,~((FW_READY_FLAG>>16)&0xFFFF));
				if ( var_u32 ) return ERR_UPGRADE_PROG_FAIL; //prog failure
			}

			prompt("Flag in flash is: %08X",\
							*(vu32*)(FLASH_UPGRADE_BASE_F+FW_READY_ADD));
		}
		else { //no empty

			//check flag correct?
			if ( (*(vu32*)(FLASH_UPGRADE_BASE_F+FW_READY_ADD) == FW_READY_FLAG) &&\
				(*(vu32*)(FLASH_UPGRADE_BASE_F+FW_READY_ADD+4) == ~FW_READY_FLAG ) ) {

				my_icar.upgrade.new_fw_ready = true ;//app_task will check this flag and reboot
			}
			else {
				prompt("FW_READY_ADD had been modified: %08X %s:%d\r\n",\
					*(vu32*)(FLASH_UPGRADE_BASE_F+FW_READY_ADD),\
					__FILE__,__LINE__);
				return 1;
			}
		}

		return 0;
	}

	//Default CMD: send current hardware and firmware revision
	//C9 21 55 00 04 00 00 00 5E E7
	buf[3] = 0;//length high
	buf[4] = 4;//length low
	
	buf[5] = 0x00 ;//00: mean buf[6] is hw rev, others: block seq
	buf[6] = my_icar.hw_rev;
	buf[7] = (my_icar.fw_rev>>8)&0xFF;//fw rev. high
	buf[8] = (my_icar.fw_rev)&0xFF;//fw rev. low

	return 0;
}

//Return 0: ok, others error.
unsigned char flash_upgrade_rec( unsigned char *buf, unsigned char *buf_start) 
{//receive upgrade data ...
	u16 buf_index, buf_len, fw_rev, fw_size ;
	unsigned char buf_type ;
	unsigned int var_u32 , fw_crc;
	//C9 57 D5 00 xx yy data
	//xx: data len, 
	//yy: KB sequence, 00: data is latest firmware revision(u16) + size(u16)
	//                 01: 1st KB of FW, 02: 2nd KB, 03: 3rd KB

	if ( (buf+4) < buf_start+GSM_BUF_LENGTH ) {
		buf_len = *(buf+4);
	}
	else {
		buf_len = *(buf+4-GSM_BUF_LENGTH);
	}

	if ( (buf+3) < buf_start+GSM_BUF_LENGTH ) {
		buf_len = ((*(buf+3))<<8) | buf_len;
	}
	else {
		buf_len = ((*(buf+3-GSM_BUF_LENGTH))<<8) | buf_len;
	}

	if ( my_icar.debug ) {
		prompt("Len= %d : ",buf_len);
	}

	if ( buf_len < 5 || buf_len > 1024+7) { 
		//err, Min.: 00 + rev(2Bytes) + size(2B) = 5 Bytes
		//Max. block index(1 Byte) + fw_rev(2B) + fw_data(1024 Bytes) +CRC(4B)
		printf("\r\n");
		prompt("Length: %d is un-correct! Check %s: %d\r\n",\
				buf_len,__FILE__,__LINE__);
		return ERR_UPGRADE_STRING_LEN ;
	}

	//extract 	fw_rev = buf[6] << 8 | buf[7];
	if ( (buf+7) < buf_start+GSM_BUF_LENGTH ) {
		fw_rev = *(buf+7);
	}
	else {
		fw_rev = *(buf+7-GSM_BUF_LENGTH);
	}

	if ( (buf+6) < buf_start+GSM_BUF_LENGTH ) {
		fw_rev = ((*(buf+6))<<8) | fw_rev;
	}
	else {
		fw_rev = ((*(buf+6-GSM_BUF_LENGTH))<<8) | fw_rev;
	}

	//check firmware revision
	if ( fw_rev <=  my_icar.fw_rev ) {//firmware old
		prompt("Error, server fw : %d is older then current: %d\r\n",fw_rev,my_icar.fw_rev);
		return ERR_UPGRADE_HAS_LATEST_FW;
	}

	// buf[5] is indicate buffer type
	if ( (buf+5) < buf_start+GSM_BUF_LENGTH ) {
		buf_type = *(buf+5);
	}
	else {
		buf_type = *(buf+5-GSM_BUF_LENGTH);
	}

	if ( buf_type == 0 ){//FW rev&size info
		//C9 20 D5 00 05 00 FF FF FF FF 39
	
		//extract 	fw_size= buf[8] << 8 | buf[9];
		if ( (buf+9) < buf_start+GSM_BUF_LENGTH ) {
			fw_size = *(buf+9);
		}
		else {
			fw_size = *(buf+9-GSM_BUF_LENGTH);
		}
	
		if ( (buf+8) < buf_start+GSM_BUF_LENGTH ) {
			fw_size = ((*(buf+8))<<8) | fw_size;
		}
		else {
			fw_size = ((*(buf+8-GSM_BUF_LENGTH))<<8) | fw_size;
		}
	
		//check firmware size
		if ( fw_size > 60*1024 ) {//must < 60KB
			prompt("Error, firmware size: %d Bytes> 60KB\r\n",fw_size);
			return ERR_UPGRADE_SIZE_LARGE;
		}

		//extract 	fw_crc= buf[13] ~ buf[10];
		if ( (buf+13) < buf_start+GSM_BUF_LENGTH ) {
			fw_crc = *(buf+13);
		}
		else {
			fw_crc = *(buf+13-GSM_BUF_LENGTH);
		}
	
		if ( (buf+12) < buf_start+GSM_BUF_LENGTH ) {
			fw_crc = ((*(buf+12))<<8) | fw_crc;
		}
		else {
			fw_crc = ((*(buf+12-GSM_BUF_LENGTH))<<8) | fw_crc;
		}
	
		if ( (buf+11) < buf_start+GSM_BUF_LENGTH ) {
			fw_crc = ((*(buf+11))<<16) | fw_crc;
		}
		else {
			fw_crc = ((*(buf+11-GSM_BUF_LENGTH))<<16) | fw_crc;
		}
	
		if ( (buf+10) < buf_start+GSM_BUF_LENGTH ) {
			fw_crc = ((*(buf+10))<<24) | fw_crc;
		}
		else {
			fw_crc = ((*(buf+10-GSM_BUF_LENGTH))<<24) | fw_crc;
		}

		//if ( my_icar.debug ) {
			printf("fw_rev: %d fw_size: %d fw_crc: %08X current fw: %d\r\n", \
					fw_rev,fw_size,fw_crc,my_icar.fw_rev);
		//}
	
		//check firmware revision
		if ( *(vu16*)(FLASH_UPGRADE_BASE_F+NEW_FW_REV) == 0xFFFF ) {//empty

			prompt("FLASH_UPGRADE empty, can be used.\r\n");
			var_u32 = flash_prog_u16(FLASH_UPGRADE_BASE_F+NEW_FW_REV,fw_rev);
			if ( var_u32 ) return ERR_UPGRADE_PROG_FAIL; //prog failure

			var_u32 = flash_prog_u16(FLASH_UPGRADE_BASE_F+NEW_FW_SIZE,fw_size);
			if ( var_u32 ) return ERR_UPGRADE_PROG_FAIL; //prog failure

			//will check again before upgrade, prevent flase failure
			var_u32 = flash_prog_u16(FLASH_UPGRADE_BASE_F+NEW_FW_REV+4,~fw_rev);
			if ( var_u32 ) return ERR_UPGRADE_PROG_FAIL; //prog failure

			var_u32 = flash_prog_u16(FLASH_UPGRADE_BASE_F+NEW_FW_SIZE+4,~fw_size);	
			if ( var_u32 ) return ERR_UPGRADE_PROG_FAIL; //prog failure

			//prog FW CRC to flash
			var_u32 = flash_prog_u16(FLASH_UPGRADE_BASE_F+FW_CRC_DAT+0,fw_crc&0xFFFF);
			if ( var_u32 ) return ERR_UPGRADE_PROG_FAIL; //prog failure

			var_u32 = flash_prog_u16(FLASH_UPGRADE_BASE_F+FW_CRC_DAT+2,(fw_crc>>16)&0xFFFF);
			if ( var_u32 ) return ERR_UPGRADE_PROG_FAIL; //prog failure

			var_u32 = flash_prog_u16(FLASH_UPGRADE_BASE_F+FW_CRC_DAT+4,~(fw_crc&0xFFFF));
			if ( var_u32 ) return ERR_UPGRADE_PROG_FAIL; //prog failure

			var_u32 = flash_prog_u16(FLASH_UPGRADE_BASE_F+FW_CRC_DAT+6,~((fw_crc>>16)&0xFFFF));
			if ( var_u32 ) return ERR_UPGRADE_PROG_FAIL; //prog failure

		}
		else { //upgrading...
			//check upgrading rev is same as new rev?
			if ( *(vu16*)(FLASH_UPGRADE_BASE_F+NEW_FW_REV) == fw_rev ) {//same

				prompt("Firmware %d ==> %d upgrading...",my_icar.fw_rev,fw_rev);
			}
			else {//difference, case:升级过程中，又有新版本发布
				if ( fw_rev > *(vu16*)(FLASH_UPGRADE_BASE_F+NEW_FW_REV) )  {//newer
					prompt("Newer Firmware %d ==> %d upgrading...",my_icar.fw_rev,fw_rev);
					flash_erase(FLASH_UPGRADE_BASE_F);
	
					//Save new firmware info:
					var_u32 = flash_prog_u16(FLASH_UPGRADE_BASE_F+NEW_FW_REV,fw_rev);
					if ( var_u32 ) return ERR_UPGRADE_PROG_FAIL; //prog failure

					var_u32 = flash_prog_u16(FLASH_UPGRADE_BASE_F+NEW_FW_SIZE,fw_size);
					if ( var_u32 ) return ERR_UPGRADE_PROG_FAIL; //prog failure		

					//will check again before upgrade, prevent flase failure
					var_u32 = flash_prog_u16(FLASH_UPGRADE_BASE_F+NEW_FW_REV+4,~fw_rev);
					if ( var_u32 ) return ERR_UPGRADE_PROG_FAIL; //prog failure

					var_u32 = flash_prog_u16(FLASH_UPGRADE_BASE_F+NEW_FW_SIZE+4,~fw_size);	
					if ( var_u32 ) return ERR_UPGRADE_PROG_FAIL; //prog failure		
					//TBD if prog error....
				}
				else {//older, maybe something wrong
					prompt("Error, server fw %d is older than upgrading firmware: %d",\
							fw_rev, *(vu16*)(FLASH_UPGRADE_BASE_F+NEW_FW_REV));
					printf("\t exit!\r\n");
					return ERR_UPGRADE_UP_NEWER;
				}
			}
		}
	}//end of if ( buf_type == 0 ){//FW rev&size info
	else { //buf_type != 0, each block data
		//check FW Rev info in flash
		if ( *(vu16*)(FLASH_UPGRADE_BASE_F+NEW_FW_REV) == 0xFFFF ) {//empty

			prompt("Error, no firmware info in flash\r\n");
			return ERR_UPGRADE_NO_INFO ;
		}
		else { //have info, check info is same as block data?
			if ( *(vu16*)(FLASH_UPGRADE_BASE_F+NEW_FW_REV) == fw_rev ) {//same

				//same, Check CRC
				/* Reset CRC generator */
				CRC->CR = CRC_CR_RESET;

				//Calc CRC: C9 F3 D5 00 07 01 00 7D 00 01 FF FF 95
				for ( buf_index = 0 ; buf_index < (buf_len-7)/4; buf_index++) {

					if ( (buf+buf_index*4+8) < buf_start+GSM_BUF_LENGTH ) {
						var_u32 = (*(buf+buf_index*4+8))<<24;
					}
					else {//data in begin of buffer
						var_u32 = (*(buf+buf_index*4+8-GSM_BUF_LENGTH))<<24;
					}

					if ( (buf+buf_index*4+9) < buf_start+GSM_BUF_LENGTH ) {
						var_u32 = ((*(buf+buf_index*4+9))<<16)|var_u32;
					}
					else {//data in begin of buffer
						var_u32 = ((*(buf+buf_index*4+9-GSM_BUF_LENGTH))<<16)|var_u32;
					}

					if ( (buf+buf_index*4+10) < buf_start+GSM_BUF_LENGTH ) {
						var_u32 = ((*(buf+buf_index*4+10))<<8)|var_u32;
					}
					else {//data in begin of buffer
						var_u32 = ((*(buf+buf_index*4+10-GSM_BUF_LENGTH))<<8)|var_u32;
					}

					if ( (buf+buf_index*4+11) < buf_start+GSM_BUF_LENGTH ) {
						var_u32 = ((*(buf+buf_index*4+11)))|var_u32;
					}
					else {//data in begin of buffer
						var_u32 = ((*(buf+buf_index*4+11-GSM_BUF_LENGTH)))|var_u32;
					}
					CRC->DR = var_u32 ;
				}

				//extract CRC value
				if (( buf+buf_len+1) < buf_start+GSM_BUF_LENGTH ) {
					var_u32 = (*(buf+buf_len+1))<<24;
				}
				else {//data in begin of buffer
					var_u32 = (*(buf+buf_len+1-GSM_BUF_LENGTH))<<24;
				}

				if (( buf+buf_len+2) < buf_start+GSM_BUF_LENGTH ) {
					var_u32 = (*(buf+buf_len+2))<<16|var_u32;
				}
				else {//data in begin of buffer
					var_u32 = (*(buf+buf_len+2-GSM_BUF_LENGTH))<<16|var_u32;
				}

				if (( buf+buf_len+3) < buf_start+GSM_BUF_LENGTH ) {
					var_u32 = (*(buf+buf_len+3))<<8|var_u32;
				}
				else {//data in begin of buffer
					var_u32 = (*(buf+buf_len+3-GSM_BUF_LENGTH))<<8|var_u32;
				}

				if (( buf+buf_len+4) < buf_start+GSM_BUF_LENGTH ) {
					var_u32 = (*(buf+buf_len+4))|var_u32;
				}
				else {//data in begin of buffer
					var_u32 = (*(buf+buf_len+4-GSM_BUF_LENGTH))|var_u32;
				}

				if ( var_u32 == CRC->DR ) {//CRC same
					prompt("OK, Block %d CRC :%08X correct.\r\n",buf_type, var_u32);

					//erase previous contents first
					flash_erase(FLASH_UPGRADE_BASE_F+FLASH_PAGE_SIZE*buf_type);

					//prog data to flash: C9 F3 D5 00 07 01 00 7D 00 01 FF FF 95
					//use fw_size to save fw data temporary
					for ( buf_index = 0 ; buf_index < (buf_len-7)/2; buf_index++) {
	
						if ( (buf+buf_index*2+9) < buf_start+GSM_BUF_LENGTH ) {
							fw_size = (*(buf+buf_index*2+9))<<8;
						}
						else {//data in begin of buffer
							fw_size = (*(buf+buf_index*2+9-GSM_BUF_LENGTH))<<8;
						}
	
						if ( (buf+buf_index*2+8) < buf_start+GSM_BUF_LENGTH ) {
							fw_size = ((*(buf+buf_index*2+8)))|fw_size;
						}
						else {//data in begin of buffer
							fw_size = ((*(buf+buf_index*2+8-GSM_BUF_LENGTH)))|fw_size;
						}
	
						/* Reload IWDG counter */
						IWDG_ReloadCounter();  

						//prompt("Page:%d, %08X + %02X: %04X \r\n",\
							(FLASH_UPGRADE_BASE_F+FLASH_PAGE_SIZE*buf_type-0x08000000)/FLASH_PAGE_SIZE,\
							FLASH_UPGRADE_BASE_F+FLASH_PAGE_SIZE*buf_type,\
							buf_index*2,fw_size);
	
						var_u32 = flash_prog_u16(FLASH_UPGRADE_BASE_F+FLASH_PAGE_SIZE*buf_type+buf_index*2,fw_size);
						if ( var_u32 ) return ERR_UPGRADE_PROG_FAIL; //prog failure
					}

					//prog CRC result to flash
					var_u32 = flash_prog_u16(FLASH_UPGRADE_BASE_F+BLK_CRC_DAT+buf_type*8,CRC->DR&0xFFFF);
					if ( var_u32 ) return ERR_UPGRADE_PROG_FAIL; //prog failure

					var_u32 = flash_prog_u16(FLASH_UPGRADE_BASE_F+BLK_CRC_DAT+buf_type*8+2,(CRC->DR>>16)&0xFFFF);
					if ( var_u32 ) return ERR_UPGRADE_PROG_FAIL; //prog failure

					var_u32 = flash_prog_u16(FLASH_UPGRADE_BASE_F+BLK_CRC_DAT+buf_type*8+4,~(CRC->DR&0xFFFF));
					if ( var_u32 ) return ERR_UPGRADE_PROG_FAIL; //prog failure

					var_u32 = flash_prog_u16(FLASH_UPGRADE_BASE_F+BLK_CRC_DAT+buf_type*8+6,~((CRC->DR>>16)&0xFFFF));
					if ( var_u32 ) return ERR_UPGRADE_PROG_FAIL; //prog failure

					//prompt("CRC in flash is: %08X",\
						*(vu32*)(FLASH_UPGRADE_BASE_F+BLK_CRC_DAT+buf_type*8));

				}
				else {//CRC different
					prompt("Rec Block %d:  ", buf_type);
					prompt("HW CRC : %08X  ",CRC->DR);
					prompt("Buf CRC: %08X\r\n", var_u32 );
					prompt("Error, Block %d CRC ERR, check %s: %d\r\n",buf_type,__FILE__,__LINE__);
					return ERR_UPGRADE_BLK_CRC ;
				}
				//TBD
			}
			else {//difference, case:升级过程中，又有新版本发布
				if ( fw_rev > *(vu16*)(FLASH_UPGRADE_BASE_F+NEW_FW_REV) )  {//newer
					prompt("FW info in flash: rev %d, but in buf is %d\r\n",\
						*(vu16*)(FLASH_UPGRADE_BASE_F+NEW_FW_REV),fw_rev);

					prompt("Will be erase upgrade info! check %s: %d\r\n",__FILE__,__LINE__);
					flash_erase(FLASH_UPGRADE_BASE_F);
			
					//TBD if prog error....
				}
				else {//older, maybe something wrong
					prompt("Error, latest firmware %d is older than upgrading firmware: %d",\
							fw_rev, *(vu16*)(FLASH_UPGRADE_BASE_F+NEW_FW_REV));
					printf("\t exit!\r\n");
					return ERR_UPGRADE_UP_NEWER;
				}
			}
		}
	}
	
	return 0;
}

