//$URL$ 
//$Rev$, $Date: 2012-02-29 09:32:56 +0800 (Wed, 29 Feb 2012) $

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
}

