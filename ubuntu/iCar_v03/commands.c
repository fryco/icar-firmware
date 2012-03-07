//$URL: https://icar-firmware.googlecode.com/svn/ubuntu/iCar_v03/commands.c $ 
//$Rev: 99 $, $Date: 2012-02-29 09:32:56 +0800 (Wed, 29 Feb 2012) $

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include "database.h"
#include "commands.h"

extern int debug_flag ;

int cmd_ask_ist( struct icar_data *mycar, struct icar_command * cmd,\
				unsigned char *rec_buf, unsigned char *snd_buf )
{//for case GSM_ASK_IST://0x3F, '?':

	unsigned char new_ist;
	unsigned int chk_count ;

	if ( debug_flag ) {
		fprintf(stderr, "CMD is ask IST...\n");
	}

	if ( strlen(mycar->sn) == 10 ) {//have SN
		if ( ask_instruction(mycar,rec_buf,&new_ist)){//error
			;
		}
		else { //ok, the new instruction save in ist

			//send respond 
			memset(snd_buf, '\0', BUFSIZE);
			snd_buf[0] = GSM_HEAD ;
			snd_buf[1] = cmd->seq ;
			snd_buf[2] = cmd->pcb | 0x80 ;
			snd_buf[3] =  00;//len high
			snd_buf[4] =  01;//len low
			snd_buf[5] =  new_ist;//new instruction

			//Calc chk
			cmd->chk = GSM_HEAD ;
			for ( chk_count = 1 ; chk_count < 1+5 ; chk_count++) {
				cmd->chk ^= snd_buf[chk_count] ;
			}

			snd_buf[6] =  cmd->chk ;

			if ( debug_flag ) {
				fprintf(stderr, "CMD %c ok, will return: ",cmd->pcb);
				for ( chk_count = 0 ; chk_count < 7 ; chk_count++ ) {
					fprintf(stderr, "%02X ",snd_buf[chk_count]);
				}
				fprintf(stderr, "\n");
			}

			write(mycar->client_socket,snd_buf,7);


		}
	}//end of strlen(cmd->pro_sn) == 10
	else { //no SN
		fprintf(stderr, "Please upload SN first!\n");

		//record this command
		if ( record_command(mycar,rec_buf,"NEED_SN",7)) {//error
			if ( debug_flag ) {
				fprintf(stderr, "Log command err= %d: %s",\
					mycar->err_code,mycar->err_msg);
				}
		}

		//send respond 
		memset(snd_buf, '\0', BUFSIZE);
		snd_buf[0] = GSM_HEAD ;
		snd_buf[1] = cmd->seq ;
		snd_buf[2] = cmd->pcb | 0x80 ;
		snd_buf[3] =  00;//len high
		snd_buf[4] =  01;//len low
		snd_buf[5] =  01;//error code, need product SN.

		//Calc chk
		cmd->chk = GSM_HEAD ;
		for ( chk_count = 1 ; chk_count < 1+5 ; chk_count++) {
			cmd->chk ^= snd_buf[chk_count] ;
		}

		snd_buf[6] =  cmd->chk ;

		if ( debug_flag ) {
			fprintf(stderr, "CMD %c error, will return: ",cmd->pcb);
			for ( chk_count = 0 ; chk_count < 7 ; chk_count++ ) {
				fprintf(stderr, "%02X ",snd_buf[chk_count]);
			}
			fprintf(stderr, "\n");
		}

		write(mycar->client_socket,snd_buf,7);
	}
	return 0 ;
}

