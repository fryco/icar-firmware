//$URL: https://icar-firmware.googlecode.com/svn/ubuntu/iCar_v03/commands.c $ 
//$Rev: 99 $, $Date: 2012-02-29 09:32:56 +0800 (Wed, 29 Feb 2012) $

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include "database.h"
#include "commands.h"
#include "cloud_post.h"

#define OS_TICKS_PER_SEC	100

const char *cloud_host="cn0086.info";
const char *log_host="127.0.0.1";
//Forum id:
//'36' ==> Machine
//'37' ==> Instruction 	
//'38' ==> Signal 	
//'39' ==> Sync time
//'40' ==> Login
//'41' ==> Log err
//'42' ==> Upgrade firmware / Update parameter
//'43' ==> 

extern int debug_flag ;

// http://www.zorc.breitbandkatze.de/crctester.c
//需要设置crcxor = 0x00000000;refin = 0;refout = 0;便可以得到和STM32内置硬件CRC32一致的结果
// CRC parameters (default values are for CRC-32):
const int order = 32;
const unsigned long polynom = 0x4c11db7;

// 'order' [1..32] is the CRC polynom order, counted without the leading '1' bit
// 'polynom' is the CRC polynom without leading '1' bit

static const unsigned int crctab[ 256 ] = 
{ 
    0x00000000, 0x04C11DB7, 0x09823B6E, 0x0D4326D9, 0x130476DC, 0x17C56B6B, 0x1A864DB2, 0x1E475005, 
    0x2608EDB8, 0x22C9F00F, 0x2F8AD6D6, 0x2B4BCB61, 0x350C9B64, 0x31CD86D3, 0x3C8EA00A, 0x384FBDBD, 
    0x4C11DB70, 0x48D0C6C7, 0x4593E01E, 0x4152FDA9, 0x5F15ADAC, 0x5BD4B01B, 0x569796C2, 0x52568B75, 
    0x6A1936C8, 0x6ED82B7F, 0x639B0DA6, 0x675A1011, 0x791D4014, 0x7DDC5DA3, 0x709F7B7A, 0x745E66CD, 
    0x9823B6E0, 0x9CE2AB57, 0x91A18D8E, 0x95609039, 0x8B27C03C, 0x8FE6DD8B, 0x82A5FB52, 0x8664E6E5, 
    0xBE2B5B58, 0xBAEA46EF, 0xB7A96036, 0xB3687D81, 0xAD2F2D84, 0xA9EE3033, 0xA4AD16EA, 0xA06C0B5D, 
    0xD4326D90, 0xD0F37027, 0xDDB056FE, 0xD9714B49, 0xC7361B4C, 0xC3F706FB, 0xCEB42022, 0xCA753D95, 
    0xF23A8028, 0xF6FB9D9F, 0xFBB8BB46, 0xFF79A6F1, 0xE13EF6F4, 0xE5FFEB43, 0xE8BCCD9A, 0xEC7DD02D, 
    0x34867077, 0x30476DC0, 0x3D044B19, 0x39C556AE, 0x278206AB, 0x23431B1C, 0x2E003DC5, 0x2AC12072, 
    0x128E9DCF, 0x164F8078, 0x1B0CA6A1, 0x1FCDBB16, 0x018AEB13, 0x054BF6A4, 0x0808D07D, 0x0CC9CDCA, 
    0x7897AB07, 0x7C56B6B0, 0x71159069, 0x75D48DDE, 0x6B93DDDB, 0x6F52C06C, 0x6211E6B5, 0x66D0FB02, 
    0x5E9F46BF, 0x5A5E5B08, 0x571D7DD1, 0x53DC6066, 0x4D9B3063, 0x495A2DD4, 0x44190B0D, 0x40D816BA, 
    0xACA5C697, 0xA864DB20, 0xA527FDF9, 0xA1E6E04E, 0xBFA1B04B, 0xBB60ADFC, 0xB6238B25, 0xB2E29692, 
    0x8AAD2B2F, 0x8E6C3698, 0x832F1041, 0x87EE0DF6, 0x99A95DF3, 0x9D684044, 0x902B669D, 0x94EA7B2A, 
    0xE0B41DE7, 0xE4750050, 0xE9362689, 0xEDF73B3E, 0xF3B06B3B, 0xF771768C, 0xFA325055, 0xFEF34DE2, 
    0xC6BCF05F, 0xC27DEDE8, 0xCF3ECB31, 0xCBFFD686, 0xD5B88683, 0xD1799B34, 0xDC3ABDED, 0xD8FBA05A, 
    0x690CE0EE, 0x6DCDFD59, 0x608EDB80, 0x644FC637, 0x7A089632, 0x7EC98B85, 0x738AAD5C, 0x774BB0EB, 
    0x4F040D56, 0x4BC510E1, 0x46863638, 0x42472B8F, 0x5C007B8A, 0x58C1663D, 0x558240E4, 0x51435D53, 
    0x251D3B9E, 0x21DC2629, 0x2C9F00F0, 0x285E1D47, 0x36194D42, 0x32D850F5, 0x3F9B762C, 0x3B5A6B9B, 
    0x0315D626, 0x07D4CB91, 0x0A97ED48, 0x0E56F0FF, 0x1011A0FA, 0x14D0BD4D, 0x19939B94, 0x1D528623, 
    0xF12F560E, 0xF5EE4BB9, 0xF8AD6D60, 0xFC6C70D7, 0xE22B20D2, 0xE6EA3D65, 0xEBA91BBC, 0xEF68060B, 
    0xD727BBB6, 0xD3E6A601, 0xDEA580D8, 0xDA649D6F, 0xC423CD6A, 0xC0E2D0DD, 0xCDA1F604, 0xC960EBB3, 
    0xBD3E8D7E, 0xB9FF90C9, 0xB4BCB610, 0xB07DABA7, 0xAE3AFBA2, 0xAAFBE615, 0xA7B8C0CC, 0xA379DD7B, 
    0x9B3660C6, 0x9FF77D71, 0x92B45BA8, 0x9675461F, 0x8832161A, 0x8CF30BAD, 0x81B02D74, 0x857130C3, 
    0x5D8A9099, 0x594B8D2E, 0x5408ABF7, 0x50C9B640, 0x4E8EE645, 0x4A4FFBF2, 0x470CDD2B, 0x43CDC09C, 
    0x7B827D21, 0x7F436096, 0x7200464F, 0x76C15BF8, 0x68860BFD, 0x6C47164A, 0x61043093, 0x65C52D24, 
    0x119B4BE9, 0x155A565E, 0x18197087, 0x1CD86D30, 0x029F3D35, 0x065E2082, 0x0B1D065B, 0x0FDC1BEC, 
    0x3793A651, 0x3352BBE6, 0x3E119D3F, 0x3AD08088, 0x2497D08D, 0x2056CD3A, 0x2D15EBE3, 0x29D4F654, 
    0xC5A92679, 0xC1683BCE, 0xCC2B1D17, 0xC8EA00A0, 0xD6AD50A5, 0xD26C4D12, 0xDF2F6BCB, 0xDBEE767C, 
    0xE3A1CBC1, 0xE760D676, 0xEA23F0AF, 0xEEE2ED18, 0xF0A5BD1D, 0xF464A0AA, 0xF9278673, 0xFDE69BC4, 
    0x89B8FD09, 0x8D79E0BE, 0x803AC667, 0x84FBDBD0, 0x9ABC8BD5, 0x9E7D9662, 0x933EB0BB, 0x97FFAD0C, 
    0xAFB010B1, 0xAB710D06, 0xA6322BDF, 0xA2F33668, 0xBCB4666D, 0xB8757BDA, 0xB5365D03, 0xB1F740B4 
}; 


