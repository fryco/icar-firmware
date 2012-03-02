#include "main.h"

extern struct ICAR_DEVICE my_icar;

unsigned char flash_map[1024] ;//for develop, simu page67 记录对应的CRC值
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

void init_flash_map( )
{//for dev. only, will be removed
	memset(flash_map, 0xFF, 1024);
}

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

//0: ok, others: error
unsigned char flash_upgrade_ask( unsigned char *buf) 
{//Ask upgrade data ...

	unsigned char blk_cnt;
	u16 fw_size;
	unsigned int crc_dat ;

	//TBD: Check error flag, feedback to server if error
	//set buf[5] > 0xF0 if error, then report to server for failure detail.

	//check upgrade process status
	if ( flash_map[FLASH_UPGRADE_BASE+NEW_FW_REV] == 0xFF && \
			flash_map[FLASH_UPGRADE_BASE+NEW_FW_REV+1] == 0xFF ) {//empty

		prompt("No upgrade data, can be used.\r\n");
	}
	else {//Upgrading...
		prompt("In upgrade %d ==> %d process...\r\n", my_icar.fw_rev,\
				flash_map[FLASH_UPGRADE_BASE+NEW_FW_REV]<<8 | \
				flash_map[FLASH_UPGRADE_BASE+NEW_FW_REV+1]);

		//Ask firmware data
		//Calc. block:
		fw_size = (	flash_map[FLASH_UPGRADE_BASE+NEW_FW_SIZE] << 8 ) |\
					flash_map[FLASH_UPGRADE_BASE+NEW_FW_SIZE+1] ;
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

		for ( buf[5] = 0 ; buf[5] <= blk_cnt ; buf[5]++ ) {
			crc_dat = 	(flash_map[FLASH_UPGRADE_BASE+BLK_CRC_DAT+buf[5]*8]<<24) |\
					(flash_map[FLASH_UPGRADE_BASE+BLK_CRC_DAT+(buf[5]*8)+1]<<16) |\
					(flash_map[FLASH_UPGRADE_BASE+BLK_CRC_DAT+(buf[5]*8)+2]<<8 ) |\
					(flash_map[FLASH_UPGRADE_BASE+BLK_CRC_DAT+(buf[5]*8)+3]    ) ;
			prompt("Block %d CRC: %X\t", buf[5],crc_dat);
			if ( crc_dat == 0xFFFFFFFF ) { //empty
				//Check ~CRC again
				crc_dat=(flash_map[FLASH_UPGRADE_BASE+BLK_CRC_DAT+(buf[5]*8)+4]<<24) |\
						(flash_map[FLASH_UPGRADE_BASE+BLK_CRC_DAT+(buf[5]*8)+5]<<16) |\
						(flash_map[FLASH_UPGRADE_BASE+BLK_CRC_DAT+(buf[5]*8)+6]<<8 ) |\
						(flash_map[FLASH_UPGRADE_BASE+BLK_CRC_DAT+(buf[5]*8)+7]    ) ;
				if ( crc_dat == 0 ) {//the CRC just 0xFFFFFFFF
					printf("Have data, no need ask.\r\n");
				}
				else { 
					if ( crc_dat == 0xFFFFFFFF ) { //empty
						printf("No data, ask BLK %d data...\r\n",buf[5]);

						//Send : BLK index + new firmware revision
						//C9 00 55 00 03 01 00 5E
						buf[3] = 0;//length high
						buf[4] = 3;//length low
						
						buf[5]++; //1: first block, 2:second block
						buf[6] = flash_map[FLASH_UPGRADE_BASE+NEW_FW_REV];  //fw rev. high
						buf[7] = flash_map[FLASH_UPGRADE_BASE+NEW_FW_REV+1];//fw rev. low
						return 0 ;
					}
					else {//Not FF, not 0, the flash failure?
						printf("Flash failure, check: %s: %d\r\n",__FILE__, __LINE__);
					}
				}
			}
			else {
				printf("Have data, no need ask.\r\n");
			}
		}//end block check
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
		//TBD: set to error flag, feedback to server
		return 0 ;
	}

	if ( buf[5] == 0 ){//FW rev&size info
		//C9 20 D5 00 05 00 FF FF FF FF 39
		fw_rev = buf[6] << 8 | buf[7];
		fw_size= buf[8] << 8 | buf[9];
		if ( my_icar.debug ) {
			printf("fw_rev: %d fw_size: %d current fw: %d\r\n", fw_rev,fw_size,my_icar.fw_rev);
		}
	
		if ( fw_rev <=  my_icar.fw_rev ) {//firmware old
			prompt("Error, firmware : %d is older then current: %d\r\n",fw_rev,my_icar.fw_rev);
			return ERR_UPGRADE_HAVE_NEW_FW;
		}
	
		//check firmware size
		if ( fw_size > 60*1024 ) {//must < 60KB
			prompt("Error, firmware size: %d Bytes> 60KB\r\n",fw_size);
			return ERR_UPGRADE_SIZE_LARGE;
		}
	
		//check firmware revision
		if ( flash_map[FLASH_UPGRADE_BASE+NEW_FW_REV] == 0xFF && \
				flash_map[FLASH_UPGRADE_BASE+NEW_FW_REV+1] == 0xFF ) {//empty
	
			prompt("FLASH_UPGRADE empty, can be used.\r\n");
			flash_map[FLASH_UPGRADE_BASE+NEW_FW_REV]   = buf[6] ;
			flash_map[FLASH_UPGRADE_BASE+NEW_FW_REV+1] = buf[7] ;
			flash_map[FLASH_UPGRADE_BASE+NEW_FW_SIZE]  = buf[8] ;
			flash_map[FLASH_UPGRADE_BASE+NEW_FW_SIZE+1]= buf[9] ;
	
			//will check again before upgrade, prevent flase failure
			flash_map[FLASH_UPGRADE_BASE+NEW_FW_REV +4] = ~buf[6] ;
			flash_map[FLASH_UPGRADE_BASE+NEW_FW_REV +5] = ~buf[7] ;
			flash_map[FLASH_UPGRADE_BASE+NEW_FW_SIZE+4] = ~buf[8] ;
			flash_map[FLASH_UPGRADE_BASE+NEW_FW_SIZE+5] = ~buf[9] ;
	
			//for test
			flash_map[440-1]=0xAB;flash_map[441]=0xCD;
			flash_map[442]=0xA5;flash_map[443]=0x5A;
		}
		else { //upgrading...
			//check upgrading rev is same as new rev?
			if ( flash_map[FLASH_UPGRADE_BASE+NEW_FW_REV] == buf[6] && \
					flash_map[FLASH_UPGRADE_BASE+NEW_FW_REV+1] == buf[7] ) {//same
	
				prompt("Firmware %d ==> %d upgrading...",my_icar.fw_rev,fw_rev);
			}
			else {//difference
				if ( buf[6] >= flash_map[FLASH_UPGRADE_BASE+NEW_FW_REV] &&
					 buf[7] >  flash_map[FLASH_UPGRADE_BASE+NEW_FW_REV+1] ) {//newer
	
					prompt("Newer Firmware %d ==> %d upgrading...",my_icar.fw_rev,fw_rev);
					//erase_page( ); //erase the old content
	
					//Save new firmware info:
					flash_map[FLASH_UPGRADE_BASE+NEW_FW_REV]   = buf[6] ;
					flash_map[FLASH_UPGRADE_BASE+NEW_FW_REV+1] = buf[7] ;
					flash_map[FLASH_UPGRADE_BASE+NEW_FW_SIZE]  = buf[8] ;
					flash_map[FLASH_UPGRADE_BASE+NEW_FW_SIZE+1]= buf[9] ;
			
					//will check again before upgrade, prevent flase failure
					flash_map[FLASH_UPGRADE_BASE+NEW_FW_REV +4] = ~buf[6] ;
					flash_map[FLASH_UPGRADE_BASE+NEW_FW_REV +5] = ~buf[7] ;
					flash_map[FLASH_UPGRADE_BASE+NEW_FW_SIZE+4] = ~buf[8] ;
					flash_map[FLASH_UPGRADE_BASE+NEW_FW_SIZE+5] = ~buf[9] ;
				}
				else {//older, maybe something wrong
					prompt("Error, latest firmware %d is older than upgrading firmware: %d",\
							fw_rev, flash_map[FLASH_UPGRADE_BASE+NEW_FW_REV]<<8 | \
									flash_map[FLASH_UPGRADE_BASE+NEW_FW_REV+1]);
					printf("\t exit!\r\n");
					return ERR_UPGRADE_UP_NEWER;
				}
			}
		}
	}//end of if ( buf[5] == 0 ){//FW rev&size info
	else { //each block data
		//check FW Rev info in flash
		if ( flash_map[FLASH_UPGRADE_BASE+NEW_FW_REV] == 0xFF && \
				flash_map[FLASH_UPGRADE_BASE+NEW_FW_REV+1] == 0xFF ) {//empty

			prompt("Error, no firmware info in flash\r\n");
			return ERR_UPGRADE_NO_INFO ;
		}
		else { //have info, check with same as with block data
			if ( flash_map[FLASH_UPGRADE_BASE+NEW_FW_REV] == buf[6] && \
					flash_map[FLASH_UPGRADE_BASE+NEW_FW_REV+1] == buf[7] ) {//same

				//show data
				prompt("Rec: ");
				for ( buf_index = 0 ; buf_index < buf_len+6 ; buf_index++ ) {
					if ( (buf+buf_index) < buf_start+GSM_BUF_LENGTH ) {
						printf("%02X ",*(buf+buf_index));
					}
					else {//data in begin of buffer
						printf("%02X ",*(buf+buf_index-GSM_BUF_LENGTH));
					}
				}
				printf("\r\n");

				//same, verify CRC
				return ERR_UPGRADE_BLK_CRC ;
				//TBD
			}
			else { //diff, maybe error
				prompt("Error, firmware rev. no match, check %s: %d\r\n",__FILE__,__LINE__);
		//TBD: set to error flag, feedback to server
				return ERR_UPGRADE_NO_MATCH ;
			}
		}
	}
	


	return 0;
}