int cmd_err_log( struct icar_data *mycar, struct icar_command * cmd,\
				unsigned char *rec_buf, unsigned char *snd_buf )
{//case GSM_CMD_ERROR: //0x45,'E' Error, upload error log

	//BKP_DR1, ERR index: 	15~12:reverse 
	//						11~8:reverse
	//						7~4:GPRS disconnect reason
	//						3~0:GSM module poweroff reason
	unsigned char err_idx=0;//1: 3~0, 2:7~4, 3: 11~8, 4:15~12
	unsigned int chk_count ;

	//C9 SEQ 45 LEN DATA CHK

	if ( debug_flag ) {
		fprintf(stderr, "CMD is error log...\n");
	}

	if ( strlen(mycar->sn) == 10 ) {
		if ( record_error(mycar,rec_buf,&err_idx)){//error
			if ( debug_flag ) {
				fprintf(stderr, "Insert error log err= %d: %s",\
					mycar->err_code,mycar->err_msg);
			}

			if ( record_command(mycar,rec_buf,"DB_ERR",7)) {
				if ( debug_flag ) {
					fprintf(stderr, "Insert err CMD err= %d: %s",\
						mycar->err_code,mycar->err_msg);
					}
			}

			//send respond 
			memset(snd_buf, '\0', BUFSIZE);
			snd_buf[0] = GSM_HEAD ;
			snd_buf[1] = cmd->seq ;
			snd_buf[2] = cmd->pcb | 0x80 ;
			snd_buf[3] =  00;//len high
			snd_buf[4] =  01;//len low
			snd_buf[5] =  02;//insert into database error.

			//Calc chk
			cmd->chk = GSM_HEAD ;
			for ( chk_count = 1 ; chk_count < 1+5 ; chk_count++) {
				cmd->chk ^= snd_buf[chk_count] ;
			}

			snd_buf[6] =  cmd->chk ;

			if ( debug_flag ) {
				fprintf(stderr, "CMD: %c error, will send: ",cmd->pcb);
				for ( chk_count = 0 ; chk_count < 7 ; chk_count++ ) {
					fprintf(stderr, "%02X ",snd_buf[chk_count]);
				}
				fprintf(stderr, "to %s\n",mycar->sn);
			}

			write(mycar->client_socket,snd_buf,7);
		}
		else { //ok

			if ( record_command(mycar,rec_buf,"NO_ERR",7)) {//error
				if ( debug_flag ) {
					fprintf(stderr, "Insert err CMD err= %d: %s",\
						mycar->err_code,mycar->err_msg);
					}
			}

			//send respond 
			memset(snd_buf, '\0', BUFSIZE);
			snd_buf[0] = GSM_HEAD ;
			snd_buf[1] = cmd->seq ;
			snd_buf[2] = cmd->pcb | 0x80 ;
			snd_buf[3] =  00;//len high
			snd_buf[4] =  01;//len low
			snd_buf[5] =  00 | (err_idx<<4);//ok

			//Calc chk
			cmd->chk = GSM_HEAD ;
			for ( chk_count = 1 ; chk_count < 1+5 ; chk_count++) {
				cmd->chk ^= snd_buf[chk_count] ;
			}

			snd_buf[6] =  cmd->chk ;

			if ( debug_flag ) {
				fprintf(stderr, "CMD: %c ok, will send: ",cmd->pcb);
				for ( chk_count = 0 ; chk_count < 7 ; chk_count++ ) {
					fprintf(stderr, "%02X ",snd_buf[chk_count]);
				}
				fprintf(stderr, "to %s\n",mycar->sn);
			}

			write(mycar->client_socket,snd_buf,7);
		}
	}//end of strlen(cmd->pro_sn) == 10
	else { //no SN
		fprintf(stderr, "Please upload SN first!\n");

		//record this command
		if ( record_command(mycar,rec_buf,"NEED_SN",7)) {//error
			if ( debug_flag ) {
				fprintf(stderr, "ERR log command err= %d: %s",\
					mycar->err_code,mycar->err_msg);
				}
		}

		//send respond 
		memset(snd_buf, '\0', BUFSIZE);
		snd_buf[0] = GSM_HEAD ;
		snd_buf[1] = cmd->seq ;
		snd_buf[2] = cmd->pcb | 0x80 ;
		snd_buf[3] =  00;//len high
		snd_buf[4] =  01;//len low
		snd_buf[5] =  01;//error code, need product SN.

		//Calc chk
		cmd->chk = GSM_HEAD ;
		for ( chk_count = 1 ; chk_count < 1+5 ; chk_count++) {
			cmd->chk ^= snd_buf[chk_count] ;
		}

		snd_buf[6] =  cmd->chk ;

		if ( debug_flag ) {
			fprintf(stderr, "CMD 0x%02X error, will return: ",cmd->pcb);
			for ( chk_count = 0 ; chk_count < 7 ; chk_count++ ) {
				fprintf(stderr, "%02X ",snd_buf[chk_count]);
			}
			fprintf(stderr, "\n");
		}

		write(mycar->client_socket,snd_buf,7);
	}
	return 0 ;
}