unsigned int crctablefast (unsigned char* p, unsigned long len) {

	// fast lookup table algorithm without augmented zero bytes, e.g. used in pkzip.
	// only usable with polynom orders of 8, 16, 24 or 32.

	unsigned long crc = 0xFFFFFFFF;

	while (len--) {
		//printf("%02X = %X\t",p,*p);
		crc = (crc << 8) ^ crctab[ ((crc >> (order-8)) & 0xff) ^ *p++];
		//printf("%08X \r\n",crc);
	}

	crc^= 0x00000000;
	crc&= 0xFFFFFFFF;

	return(crc);
}

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

int cmd_ask_ist( struct icar_data *mycar, struct icar_command * cmd,\
				unsigned char *rec_buf, unsigned char *snd_buf )
{//for case GSM_ASK_IST://0x3F, '?':

	unsigned char new_ist;
	unsigned int chk_count ;
	pid_t cloud_pid;
	unsigned char post_buf[BUFSIZE];

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

			if ( new_ist ) {
				//Create new process (non-block) for cloud post
				cloud_pid = fork();
				if (cloud_pid == 0) { //In child process
					//fprintf(stderr, "In child:%d for cloud post\n",getpid());

					sprintf(post_buf,"ip=%s&fid=37&subject=%s => Ask instruction&message=New instruction is %c\r\n\r\nip: %s",\
							(char *)inet_ntoa(mycar->client_addr.sin_addr),\
							mycar->sn,new_ist,\
							(char *)inet_ntoa(mycar->client_addr.sin_addr));
	
					cloud_post( cloud_host, &post_buf, 80 );
					cloud_post( log_host, &post_buf, 86 );
					exit( 0 );
				}
				else {//In parent process
					//fprintf(stderr, "In parent:%d and return\n",getpid());
					return 0 ;
				}
			}
			else { //no new instruction
				return 0 ;
			}
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

		//Create new process (non-block) for cloud post
		cloud_pid = fork();
		if (cloud_pid == 0) { //In child process

			sprintf(post_buf,"ip=%s&fid=41&subject=Need SN&message=Client ip: %s",\
					(char *)inet_ntoa(mycar->client_addr.sin_addr),\
					(char *)inet_ntoa(mycar->client_addr.sin_addr));

			cloud_post( cloud_host, &post_buf, 80 );
			cloud_post( log_host, &post_buf, 86 );
			exit( 0 );
		}
	}
	return 0 ;
}

