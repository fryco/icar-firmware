//$URL: https://rokko-firmware.googlecode.com/svn/ubuntu/iCar_v03/commands.c $ 
//$Rev: 99 $, $Date: 2012-02-29 09:32:56 +0800 (Wed, 29 Feb 2012) $

#include "config.h"
#include "rokkod.h"

extern unsigned int foreground;

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
			snd_buf[5] =  00;//return status:0£º³É¹¦
			snd_buf[6] =  (time(NULL) >> 24)&0xFF;//time high
			snd_buf[7] =  (time(NULL) >> 16)&0xFF;//time high
			snd_buf[8] =  (time(NULL) >> 8)&0xFF;//time low
			snd_buf[9] =  (time(NULL) >> 00)&0xFF;//time low
		
			//MCU ID£º12 B, from DB, for verify server
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
			snd_buf[5] =  01;// µÇÂ½Ê§°Ü
		
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

//0:ok, others: err
unsigned char snd_cmd( struct rokko_data *rokko, unsigned char *sequence, unsigned char *snd_buf )
{//send command to client

	unsigned short var_short;
			
			bzero( snd_buf, BUFSIZE);
			
			snd_buf[0] = GSM_HEAD ;
			snd_buf[1] = *sequence ;

			if ( snd_buf[1] == 0xFF ) { //Server => client, seq: 0x80~FF
				*sequence = 0x80 ;
			}
			else {
				*sequence = snd_buf[1]+1;//increase seq
			}

			snd_buf[2] = GSM_CMD_NEW ;
			snd_buf[3] =  00;//len high
			snd_buf[4] =  01;//len low

			snd_buf[5] = GSM_CMD_UPGRADE ;
			
			//Calc CRC16
			var_short = crc16tablefast(snd_buf , ((snd_buf[3]<<8)|(snd_buf[4]))+5);
			
			snd_buf[6] = (var_short)>>8 ;
			snd_buf[7] = (var_short)&0xFF ;
			
			if ( foreground ) {
				fprintf(stderr, "Ask new data: ");
				for ( var_short = 0 ; var_short < ((snd_buf[3]<<8)|(snd_buf[4]))+7 ; var_short++ ) {
					fprintf(stderr, "%02X ",snd_buf[var_short]);
				}
				fprintf(stderr, "to %s\n",rokko->sn);
			}
		
			write(rokko->client_socket,snd_buf,((snd_buf[3]<<8)|(snd_buf[4]))+7);


}	