int cmd_rec_signal( struct icar_data *mycar, struct icar_command * cmd,\
				unsigned char *rec_buf, unsigned char *snd_buf )
{//case GSM_CMD_RECORD: //0x52, 'R' Record GSM signal,adc ...

	unsigned int chk_count ;

	//C9 SEQ 52 LEN IP GSM_S Vol CHK

	if ( debug_flag ) {
		fprintf(stderr, "CMD is Record GSM signal...\n");
	}

	if ( strlen(mycar->sn) == 10 ) {

		if ( record_signal(mycar,rec_buf)) {//error
			if ( debug_flag ) {
				fprintf(stderr, "Record GSM signal err= %d: %s",\
					mycar->err_code,mycar->err_msg);
			}

			if ( record_command(mycar,rec_buf,"DB_ERR",7)) {
				if ( debug_flag ) {
					fprintf(stderr, "Record command err= %d: %s",\
						mycar->err_code,mycar->err_msg);
					}
			}

			//send respond 
			memset(snd_buf, '\0', BUFSIZE);
			snd_buf[0] = GSM_HEAD ;
			snd_buf[1] = cmd->seq ;
			snd_buf[2] = cmd->pcb | 0x80 ;
			snd_buf[3] =  00;//len high
			snd_buf[4] =  01;//len low
			snd_buf[5] =  02;//insert into database error.

			//Calc chk
			cmd->chk = GSM_HEAD ;
			for ( chk_count = 1 ; chk_count < 1+5 ; chk_count++) {
				cmd->chk ^= snd_buf[chk_count] ;
			}

			snd_buf[6] =  cmd->chk ;

			if ( debug_flag ) {
				fprintf(stderr, "CMD %c error, will send: ",cmd->pcb);
				for ( chk_count = 0 ; chk_count < 7 ; chk_count++ ) {
					fprintf(stderr, "%02X ",snd_buf[chk_count]);
				}
				fprintf(stderr, "to %s\n",mycar->sn);
			}

			write(mycar->client_socket,snd_buf,7);
		}
		else { //ok

			if ( record_command(mycar,rec_buf,"NO_ERR",7)) {//error
				if ( debug_flag ) {
					fprintf(stderr, "Record command err= %d: %s",\
						mycar->err_code,mycar->err_msg);
					}
			}

			//send respond 
			memset(snd_buf, '\0', BUFSIZE);
			snd_buf[0] = GSM_HEAD ;
			snd_buf[1] = cmd->seq ;
			snd_buf[2] = cmd->pcb | 0x80 ;
			snd_buf[3] =  00;//len high
			snd_buf[4] =  01;//len low
			snd_buf[5] =  00;//ok

			//Calc chk
			cmd->chk = GSM_HEAD ;
			for ( chk_count = 1 ; chk_count < 1+5 ; chk_count++) {
				cmd->chk ^= snd_buf[chk_count] ;
			}

			snd_buf[6] =  cmd->chk ;

			if ( debug_flag ) {
				fprintf(stderr, "CMD %c ok, will send: ",cmd->pcb);
				for ( chk_count = 0 ; chk_count < 7 ; chk_count++ ) {
					fprintf(stderr, "%02X ",snd_buf[chk_count]);
				}
				fprintf(stderr, "to %s\n",mycar->sn);
			}

			write(mycar->client_socket,snd_buf,7);
		}
	}//end of strlen(cmd->pro_sn) == 10
	else { //no SN
		fprintf(stderr, "Please upload SN first!\n");

		//record this command
		if ( record_command(mycar,rec_buf,"NEED_SN",7)) {//error
			if ( debug_flag ) {
				fprintf(stderr, "Record command err= %d: %s",\
					mycar->err_code,mycar->err_msg);
				}
		}

		//send respond 
		memset(snd_buf, '\0', BUFSIZE);
		snd_buf[0] = GSM_HEAD ;
		snd_buf[1] = cmd->seq ;
		snd_buf[2] = cmd->pcb | 0x80 ;
		snd_buf[3] =  00;//len high
		snd_buf[4] =  01;//len low
		snd_buf[5] =  01;//error code, need product SN.

		//Calc chk
		cmd->chk = GSM_HEAD ;
		for ( chk_count = 1 ; chk_count < 1+5 ; chk_count++) {
			cmd->chk ^= snd_buf[chk_count] ;
		}

		snd_buf[6] =  cmd->chk ;

		if ( debug_flag ) {
			fprintf(stderr, "CMD %c error, will return: ",cmd->pcb);
			for ( chk_count = 0 ; chk_count < 7 ; chk_count++ ) {
				fprintf(stderr, "%02X ",snd_buf[chk_count]);
			}
			fprintf(stderr, "\n");
		}

		write(mycar->client_socket,snd_buf,7);
	}
	return 0 ;
}