int cmd_err_log( struct icar_data *mycar, struct icar_command * cmd,\
				unsigned char *rec_buf, unsigned char *snd_buf )
{//case GSM_CMD_ERROR: //0x45,'E' Error, upload error log

	//BKP_DR1, ERR index: 	15~12:MCU reset 
	//						11~8:upgrade fw failure code
	//						7~4:GPRS disconnect reason
	//						3~0:GSM module poweroff reason
	unsigned char err_idx=0;//1: 3~0, 2:7~4, 3: 11~8, 4:15~12
	unsigned int chk_count ;
	pid_t cloud_pid;
	unsigned char post_buf[BUFSIZE];

	//C9 SEQ 45 LEN DATA CHK

	if ( debug_flag ) {
		fprintf(stderr, "CMD is error log...\n");
	}

	if ( strlen(mycar->sn) == 10 ) {
		if ( record_error(mycar,rec_buf,&err_idx,post_buf)){//error
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

			//Create new process (non-block) for cloud post
			cloud_pid = fork();
			if (cloud_pid == 0) { //In child process
				//fprintf(stderr, "In child:%d for cloud post\n",getpid());

				//cloud_post( cloud_host, &post_buf, 80 );
				cloud_post( log_host, &post_buf, 86 );
				exit( 0 );
			}
			else {//In parent process
				//fprintf(stderr, "In parent:%d and return\n",getpid());
				return 0 ;
			}
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

		//Create new process (non-block) for cloud post
		cloud_pid = fork();
		if (cloud_pid == 0) { //In child process

			sprintf(post_buf,"ip=%s&fid=41&subject=Need SN&message=Client ip: %s",\
					(char *)inet_ntoa(mycar->client_addr.sin_addr),\
					(char *)inet_ntoa(mycar->client_addr.sin_addr));

			cloud_post( cloud_host, &post_buf, 80 );
			cloud_post( log_host, &post_buf, 86 );
			exit( 0 );
		}
	}
	return 0 ;
}

int cmd_rec_signal( struct icar_data *mycar, struct icar_command * cmd,\
				unsigned char *rec_buf, unsigned char *snd_buf )
{//case GSM_CMD_RECORD: //0x52, 'R' Record GSM signal,adc ...

	unsigned int chk_count ;
	pid_t cloud_pid;
	unsigned char post_buf[BUFSIZE];

	//C9 SEQ 52 LEN IP GSM_S Vol CHK

	if ( debug_flag ) {
		fprintf(stderr, "CMD is Record GSM signal...\n");
	}

	if ( strlen(mycar->sn) == 10 ) {

		if ( record_signal(mycar,rec_buf,post_buf)) {//error
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

			//Create new process (non-block) for cloud post
			cloud_pid = fork();
			if (cloud_pid == 0) { //In child process

				cloud_post( cloud_host, &post_buf, 80 );
				cloud_post( log_host, &post_buf, 86 );
				exit( 0 );
			}
			else {//In parent process
				//fprintf(stderr, "In parent:%d and return\n",getpid());
				return 0 ;
			}
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

		//Create new process (non-block) for cloud post
		cloud_pid = fork();
		if (cloud_pid == 0) { //In child process

			sprintf(post_buf,"ip=%s&fid=41&subject=Need SN&message=Client ip: %s",\
					(char *)inet_ntoa(mycar->client_addr.sin_addr),\
					(char *)inet_ntoa(mycar->client_addr.sin_addr));

			cloud_post( cloud_host, &post_buf, 80 );
			cloud_post( log_host, &post_buf, 86 );
			exit( 0 );
		}
	}
	return 0 ;
}

//0: ok, 1: disconnect immediately
int cmd_sn_upload( struct icar_data *mycar, struct icar_command * cmd,\
				unsigned char *rec_buf, unsigned char *snd_buf )
{//case GSM_CMD_SN: //0x53, 'S', upload SN to server

	unsigned int chk_count ;
	pid_t cloud_pid;
	unsigned char post_buf[BUFSIZE];

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
		if ( record_ip(mycar,rec_buf,post_buf)) 
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

	//Create new process (non-block) for cloud post
	cloud_pid = fork();
	if (cloud_pid == 0) { //In child process

		cloud_post( cloud_host, &post_buf, 80 );
		cloud_post( log_host, &post_buf, 86 );
		exit( 0 );
	}
	else {//In parent process
		//fprintf(stderr, "In parent:%d and return\n",getpid());
		return 0 ;
	}
}

int cmd_get_time( struct icar_data *mycar, struct icar_command * cmd,\
				unsigned char *rec_buf, unsigned char *snd_buf )
{//case GSM_CMD_TIME: //0x54, 'T', Get server Time

	unsigned int chk_count ;
	pid_t cloud_pid;
	unsigned char post_buf[BUFSIZE];

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

		//Create new process (non-block) for cloud post
		cloud_pid = fork();
		if (cloud_pid == 0) { //In child process
			//fprintf(stderr, "In child:%d for cloud post\n",getpid());

			sprintf(post_buf,"ip=%s&fid=39&subject=%s => Sync time&message=Synchronize server time to client.\r\n\
					\r\nip: %s",(char *)inet_ntoa(mycar->client_addr.sin_addr),\
					mycar->sn,(char *)inet_ntoa(mycar->client_addr.sin_addr));

			cloud_post( cloud_host, &post_buf, 80 );
			cloud_post( log_host, &post_buf, 86 );
			exit( 0 );
		}
		else {//In parent process
			//fprintf(stderr, "In parent:%d and return\n",getpid());
			return 0 ;
		}
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

		//Create new process (non-block) for cloud post
		cloud_pid = fork();
		if (cloud_pid == 0) { //In child process

			sprintf(post_buf,"ip=%s&fid=41&subject=Need SN&message=Client ip: %s",\
					(char *)inet_ntoa(mycar->client_addr.sin_addr),\
					(char *)inet_ntoa(mycar->client_addr.sin_addr));

			cloud_post( cloud_host, &post_buf, 80 );
			cloud_post( log_host, &post_buf, 86 );
			exit( 0 );
		}
	}
	return 0 ;
}

//0: ok, others: error
int cmd_upgrade_fw( struct icar_data *mycar, struct icar_command * cmd,\
				unsigned char *rec_buf, unsigned char *snd_buf )
{//case GSM_CMD_UPGRADE://0x55, 'U', Upgrade firmware

	int fd;
	unsigned int i, chk_count , data_len, fpos, fw_size, fw_rev;
	unsigned char *filename="./fw/stm32_v00/20120608.bin";
	unsigned char rev_info[MAX_FW_SIZE], *rev_pos;
	pid_t cloud_pid;
	unsigned char post_buf[BUFSIZE];

	if ( debug_flag ) {
		fprintf(stderr, "CMD is Upgrade fw...\n");
	}

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
	if ( debug_flag ) {
	    fprintf(stderr,"File: %s size is:%d Rev:%d\n", filename,fw_size,fw_rev);
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
		//TBD

		//Check input detail
		//C9 F9 55 00 04 00 00 00 5E
		//buf[5] = 0x00 ;//00: mean buf[6] is hw rev, others: block seq

		memset(snd_buf, '\0', BUFSIZE);
		if ( rec_buf[5] == 0 ) { //HW,FW info
			if ( debug_flag ) {
				fprintf(stderr, "Current HW rev: %d, FW rev: %d\r\n",\
					rec_buf[6],rec_buf[7]<<8 | rec_buf[8]);
			}

			//send respond : FW rev&size
			//C9 57 D5 00 05 00 data
			//05: data len, 00: data is latest firmware revision(u16) + size(u16)
			snd_buf[0] = GSM_HEAD ;
			snd_buf[1] = cmd->seq ;
			snd_buf[2] = cmd->pcb | 0x80 ;
			snd_buf[3] =  00;//len high
			snd_buf[4] =  9;//len low
			snd_buf[5] =  00;
			snd_buf[6] =  (fw_rev >> 8)&0xFF;//Rev high
			snd_buf[7] =  (fw_rev)&0xFF;//Rev low
			snd_buf[8] =  (fw_size >> 8)&0xFF;//Size high
			snd_buf[9] =  (fw_size)&0xFF;//Size low

			//generate FW CRC:
			lseek( fd, 0, SEEK_SET );
			memset(rev_info, '\0', MAX_FW_SIZE);
			fpos = read(fd, rev_info, MAX_FW_SIZE);

			//data align 4
			if ((fpos)%4) {
				for ( chk_count = 0 ; chk_count < (4 - ((fpos)%4)) ; chk_count++) {
					rev_info[chk_count+fpos]= 0xFF ;
					fprintf(stderr, "rev_info[%d] = %X\r\n",\
							chk_count+fpos,rev_info[chk_count+fpos]);
				}
				fpos = chk_count+fpos;
				fprintf(stderr, "Add %d Byte for data align. %s: %d\r\n",\
					chk_count,__FILE__,__LINE__);
			}

			chk_count = crctablefast(rev_info,fpos);

			snd_buf[10] = (chk_count >> 24) & 0xFF ;
			snd_buf[11] = (chk_count >> 16) & 0xFF ;
			snd_buf[12] = (chk_count >> 8) & 0xFF ;
			snd_buf[13] = (chk_count) & 0xFF ;

			if ( debug_flag ) {
				fprintf(stderr, "Read %d Bytes, CRC: %08X\r\n",fpos,chk_count);
			}

			//Create new process (non-block) for cloud post
			cloud_pid = fork();
			if (cloud_pid == 0) { //In child process
	
				sprintf(post_buf,"ip=%s&fid=42&subject=%s => Upgrade, firmware info&message=Current HW rev: %d,  FW rev: %d\r\n\
						\r\nNew firmware rev: %d,  size: %d \r\n\r\nip: %s",\
						(char *)inet_ntoa(mycar->client_addr.sin_addr),\
						mycar->sn,rec_buf[6],rec_buf[7]<<8 | rec_buf[8],fw_rev,fw_size,\
						(char *)inet_ntoa(mycar->client_addr.sin_addr));
	
				cloud_post( cloud_host, &post_buf, 80 );
				cloud_post( log_host, &post_buf, 86 );
				exit( 0 );
			}
		}
		else {//others : block seq
			fprintf(stderr, "Ask Block %d, FW rev: %d  \t",\
					rec_buf[5],rec_buf[6]<<8 | rec_buf[7]);

			//send respond : Block data
			//C9 57 D5 00 xx yy FW_Rev(2 Bytes) + data + CRC
			//xx: data len, yy: block seq, data: block data
			snd_buf[0] = GSM_HEAD ;
			snd_buf[1] = cmd->seq ;
			snd_buf[2] = cmd->pcb | 0x80 ;

			snd_buf[5] =  rec_buf[5];//block seq

			snd_buf[6] =  (fw_rev >> 8)&0xFF;//Rev high
			snd_buf[7] =  (fw_rev)&0xFF;//Rev low

			//Block data, Max data len is 1K!!!
			//read fw according to block seq
			lseek( fd, (rec_buf[5]-1)*1024, SEEK_SET );
			fpos = read(fd, &snd_buf[8], 1*1024);
			data_len = fpos + 3 ;//include blk seq, rev info

			fprintf(stderr, "Read: %d, BLK: %d\r\n",fpos,rec_buf[5]);

			//simu data, for test only
			//for ( chk_count = 0 ; chk_count < (data_len-3); chk_count++) {
				//snd_buf[8+chk_count]= chk_count+9 ;
			//}

			//data align 4
			if ((data_len-3)%4) {
				for ( chk_count = 0 ; chk_count < (4 - ((data_len-3)%4)) ; chk_count++) {
					snd_buf[5+chk_count+data_len]= 0xFF ;
					fprintf(stderr, "snd_buf[%d] = %X\r\n",\
							5+chk_count+data_len,snd_buf[5+chk_count+data_len]);
				}
				data_len = chk_count+data_len;
			}

			//Calc CRC: C9 F3 D5 00 07 01 00 7D 00 01 FF FF 95
			chk_count = crctablefast(&snd_buf[8],data_len-3);

			snd_buf[data_len+5] = (chk_count >> 24) & 0xFF ;
			snd_buf[data_len+6] = (chk_count >> 16) & 0xFF ;
			snd_buf[data_len+7] = (chk_count >> 8) & 0xFF ;
			snd_buf[data_len+8] = (chk_count) & 0xFF ;

			//update len
			snd_buf[3] = ((data_len+4) >> 8) & 0xFF;
			snd_buf[4] = ((data_len+4) ) & 0xFF;

			//Create new process (non-block) for cloud post
			cloud_pid = fork();
			if (cloud_pid == 0) { //In child process
	
				sprintf(post_buf,"ip=%s&fid=42&subject=%s => Upgrade, Block %d&message=Sending block: %d,  data length: %d\r\n\
						\r\nNew firmware rev: %d,  size: %d \r\n\r\nip: %s",\
						(char *)inet_ntoa(mycar->client_addr.sin_addr),\
						mycar->sn,rec_buf[5],rec_buf[5],data_len-3,fw_rev,fw_size,\
						(char *)inet_ntoa(mycar->client_addr.sin_addr));
	
				cloud_post( cloud_host, &post_buf, 80 );
				cloud_post( log_host, &post_buf, 86 );
				exit( 0 );
			}
		}

		//Calc chk
		data_len = ((snd_buf[3])<<8) | snd_buf[4] ;
		cmd->chk = GSM_HEAD ;
		for ( chk_count = 1 ; chk_count < data_len+5 ; chk_count++) {
			cmd->chk ^= snd_buf[chk_count] ;
		}

		snd_buf[chk_count] =  cmd->chk ;

		if ( debug_flag ) {
			fprintf(stderr, "CMD %c ok, will return: ",cmd->pcb);
			if ( data_len < 128 ) {
				for ( chk_count = 0 ; chk_count < data_len+6 ; chk_count++ ) {
					fprintf(stderr, "%02X ",snd_buf[chk_count]);
				}
			}
			else {
				for ( chk_count = data_len - 8 ; chk_count < data_len+6 ; chk_count++ ) {
					fprintf(stderr, "%02X ",snd_buf[chk_count]);
				}
			}
			fprintf(stderr, "to %s\n",cmd->pro_sn);
		} 

		write(mycar->client_socket,snd_buf,data_len+6);
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

		//Create new process (non-block) for cloud post
		cloud_pid = fork();
		if (cloud_pid == 0) { //In child process

			sprintf(post_buf,"ip=%s&fid=41&subject=Need SN&message=Client ip: %s",\
					(char *)inet_ntoa(mycar->client_addr.sin_addr),\
					(char *)inet_ntoa(mycar->client_addr.sin_addr));

			cloud_post( cloud_host, &post_buf, 80 );
			cloud_post( log_host, &post_buf, 86 );
			exit( 0 );
		}
	}

	close(fd);
	return 0 ;
}

//0: ok, others: error
int cmd_update_para( struct icar_data *mycar, struct icar_command * cmd,\
				unsigned char *rec_buf, unsigned char *snd_buf )
{//case GSM_CMD_UPDATE://0x75, 'u', Update parameter

	unsigned int i, chk_count , data_len, var_u32;
	pid_t cloud_pid;
	unsigned char post_buf[BUFSIZE];

	//HEAD SEQ PCB Length(2 bytes) HW rev + FW rev + xx(1 byte) check
	//xx: 01: 1st page para, 02: 2nd page para

	if ( debug_flag ) {
		fprintf(stderr, "CMD is Update parameter...\n");
	}

	if ( strlen(mycar->sn) == 10 ) {

		fprintf(stderr, "SN: %s\t",mycar->sn);
		fprintf(stderr, "cmd->pro_sn: %s\r\n",cmd->pro_sn);

		//record this command
		if ( record_command(mycar,rec_buf,"NO_ERR",7)) {
			if ( debug_flag ) {
				fprintf(stderr, "Record command err= %d: %s",\
					mycar->err_code,mycar->err_msg);
				}
		}

		//1，从数据库取需要更新的参数
		//2，按照偏移量逐一填写好，如 01 xx xx xx xx 表示offset 01的参数，数值是xxxxxxxx
		//3, 数据传回MCU后，按此格式逐一解析，并保存。
		//4, Offset 与 drv_flash.h 保持一致
		//Offset 摘要：
		//#define PARA_RELAY_ON				4/4=1
		//#define PARA_OBD_TYPE				32/4=8

		if ( debug_flag ) {
			fprintf(stderr, "Current HW rev: %d, FW rev: %d\r\n",\
				rec_buf[5],rec_buf[6]<<8 | rec_buf[7]);
		}

		memset(snd_buf, '\0', BUFSIZE);

			snd_buf[0] = GSM_HEAD ;
			snd_buf[1] = cmd->seq ;
			snd_buf[2] = cmd->pcb | 0x80 ;
			snd_buf[3] = 0;
			snd_buf[4] = 5;//data len

			snd_buf[5] = 1;//PARA_RELAY_ON
			var_u32 = 3*60*OS_TICKS_PER_SEC;// 3 mins
			snd_buf[6] = (var_u32>>24)&0xFF;
			snd_buf[7] = (var_u32>>16)&0xFF;
			snd_buf[8] = (var_u32>>8)&0xFF;
			snd_buf[9] = (var_u32)&0xFF;

		//Calc chk
		data_len = ((snd_buf[3])<<8) | snd_buf[4] ;
		cmd->chk = GSM_HEAD ;
		for ( chk_count = 1 ; chk_count < data_len+5 ; chk_count++) {
			cmd->chk ^= snd_buf[chk_count] ;
		}

		snd_buf[chk_count] =  cmd->chk ;

		if ( debug_flag ) {
			fprintf(stderr, "CMD %c ok, will return: ",cmd->pcb);
			if ( data_len < 128 ) {
				for ( chk_count = 0 ; chk_count < data_len+6 ; chk_count++ ) {
					fprintf(stderr, "%02X ",snd_buf[chk_count]);
				}
			}
			else {
				for ( chk_count = data_len - 8 ; chk_count < data_len+6 ; chk_count++ ) {
					fprintf(stderr, "%02X ",snd_buf[chk_count]);
				}
			}
			fprintf(stderr, "to %s\n",cmd->pro_sn);
		} 

		write(mycar->client_socket,snd_buf,data_len+6);

		//Create new process (non-block) for cloud post
		cloud_pid = fork();
		if (cloud_pid == 0) { //In child process

			sprintf(post_buf,"ip=%s&fid=42&subject=%s => Update parameter&message=Current HW rev: %d,  FW rev: %d\r\n\
					\r\n \r\n\r\nip: %s",\
					(char *)inet_ntoa(mycar->client_addr.sin_addr),\
					mycar->sn,rec_buf[5],rec_buf[6]<<8 | rec_buf[7],\
					(char *)inet_ntoa(mycar->client_addr.sin_addr));

			cloud_post( cloud_host, &post_buf, 80 );
			cloud_post( log_host, &post_buf, 86 );
			exit( 0 );
		}
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

		//Create new process (non-block) for cloud post
		cloud_pid = fork();
		if (cloud_pid == 0) { //In child process

			sprintf(post_buf,"ip=%s&fid=41&subject=Need SN&message=Client ip: %s",\
					(char *)inet_ntoa(mycar->client_addr.sin_addr),\
					(char *)inet_ntoa(mycar->client_addr.sin_addr));

			cloud_post( cloud_host, &post_buf, 80 );
			cloud_post( log_host, &post_buf, 86 );
			exit( 0 );
		}
	}

	return 0 ;
}
