//$URL: https://rokko-firmware.googlecode.com/svn/ubuntu/iCar_v03/commands.c $ 
//$Rev: 99 $, $Date: 2012-02-29 09:32:56 +0800 (Wed, 29 Feb 2012) $

#include "config.h"
#include "rokkod.h"

extern unsigned int foreground;

static unsigned int mypow( unsigned char n)
{
	unsigned int result ;

	result = 1 ;
	while ( n ) {
		result = result*10;
		n--;
	}
	return result;
}

static void conv_rev( unsigned char *p , unsigned int *fw_rev)
{//$Rev: 9999 $
	unsigned char i , j;

	i = 0 , p = p + 6 ;
	while ( *(p+i) != 0x20 ) {
		i++ ;
		if ( *(p+i) == 0x24 || i > 4 ) break ; //$
	}

	j = 0 ;
	while ( i ) {
		i-- ;
		*fw_rev = (*(p+i)-0x30)*mypow(j) + *fw_rev;
		j++;
	}
}

/****************** Below for server respond client command *********/
//0: ok, 1: disconnect immediately
unsigned char rec_cmd_login( struct rokko_data *rokko, struct rokko_command * cmd,\
				unsigned char *rec_buf, unsigned char *snd_buf )
{//case GSM_CMD_LOGIN: //0x4C, 'L', Login to server

	unsigned int chk_count ;
	pid_t cloud_pid;
	unsigned char post_buf[BUFSIZE];

	//HEAD SEQ CMD Length(2 bytes) OSTime SN(char 10) IP check

	//DE 6A 4C 00 1D 00 04 19 DA //CMD + OSTIME
	//44 45 4D 4F 44 41 33 30 42 32 //SN
	//00 00 00 0D //HW, FW revision
	//31 30 2E 31 31 31 2E 32 36 2E 36 75 5C //IP + CRC16

	if ( strlen(rokko->sn) == 10 ) {
		if ( strncmp(rokko->sn,&rec_buf[9], 10) ) {
			//no same
			//record_error(rokko,&recv_buf[buf_index]); 
//Need change
			fprintf(stderr, "\r\nSN error! First SN: %s, ",\
				rokko->sn);
			strncpy(cmd->pro_sn, &rec_buf[9], 10);
			cmd->pro_sn[10] = 0x0;
			fprintf(stderr, "now is: %s\n\n",rokko->sn);
			return 1 ;
		}
		else {
			fprintf(stderr, "\r\nNo need update ");
			return 0 ;
		}
	}
	else {	//check product SN from DB
		strncpy(cmd->pro_sn, &rec_buf[9], 10);
		cmd->pro_sn[10] = 0x0;
		fprintf(stderr, "SN: %s\t",rokko->sn);
		fprintf(stderr, "cmd->pro_sn: %s\r\n",cmd->pro_sn);
		
		if ( 1 ) {//if check DB ok, TBD
			//send successful respond 
			bzero( snd_buf, BUFSIZE);
			snd_buf[0] = GSM_HEAD ;
			snd_buf[1] = cmd->seq ;
			snd_buf[2] = cmd->pcb | 0x80 ;
			snd_buf[3] =  00;//len high
			snd_buf[4] =  16;//len low
			snd_buf[5] =  00;//return status:0：成功
			snd_buf[6] =  (time(NULL) >> 24)&0xFF;//time high
			snd_buf[7] =  (time(NULL) >> 16)&0xFF;//time high
			snd_buf[8] =  (time(NULL) >> 8)&0xFF;//time low
			snd_buf[9] =  (time(NULL) >> 00)&0xFF;//time low
		
			//MCU ID：12 B, from DB, for verify server
			snd_buf[10] = 0x06 ;
			snd_buf[11] = 0x6B ;
			snd_buf[12] = 0xFF ;
			snd_buf[13] = 0x48 ;
					
			snd_buf[14] = 0x56 ;
			snd_buf[15] = 0x48 ;
			snd_buf[16] = 0x67 ;
			snd_buf[17] = 0x49 ;
		
			snd_buf[18] = 0x87 ;
			snd_buf[19] = 0x10 ;
			snd_buf[20] = 0x12 ;
			snd_buf[21] = 0x37 ;
		
			//Calc CRC16
			cmd->crc16 = crc16tablefast(snd_buf , ((snd_buf[3]<<8)|(snd_buf[4]))+6);
			
			snd_buf[22] = (cmd->crc16)>>8 ;
			snd_buf[23] = (cmd->crc16)&0xFF ;
		
			if ( foreground ) {
				fprintf(stderr, "CMD is Login, will send: ");
				for ( chk_count = 0 ; chk_count < ((snd_buf[3]<<8)|(snd_buf[4]))+8 ; chk_count++ ) {
					fprintf(stderr, "%02X ",snd_buf[chk_count]);
				}
				fprintf(stderr, "to %s\n",cmd->pro_sn);
			}
		
			write(rokko->client_socket,snd_buf,((snd_buf[3]<<8)|(snd_buf[4]))+8);
			
			return 0 ;
		} //end check DB ok
		else {//failure
			//send failure respond 
			bzero( snd_buf, BUFSIZE);
			snd_buf[0] = GSM_HEAD ;
			snd_buf[1] = cmd->seq ;
			snd_buf[2] = cmd->pcb | 0x80 ;
			snd_buf[3] =  00;//len high
			snd_buf[4] =  00;//len low
			snd_buf[5] =  01;// 登陆失败
		
			//Calc CRC16
			cmd->crc16 = crc16tablefast(snd_buf , ((snd_buf[3]<<8)|(snd_buf[4]))+6);
			
			snd_buf[6] = (cmd->crc16)>>8 ;
			snd_buf[7] = (cmd->crc16)&0xFF ;
		
			if ( foreground ) {
				fprintf(stderr, "CMD is Login, but failure, will send: ");
				for ( chk_count = 0 ; chk_count < ((snd_buf[3]<<8)|(snd_buf[4]))+8 ; chk_count++ ) {
					fprintf(stderr, "%02X ",snd_buf[chk_count]);
				}
				fprintf(stderr, "to %s\n",cmd->pro_sn);
			}
		
			write(rokko->client_socket,snd_buf,((snd_buf[3]<<8)|(snd_buf[4]))+8);
			return 1 ;
		}
	}
}