//0: ok, 1: disconnect immediately
int cmd_sn_upload( struct icar_data *mycar, struct icar_command * cmd,\
				unsigned char *rec_buf, unsigned char *snd_buf )
{//case GSM_CMD_SN: //0x53, 'S', upload SN to server

	unsigned int chk_count ;

	//HEAD SEQ CMD Length(2 bytes) OSTime SN(char 10) IP check
	//C9 01 53 00 1B 00 00 0D 99 //CMD + OSTime
	//30 32 50 31 43 30 44 32 41 37 //SN
	//31 30 2E 32 30 31 2E 31 33 37 2E 32 37 28 //IP+CHK

	if ( strlen(mycar->sn) == 10 ) {
		if ( strncmp(mycar->sn,&rec_buf[9], 10) ) {
			//no same
			//record_error(mycar,&recv_buf[buf_index]); 
//Need change
			fprintf(stderr, "\r\nSN error! First SN: %s, ",\
				mycar->sn);
			strncpy(cmd->pro_sn, &rec_buf[9], 10);
			cmd->pro_sn[10] = 0x0;
			fprintf(stderr, "now is: %s\n\n",mycar->sn);
			return 1 ;
		}
		else {
			fprintf(stderr, "\r\nNo need update ");
		}
	}
	else {	//record the product SN
		strncpy(cmd->pro_sn, &rec_buf[9], 10);
		cmd->pro_sn[10] = 0x0;

		//Update IP to server's DB
		if ( record_ip(mycar,rec_buf)) 
		{//no error
			if ( debug_flag ) {
				fprintf(stderr, "Update GSM IP err= %d: %s",\
					mycar->err_code,mycar->err_msg);
			}
		}
	}
	fprintf(stderr, "SN: %s\t",mycar->sn);
	fprintf(stderr, "cmd->pro_sn: %s\r\n",cmd->pro_sn);

	//record this command
	if ( record_command(mycar,rec_buf,"NO_ERR",10)) {
		if ( debug_flag ) {
			fprintf(stderr, "Record command err= %d: %s",\
				mycar->err_code,mycar->err_msg);
			}
	}

	//send respond 
	memset(snd_buf, '\0', BUFSIZE);
	snd_buf[0] = GSM_HEAD ;
	snd_buf[1] = cmd->seq ;
	snd_buf[2] = cmd->pcb | 0x80 ;
	snd_buf[3] =  00;//len high
	snd_buf[4] =  04;//len low
	snd_buf[5] =  (time(NULL) >> 24)&0xFF;//time high
	snd_buf[6] =  (time(NULL) >> 16)&0xFF;//time high
	snd_buf[7] =  (time(NULL) >> 8)&0xFF;//time low
	snd_buf[8] =  (time(NULL) >> 00)&0xFF;//time low

	//Calc chk
	cmd->chk = GSM_HEAD ;
	for ( chk_count = 1 ; chk_count < 4+5 ; chk_count++) {
		cmd->chk ^= snd_buf[chk_count] ;
		//fprintf(stderr, "%d\t%02X\t%02X\r\n",chk_count,snd_buf[chk_count],cmd->chk);
	}

	snd_buf[9] =  cmd->chk ;

	if ( debug_flag ) {
		fprintf(stderr, "CMD is SN, will send: ");
		for ( chk_count = 0 ; chk_count < 10 ; chk_count++ ) {
			fprintf(stderr, "%02X ",snd_buf[chk_count]);
		}
		fprintf(stderr, "to %s\n",cmd->pro_sn);
	}

	write(mycar->client_socket,snd_buf,10);

	return 0 ;
}

int cmd_get_time( struct icar_data *mycar, struct icar_command * cmd,\
				unsigned char *rec_buf, unsigned char *snd_buf )
{//case GSM_CMD_TIME: //0x54, 'T', Get server Time

	unsigned int chk_count ;

	//C9 A4 54 00 00 23

	if ( debug_flag ) {
		fprintf(stderr, "CMD is Time...\n");
	}

	if ( strlen(mycar->sn) == 10 ) {

		fprintf(stderr, "SN: %s\t",mycar->sn);
		fprintf(stderr, "cmdpro_sn: %s\r\n",cmd->pro_sn);

		//record this command
		if ( record_command(mycar,rec_buf,"NO_ERR",10)) {
			if ( debug_flag ) {
				fprintf(stderr, "Record command err= %d: %s",\
					mycar->err_code,mycar->err_msg);
				}
		}

		//send respond 
		memset(snd_buf, '\0', BUFSIZE);
		snd_buf[0] = GSM_HEAD ;
		snd_buf[1] = cmd->seq ;
		snd_buf[2] = cmd->pcb | 0x80 ;
		snd_buf[3] =  00;//len high
		snd_buf[4] =  04;//len low
		snd_buf[5] =  (time(NULL) >> 24)&0xFF;//time high
		snd_buf[6] =  (time(NULL) >> 16)&0xFF;//time high
		snd_buf[7] =  (time(NULL) >> 8)&0xFF;//time low
		snd_buf[8] =  (time(NULL) >> 00)&0xFF;//time low

		//Calc chk
		cmd->chk = GSM_HEAD ;
		for ( chk_count = 1 ; chk_count < 4+5 ; chk_count++) {
			cmd->chk ^= snd_buf[chk_count] ;
		}

		snd_buf[9] =  cmd->chk ;

		if ( debug_flag ) {
			fprintf(stderr, "CMD is Time, will send: ");
			for ( chk_count = 0 ; chk_count < 10 ; chk_count++ ) {
				fprintf(stderr, "%02X ",snd_buf[chk_count]);
			}
			fprintf(stderr, "to %s\n",cmd->pro_sn);
		}

		write(mycar->client_socket,snd_buf,10);
	}

	else { //no SN
		fprintf(stderr, "Please upload SN first!\n");

		//record this command
		if ( record_command(mycar,rec_buf,"NEED_SN",7)) {//error
			if ( debug_flag ) {
				fprintf(stderr, "Record command err= %d: %s",\
					mycar->err_code,mycar->err_msg);
				}
		}

		//send respond 
		memset(snd_buf, '\0', BUFSIZE);
		snd_buf[0] = GSM_HEAD ;
		snd_buf[1] = cmd->seq ;
		snd_buf[2] = cmd->pcb | 0x80 ;
		snd_buf[3] =  00;//len high
		snd_buf[4] =  01;//len low
		snd_buf[5] =  01;//error code, need product SN.

		//Calc chk
		cmd->chk = GSM_HEAD ;
		for ( chk_count = 1 ; chk_count < 1+5 ; chk_count++) {
			cmd->chk ^= snd_buf[chk_count] ;
			//fprintf(stderr, "%d\t%02X\t%02X\r\n",chk_count,snd_buf[chk_count],cmd->chk);
		}

		snd_buf[6] =  cmd->chk ;

		if ( debug_flag ) {
			fprintf(stderr, "CMD 0x%02X error, will return: ",cmd->pcb);
			for ( chk_count = 0 ; chk_count < 7 ; chk_count++ ) {
				fprintf(stderr, "%02X ",snd_buf[chk_count]);
			}
			fprintf(stderr, "\n");
		}

		write(mycar->client_socket,snd_buf,7);
	}
	return 0 ;
}