/****************** Below for server send command to client *********/
//0:ok, others: err
unsigned char snd_cmd_upgrade( struct rokko_data *rokko, unsigned char *sequence, unsigned char *snd_buf )
{//send command upgrade to client

	int fd;
	unsigned int  i, fw_size, fw_rev, fpos;
	unsigned char rev_info[MAX_FW_SIZE], *rev_pos;
	unsigned char *filename="./fw/stm32_v00/20121211.bin";
	unsigned short var_short;
	
	//Get the firmware file info
	fd = open(filename, O_RDONLY, 0700);  
	if (fd == -1)  {  
		fprintf(stderr,"open file %s failed!\n%s\n", filename,strerror(errno));  
		return 10;  
	}  

	fw_size = lseek(fd, 0, SEEK_END);  
	if (fw_size == -1)  
	{  
		fprintf(stderr,"lseek failed!\n%s\n", strerror(errno));  
		close(fd);  
		return 20;  
	}  

	//check firmware size
	if ( fw_size < MIN_FW_SIZE ) {//must > 40KB
		fprintf(stderr,"Error, firmware size: %d Bytes < 40KB\r\n",fw_size);
		close(fd);  
		return 30;  
	}

	//check firmware size
	if ( fw_size > MAX_FW_SIZE-1 ) {//must < 60KB
		fprintf(stderr,"Error, firmware size: %d Bytes> 60KB\r\n",fw_size);
		close(fd);  
		return 30;  
	}

	//check firmware size
	if ( fw_size > MAX_FW_SIZE-1 ) {//must < 60KB
		fprintf(stderr,"Error, firmware size: %d Bytes> 60KB\r\n",fw_size);
		close(fd);  
		return 30;  
	}

	lseek( fd, -20L, SEEK_END );
	fpos = read(fd, rev_info, 20);
	if (fpos == -1)  
	{  
		fprintf(stderr,"File read failed!\n%s\n", strerror(errno));  
		close(fd);  
		return 40;  
	}  

	//replace zero with 0x20
	for ( i = 0 ; i < fpos ; i++ ) {
		if ( rev_info[i] == 0 ) {
			rev_info[i] = 0x20 ;
		}
	}

	rev_pos = strstr(rev_info,"$Rev: ");
	if ( rev_pos == NULL ) { //no found
		fprintf(stderr,"Can't find revision info!\n");
		close(fd);  
		return 4;  
	}

	fw_rev = 0 ;
	conv_rev( rev_pos, &fw_rev);

			bzero( snd_buf, BUFSIZE);
			
			snd_buf[0] = GSM_HEAD ;
			snd_buf[1] = *sequence ;

			if ( snd_buf[1] == 0xFF ) { //Server => client, seq: 0x80~FF
				*sequence = 0x80 ;
			}
			else {
				*sequence = snd_buf[1]+1;//increase seq
			}

			snd_buf[2] = GSM_CMD_UPGRADE ;
			snd_buf[3] =  00;//len high
			snd_buf[4] =  07;//len low

			//00 表示后面跟的数据是硬件版本号(2B)、FW版本号(2B)、FW长度(2B)
			snd_buf[5] =  00 ;

			snd_buf[6] =  00 ; //HW rev
			snd_buf[7] =  00 ;
			
			snd_buf[8] =  (fw_rev >> 8)&0xFF;//Rev high
			snd_buf[9] =  (fw_rev)&0xFF;//Rev low

			snd_buf[10]=  (fw_size >> 8)&0xFF;//Size high
			snd_buf[11]=  (fw_size)&0xFF;//Size low

			//Calc CRC16
			var_short = crc16tablefast(snd_buf , ((snd_buf[3]<<8)|(snd_buf[4]))+5);
			
			snd_buf[12] = (var_short)>>8 ;
			snd_buf[13] = (var_short)&0xFF ;
			
			if ( foreground ) {
				fprintf(stderr, "Send CMD %c : ",snd_buf[2]);
				for ( var_short = 0 ; var_short < ((snd_buf[3]<<8)|(snd_buf[4]))+7 ; var_short++ ) {
					fprintf(stderr, "%02X ",snd_buf[var_short]);
				}
				fprintf(stderr, "to %s\n",rokko->sn);
			}
		
			write(rokko->client_socket,snd_buf,((snd_buf[3]<<8)|(snd_buf[4]))+7);

}

//0:ok, others: err
unsigned char snd_cmd( struct rokko_data *rokko, unsigned char *seq, unsigned char *send_buf )
{//send command to client

	unsigned char cmd = 0 ;			

	//get CMD from DB first, TBD

	cmd = GSM_CMD_UPGRADE ;
	//handle the cmd
	switch ( cmd ) {
	
		case GSM_CMD_UPGRADE: //0x55, 'U', Upgrade firmware
			snd_cmd_upgrade( rokko, seq, send_buf ) ;
			break;

		default:
			fprintf(stderr, "Unknow command: %c(0x%X)\r\n",cmd,cmd);
			break;
	}//end of handle the cmd
	
	return 0 ;
}