int cmd_upgrade_fw( struct icar_data *mycar, struct icar_command * cmd,\
				unsigned char *rec_buf, unsigned char *snd_buf )
{//case GSM_CMD_UPGRADE://0x55, 'U', Upgrade firmware

	unsigned int chk_count ;

	if ( debug_flag ) {
		fprintf(stderr, "CMD is Upgrade...\n");
	}

	if ( strlen(mycar->sn) == 10 ) {

		fprintf(stderr, "SN: %s\t",mycar->sn);
		fprintf(stderr, "cmd->pro_sn: %s\r\n",cmd->pro_sn);

		//record this command
		if ( record_command(mycar,rec_buf,"NO_ERR",10)) {
			if ( debug_flag ) {
				fprintf(stderr, "Record command err= %d: %s",\
					mycar->err_code,mycar->err_msg);
				}
		}

		//if buf[5] > 0xF0, error report from STM32 for upgrade flash


		//Check input detail
		//C9 F9 55 00 04 00 00 00 5E
		//buf[5] = 0x00 ;//00: mean buf[6] is hw rev, others: block seq

//把所有数据显示出来
fprintf(stderr, "Rec: ");
for ( chk_count = 0 ; chk_count < rec_buf[4]+6 ; chk_count++ ) {
	fprintf(stderr, "%02X ",rec_buf[chk_count]);
}

		memset(snd_buf, '\0', BUFSIZE);
		if ( rec_buf[5] == 0 ) { //HW,FW info
			if ( debug_flag ) {
				fprintf(stderr, "\r\nHW rev: %d, FW rev: %d\r\n",\
					rec_buf[6],rec_buf[7]<<8 | rec_buf[8]);
			}

			//send respond : FW rev&size
			//C9 57 D5 00 05 00 data
			//05: data len, 00: data is latest firmware revision(u16) + size(u16)
			snd_buf[0] = GSM_HEAD ;
			snd_buf[1] = cmd->seq ;
			snd_buf[2] = cmd->pcb | 0x80 ;
			snd_buf[3] =  00;//len high
			snd_buf[4] =  05;//len low
			snd_buf[5] =  00;
			snd_buf[6] =  0x00;//Rev high
			snd_buf[7] =  0x7D;//Rev low
			snd_buf[8] =  0xE4;//Size high
			snd_buf[9] =  0x03;//Size low
		}
		else {//others : block seq
			fprintf(stderr, "\r\nAsk Block %d, FW rev: %d\r\n",\
					rec_buf[5]-1,rec_buf[6]<<8 | rec_buf[7]);

			//send respond : Block data
			//C9 57 D5 00 xx yy FW_Rev(2 Bytes) + data + CRC
			//xx: data len, yy: block seq, data: block data
			snd_buf[0] = GSM_HEAD ;
			snd_buf[1] = cmd->seq ;
			snd_buf[2] = cmd->pcb | 0x80 ;

			snd_buf[5] =  rec_buf[5];//block seq
			snd_buf[6] =  0x00;//Rev high, get Rev. info from binary
			snd_buf[7] =  0x7D;//Rev low

			//Block data
			//data len
			snd_buf[3] =  04;//len high
			snd_buf[4] =  32+3;//len low, > 3

			for ( chk_count = 0 ; chk_count < ((snd_buf[3])<<8)+snd_buf[4]-3 ; chk_count++) {
				snd_buf[8+chk_count]= chk_count ;
			}

			fprintf(stderr, "chk_count: %d,  Len: %d\r\n",\
				chk_count,((snd_buf[3])<<8)+snd_buf[4]);

		}

		//Calc chk
		cmd->chk = GSM_HEAD ;
		for ( chk_count = 1 ; chk_count < ((snd_buf[3])<<8)+snd_buf[4]+5 ; chk_count++) {
			cmd->chk ^= snd_buf[chk_count] ;
		}

		snd_buf[chk_count] =  cmd->chk ;

		if ( debug_flag ) {
			fprintf(stderr, "CMD %c ok, will return: ",cmd->pcb);
			for ( chk_count = 0 ; chk_count < ((snd_buf[3])<<8)+snd_buf[4]+6 ; chk_count++ ) {
				fprintf(stderr, "%02X ",snd_buf[chk_count]);
			}
			fprintf(stderr, "to %s\n",cmd->pro_sn);
		}

		write(mycar->client_socket,snd_buf,((snd_buf[3])<<8)+snd_buf[4]+6);
	}

	else { //no SN
		fprintf(stderr, "Please upload SN first!\n");

		//record this command
		if ( record_command(mycar,rec_buf,"NEED_SN",7)) {//error
			if ( debug_flag ) {
				fprintf(stderr, "Record command err= %d: %s",\
					mycar->err_code,mycar->err_msg);
				}
		}

		//send respond 
		memset(snd_buf, '\0', BUFSIZE);
		snd_buf[0] = GSM_HEAD ;
		snd_buf[1] = cmd->seq ;
		snd_buf[2] = cmd->pcb | 0x80 ;
		snd_buf[3] =  00;//len high
		snd_buf[4] =  01;//len low
		snd_buf[5] =  01;//error code, need product SN.

		//Calc chk
		cmd->chk = GSM_HEAD ;
		for ( chk_count = 1 ; chk_count < 1+5 ; chk_count++) {
			cmd->chk ^= snd_buf[chk_count] ;
			//fprintf(stderr, "%d\t%02X\t%02X\r\n",chk_count,snd_buf[chk_count],cmd->chk);
		}

		snd_buf[6] =  cmd->chk ;

		if ( debug_flag ) {
			fprintf(stderr, "CMD %c error, will return: ",cmd->pcb);
			for ( chk_count = 0 ; chk_count < 7 ; chk_count++ ) {
				fprintf(stderr, "%02X ",snd_buf[chk_count]);
			}
			fprintf(stderr, "\n");
		}

		write(mycar->client_socket,snd_buf,7);
	}
	return 0 ;
}
