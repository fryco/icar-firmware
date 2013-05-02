//TBD: �����������¼ÿ������ʱ�䡢���͡������ֽ���;

//$URL$ 
//$Rev$, $Date$
//fprintf(stderr, "rokko: %X\tbuf: %X\t%s:%d\n",rokko,rec_buf,__FILE__,__LINE__);

#include "config.h"
#include "rokkod.h"

extern unsigned int foreground;

//Forum id:
//'36' ==> Machine
//'37' ==> Instruction 	
//'38' ==> Signal 	
//'39' ==> Sync time
//'40' ==> Login
//'41' ==> Log err
//'42' ==> Upgrade firmware / Update parameter
//'43' ==> Server status

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
{//$Rev$
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

//check send buf
void check_sndbuf( unsigned char *snd_buf )
{
	char log_str[EMAIL+1];
	
	//buf[0] = GSM_HEAD
	//buf[1] = SEQ
	//buf[2] = CMD
	//buf[3] = len_h
	//buf[4] = len_l
	
	if ( (snd_buf[0] != GSM_HEAD) || ((snd_buf[3]<<8)|(snd_buf[4])) > 1200 ) {
		snprintf(log_str,sizeof(log_str),"!!!ERR!!! %02X,%02X,%02X @ %s:%d\n",\
				snd_buf[2],snd_buf[3],snd_buf[4],__FILE__,__LINE__);
		log_save(log_str, FORCE_SAVE_FILE );
		if ( foreground ) fprintf(stderr,"%s",log_str);
		exit(1);
	}
	return ;
}
//send failure to client
unsigned char failure_cmd( struct rokko_data *rokko, unsigned char *snd_buf, \
						struct rokko_command *cmd, unsigned char err_code )
{
	unsigned short crc16;
	
	//send failure respond 
	bzero( snd_buf, BUFSIZE);
	snd_buf[0] = GSM_HEAD ;
	snd_buf[1] = cmd->seq ;
	snd_buf[2] = cmd->pcb | 0x80 ;
	snd_buf[3] =  00;//len high
	snd_buf[4] =  01;//len low
	snd_buf[5] =  err_code;	// 01:��½ʧ��, 02��������æ,
							//03: CRC����, 04: �Ѿ�������FW�汾, 05��Ӳ����֧��


	//Calc CRC16
	crc16 = crc16tablefast(snd_buf , ((snd_buf[3]<<8)|(snd_buf[4]))+5);
	
	snd_buf[6] = (crc16)>>8 ;
	snd_buf[7] = (crc16)&0xFF ;

	if ( foreground ) {
		fprintf(stderr, "CMD is %c(0x%02X), but failure, reply: ",(cmd->pcb)&0x7F,cmd->pcb);
		for ( crc16 = 0 ; crc16 < ((snd_buf[3]<<8)|(snd_buf[4]))+7 ; crc16++ ) {
			fprintf(stderr, "%02X ",snd_buf[crc16]);
		}
		fprintf(stderr, "to %s\n",rokko->pro_sn);
	}

	check_sndbuf( snd_buf );
	write(rokko->client_socket,snd_buf,((snd_buf[3]<<8)|(snd_buf[4]))+7);
	//save transmit count
	rokko->tx_cnt += ((snd_buf[3]<<8)|(snd_buf[4]))+7;
	
	return 0;
}

//0:ok, others: err
unsigned char queue_free(struct SENT_QUEUE *queue, struct rokko_command * cmd)
{
	unsigned char queue_index ;

	//find the sent record queue_sent by SEQ
	for ( queue_index = 0 ; queue_index < MAX_CMD_QUEUE ; queue_index++) {
		
		fprintf(stderr,"queue: %d seq: 0x%02X CMD %c\r\n",queue_index,\
					queue[queue_index].send_seq, queue[queue_index].send_pcb);
		
		if ( queue[queue_index].send_seq == cmd->seq \
			&& cmd->pcb==(queue[queue_index].send_pcb | 0x80)) { 
		
			fprintf(stderr,"queue: %d seq: 0x%02X CMD %c match, set confirm\r\n",queue_index,\
						cmd->seq, queue[queue_index].send_pcb);
		
			//found, set confirm
			queue[queue_index].confirm = 1 ;
			return 0 ;
		}//end of queue[queue_index].send_pcb == 0
	}
	
	fprintf(stderr, "No find CMD %c in queue, check %s:%d\r\n",(cmd->pcb)&0x7F,__FILE__, __LINE__);
	return 1 ;
}

/****************** Below for server respond client command *********/
int rec_cmd_console( struct rokko_data *rokko, struct rokko_command * cmd,\
				unsigned char *rec_buf, unsigned char *snd_buf, \
				struct rokko_data *rokko_all, unsigned int conn_amount )
{//case GSM_CMD_CONSOLE: //0x43,'C' Console command

	pid_t cloud_pid;
	unsigned char post_buf[BUFSIZE];
	unsigned short crc16, data_len;
	//DE SEQ 43 LEN DATA CHK

	//Console special mark: HW rev
	//buf[19] =  0xFA  ;//hw revision, 1 byte
	//buf[20] =  0xFA  ;//reverse

	if ( (strlen(rokko->pro_sn) == 10) && rokko->hw_rev == 0xFAFA ) {
		{ //ok

			data_len = ((rec_buf[3])<<8) | rec_buf[4];
			if ( data_len > 32 ) data_len = 32 ; //prevent error
				
			if ( foreground ) {
				fprintf(stderr, "Console CMD is: %c, SN: ",rec_buf[5]);
				for ( crc16 = 0 ; crc16 < 10 ; crc16++ ) {
					fprintf(stderr, "%02X ",rec_buf[crc16+6]);
				}
				fprintf(stderr, "From: %s, hw_rev:%X\n",rokko->pro_sn,rokko->hw_rev);
			}

			switch ( rec_buf[5] ) {
		
			case CONSOLE_CMD_LIST_ALL : //list all or special active client
				if ( console_list_all( rokko,&cmd,\
							rec_buf, snd_buf, rokko_all, conn_amount )) {//err
					break; //send err to console in below
				}
				else { //ok
					return 0 ;
				}
		
			case CONSOLE_CMD_LIST_SPE : //list special active client
				if ( console_list_spe( rokko,&cmd,\
							rec_buf, snd_buf, rokko_all, conn_amount )) {//err
					break; //send err to console in below
				}
				else { //ok
					return 0 ;
				}

			default:
				fprintf(stderr,"Unknow CMD! @%d\n",__LINE__);
				break;
			}

			bzero( snd_buf, BUFSIZE); //send failure to console
			snd_buf[0] = GSM_HEAD ;
			snd_buf[1] = cmd->seq ;
			snd_buf[2] = cmd->pcb | 0x80 ;
			snd_buf[3] =  00;//len high
			snd_buf[4] =  02;//len low
			snd_buf[5] =  ERR_RETURN_CONSOLE_CMD ;
			snd_buf[6] =  00;//ok

			data_len = ((snd_buf[3])<<8) | snd_buf[4] ;
	
			//Calc CRC16
			crc16 = 0xFFFF & (crc16tablefast(snd_buf , data_len+5));
	
			snd_buf[data_len+5] = (crc16)>>8 ;
			snd_buf[data_len+6] = (crc16)&0xFF ;
	
			if ( foreground ) {
				fprintf(stderr, "CMD: %c ok, reply: ",cmd->pcb);
				for ( crc16 = 0 ; crc16 < data_len+7 ; crc16++ ) {
					fprintf(stderr, "%02X ",snd_buf[crc16]);
				}
				fprintf(stderr, "to %s\n",rokko->pro_sn);
			}
		
			check_sndbuf( snd_buf );
			write(rokko->client_socket,snd_buf,data_len+7);
			//save transmit count
			rokko->tx_cnt += data_len+7 ;
			
			return 0 ;
/*
			//Create new process (non-block) for cloud post
			cloud_pid = fork();
			if (cloud_pid == 0) { //In child process
				fprintf(stderr, "In child:%d for cloud post\n",getpid());

				cloud_post( cloud_host, &post_buf, 80 );
				cloud_post( log_host, &post_buf, 86 );
				exit( 0 );
			}
			else {//In parent process
				//fprintf(stderr, "In parent:%d and return\n",getpid());
				return 0 ;
			}*/
		}
	}//end of strlen(rokko->pro_sn) == 10
	else { //no SN

		fprintf(stderr, "SN:%s, len:%d No SN! %s:%d\r\n",rokko->pro_sn,strlen(rokko->pro_sn),__FILE__, __LINE__);
		failure_cmd( rokko, snd_buf, cmd, ERR_RETURN_NO_LOGIN );
			
		return 1 ;
/*
		//Create new process (non-block) for cloud post
		cloud_pid = fork();
		if (cloud_pid == 0) { //In child process

			sprintf(post_buf,"ip=%s&fid=41&subject=Need SN&message=Client ip: %s",\
					(char *)inet_ntoa(rokko->client_addr.sin_addr),\
					(char *)inet_ntoa(rokko->client_addr.sin_addr));

			cloud_post( cloud_host, &post_buf, 80 );
			cloud_post( log_host, &post_buf, 86 );
			exit( 0 );
		}*/
	}
	return 0 ;
}

int rec_cmd_errlog( struct rokko_data *rokko, struct rokko_command * cmd,\
				unsigned char *rec_buf, unsigned char *snd_buf )
{//case GSM_CMD_ERROR: //0x45,'E' Error, upload error log

	//BKP_DR1, ERR index: 	15~12:MCU reset 
	//						11~8:upgrade fw failure code
	//						7~4:GPRS disconnect reason
	//						3~0:GSM module poweroff reason
	unsigned char err_idx=0;//1: 3~0, 2:7~4, 3: 11~8, 4:15~12
	pid_t cloud_pid;
	unsigned char post_buf[BUFSIZE];
	unsigned short crc16, data_len, err_code;
	//DE SEQ 45 LEN DATA CHK

	//err_code, 2 byte
	err_code = rec_buf[9] << 8 | rec_buf[10];
	if ( err_code & 0xF000 ) { //highest priority, MCU reset
		err_code = (err_code & 0xF000) >> 12 ;
		err_idx = 4 ;
	}
	else {
		if ( err_code & 0x0F00 ) { //higher priority
			err_code = (err_code & 0x0F00) >> 8 ;
			err_idx = 3 ;
		}
		else {	
			if ( err_code & 0x00F0 ) { //GPRS disconnect err
				err_code = (err_code & 0x00F0) >> 4 ;
				err_idx = 2 ;
		 	}
			else {		
				if ( err_code & 0x000F ) { //GSM module poweroff err
					err_code = (err_code & 0x000F) ;
					err_idx = 1 ;
				}
				else {//program logic error
					fprintf(stderr, "firmware logic error, chk: %s: %d\r\n",\
						__FILE__,__LINE__);	
				}//end (i & 0x000F)
			}//end (i & 0x00F0)
		}//end (i & 0x0F00)
	}//end (i & 0xF000)


	if ( strlen(rokko->pro_sn) == 10 ) {
		{ //ok

			bzero( snd_buf, BUFSIZE);
			snd_buf[0] = GSM_HEAD ;
			snd_buf[1] = cmd->seq ;
			snd_buf[2] = cmd->pcb | 0x80 ;
			snd_buf[3] =  00;//len high
			snd_buf[4] =  02;//len low
			snd_buf[5] =  00;//return status:0���ɹ�
			snd_buf[6] =  err_idx;//ok

			data_len = ((snd_buf[3])<<8) | snd_buf[4] ;
	
			//Calc CRC16
			crc16 = 0xFFFF & (crc16tablefast(snd_buf , data_len+5));
	
			snd_buf[data_len+5] = (crc16)>>8 ;
			snd_buf[data_len+6] = (crc16)&0xFF ;
	
			if ( foreground ) {
				fprintf(stderr, "CMD: %c ok, reply: ",cmd->pcb);
				for ( crc16 = 0 ; crc16 < data_len+7 ; crc16++ ) {
					fprintf(stderr, "%02X ",snd_buf[crc16]);
				}
				fprintf(stderr, "to %s\n",rokko->pro_sn);
			}
		
			check_sndbuf( snd_buf );
			write(rokko->client_socket,snd_buf,data_len+7);
			//save transmit count
			rokko->tx_cnt += data_len+7 ;
			
			return 0 ;
/*
			//Create new process (non-block) for cloud post
			cloud_pid = fork();
			if (cloud_pid == 0) { //In child process
				fprintf(stderr, "In child:%d for cloud post\n",getpid());

				cloud_post( cloud_host, &post_buf, 80 );
				cloud_post( log_host, &post_buf, 86 );
				exit( 0 );
			}
			else {//In parent process
				//fprintf(stderr, "In parent:%d and return\n",getpid());
				return 0 ;
			}*/
		}
	}//end of strlen(rokko->pro_sn) == 10
	else { //no SN

		fprintf(stderr, "SN:%s, len:%d No SN! %s:%d\r\n",rokko->pro_sn,strlen(rokko->pro_sn),__FILE__, __LINE__);
		failure_cmd( rokko, snd_buf, cmd, ERR_RETURN_NO_LOGIN );
			
		return 1 ;
/*
		//Create new process (non-block) for cloud post
		cloud_pid = fork();
		if (cloud_pid == 0) { //In child process

			sprintf(post_buf,"ip=%s&fid=41&subject=Need SN&message=Client ip: %s",\
					(char *)inet_ntoa(rokko->client_addr.sin_addr),\
					(char *)inet_ntoa(rokko->client_addr.sin_addr));

			cloud_post( cloud_host, &post_buf, 80 );
			cloud_post( log_host, &post_buf, 86 );
			exit( 0 );
		}*/
	}
	return 0 ;
}


//0: ok, 1: disconnect immediately
unsigned char rec_cmd_login( struct rokko_data *rokko, struct rokko_command * cmd,\
				unsigned char *rec_buf, unsigned char *snd_buf )
{//case GSM_CMD_LOGIN: //0x4C, 'L', Login to server

	unsigned short crc16 ;
	unsigned int ostime;
	pid_t cloud_pid;
	unsigned char post_buf[BUFSIZE];

	//HEAD SEQ CMD Length(2 bytes) OSTime SN(char 10) IP check

	//DE 6A 4C 00 1D 00 04 19 DA //CMD + OSTIME
	//44 45 4D 4F 44 41 33 30 42 32 //SN
	//00 00 00 0D //HW, FW revision
	//31 30 2E 31 31 31 2E 32 36 2E 36 75 5C //IP + CRC16

	if ( strlen(rokko->pro_sn) == 10 ) {
	
		if ( strncmp(rokko->pro_sn,&rec_buf[9], 10) ) {
			//no same
			unsigned char fake_sn[12];
			
			bzero( fake_sn, sizeof(fake_sn));
			strncpy(fake_sn, &rec_buf[9], 10);

			bzero( snd_buf, BUFSIZE);
			snprintf(snd_buf,BUFSIZE,"Fake SN: %s => %s, login failure. IP:%s\n",rokko->pro_sn,fake_sn,\
						(char *)inet_ntoa(rokko->client_addr.sin_addr));
			log_err(snd_buf);
			
			return 1 ;//close immediately
		}
		else {
			ostime = rec_buf[5] << 24 | rec_buf[6] << 16 | rec_buf[7] << 8 | rec_buf[8];

			unsigned char up_buf[EMAIL];
	
			ticks2time(ostime, up_buf, sizeof(up_buf));
			//fprintf(stderr, "%s Up: %s",rokko->pro_sn,up_buf); 

			bzero( snd_buf, BUFSIZE);
			snd_buf[0] = GSM_HEAD ;
			snd_buf[1] = cmd->seq ;
			snd_buf[2] = cmd->pcb | 0x80 ;
			snd_buf[3] =  00;//len high
			snd_buf[4] =  17;//len low
			snd_buf[5] =  00;//return status:0���ɹ�
			snd_buf[6] =  (time(NULL) >> 24)&0xFF;//time high
			snd_buf[7] =  (time(NULL) >> 16)&0xFF;//time high
			snd_buf[8] =  (time(NULL) >> 8)&0xFF;//time low
			snd_buf[9] =  (time(NULL) >> 00)&0xFF;//time low
		
			//MCU ID��12 B, from DB, for verify server
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
			crc16 = crc16tablefast(snd_buf , ((snd_buf[3]<<8)|(snd_buf[4]))+5);
			
			snd_buf[22] = (crc16)>>8 ;
			snd_buf[23] = (crc16)&0xFF ;

/*			if ( foreground ) {
				fprintf(stderr, "CMD is Login, reply: ");
				for ( crc16 = 0 ; crc16 < ((snd_buf[3]<<8)|(snd_buf[4]))+7 ; crc16++ ) {
					fprintf(stderr, "%02X ",snd_buf[crc16]);
				}
				fprintf(stderr, "to %s\n",rokko->pro_sn);
			}
*/		
			check_sndbuf( snd_buf );
			write(rokko->client_socket,snd_buf,((snd_buf[3]<<8)|(snd_buf[4]))+7);
			//save transmit count
			rokko->tx_cnt += ((snd_buf[3]<<8)|(snd_buf[4]))+7 ;
			
			return 0 ;
		}
	}
	else {	//check product SN from DB
		strncpy(rokko->pro_sn, &rec_buf[9], 10);
		rokko->pro_sn[10] = 0x0;

		//HW/FW revision
		rokko->hw_rev = rec_buf[19]<<8|rec_buf[20];//HW rev
		rokko->fw_rev = rec_buf[21]<<8|rec_buf[22];//FW rev

		ostime = rec_buf[5] << 24 | rec_buf[6] << 16 | rec_buf[7] << 8 | rec_buf[8];

		unsigned char up_buf[EMAIL];

		ticks2time(ostime, up_buf, sizeof(up_buf));
		//fprintf(stderr, "%s Up: %s",rokko->pro_sn,up_buf); 
/*
		if ( foreground ) {
			fprintf(stderr, "SN: %s\t",rokko->pro_sn);
			fprintf(stderr, "rokko->pro_sn: %s\tHW:%d\tFW:%d\r\n",\
							rokko->pro_sn,rokko->hw_rev,rokko->fw_rev);
			fprintf(stderr, "%s",up_buf);
		}
*/				
		if ( 1 ) {//if check DB ok, TBD
			//send successful respond 
			bzero( snd_buf, BUFSIZE);
			snd_buf[0] = GSM_HEAD ;
			snd_buf[1] = cmd->seq ;
			snd_buf[2] = cmd->pcb | 0x80 ;
			snd_buf[3] =  00;//len high
			snd_buf[4] =  17;//len low
			snd_buf[5] =  00;//return status:0���ɹ�
			snd_buf[6] =  (time(NULL) >> 24)&0xFF;//time high
			snd_buf[7] =  (time(NULL) >> 16)&0xFF;//time high
			snd_buf[8] =  (time(NULL) >> 8)&0xFF;//time low
			snd_buf[9] =  (time(NULL) >> 00)&0xFF;//time low
		
			//MCU ID��12 B, from DB, for verify server
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
			crc16 = crc16tablefast(snd_buf , ((snd_buf[3]<<8)|(snd_buf[4]))+5);
			
			snd_buf[22] = (crc16)>>8 ;
			snd_buf[23] = (crc16)&0xFF ;

			if ( foreground ) {
				fprintf(stderr, "CMD is Login, reply: ");
				for ( crc16 = 0 ; crc16 < ((snd_buf[3]<<8)|(snd_buf[4]))+7 ; crc16++ ) {
					fprintf(stderr, "%02X ",snd_buf[crc16]);
				}
				fprintf(stderr, "to %s\n",rokko->pro_sn);
			}

			check_sndbuf( snd_buf );
			write(rokko->client_socket,snd_buf,((snd_buf[3]<<8)|(snd_buf[4]))+7);
			//save transmit count
			rokko->tx_cnt += ((snd_buf[3]<<8)|(snd_buf[4]))+7 ;
	
			//save to log file
			bzero( snd_buf, BUFSIZE);
			snprintf(snd_buf,BUFSIZE,"SN: %s login. IP:%s\n",rokko->pro_sn,\
						(char *)inet_ntoa(rokko->client_addr.sin_addr));
			log_save(snd_buf,0);

			//Create new process (non-block) for cloud post
			cloud_pid = fork();
			if (cloud_pid == 0) { //In child process


				
/*				
				bzero( post_buf, BUFSIZE);		
				snprintf(post_buf,BUFSIZE,"ip=%s&fid=40&subject=%s, HW:%d FW:%d from %s&message=uptime(H:M:S): %d:%02d:%02d\r\n\
				\r\nLAN ip:%s\r\n\r\nip: %s",\
				(char *)inet_ntoa(rokko->client_addr.sin_addr),mycar->pro_sn,rokko->hw_rev,rokko->fw_rev,\
				ostime/360000,((ostime/100)%3600)/60,((ostime/100)%3600)%60,\
				gsm_ip,(char *)inet_ntoa(mycar->client_addr.sin_addr));

				//cloud_post( cloud_host, &post_buf, 80 );
				//cloud_post( log_host, &post_buf, 86 );
*/				exit( 0 );
			}

			return 0 ;
		} //end check DB ok
		else {//failure
			if ( foreground ) {
				fprintf(stderr, "SN:%s, len:%d No SN! %s:%d\r\n",rokko->pro_sn,strlen(rokko->pro_sn),__FILE__, __LINE__);
			}

			bzero( snd_buf, BUFSIZE);
			snprintf(snd_buf,BUFSIZE,"SN: %s err, no this SN in DB, login failure. IP:%s\n",rokko->pro_sn,\
						(char *)inet_ntoa(rokko->client_addr.sin_addr));
			log_err(snd_buf);

			failure_cmd( rokko, snd_buf, cmd, ERR_RETURN_NO_LOGIN );
			
			return 1 ;
		}
	}
}

int rec_cmd_record( struct rokko_data *rokko, struct rokko_command * cmd,\
				unsigned char *rec_buf, unsigned char *snd_buf )
{//case GSM_CMD_RECORD: //0x52, 'R' Record GSM signal,adc ...

	pid_t cloud_pid;
	unsigned char post_buf[BUFSIZE];
	unsigned char rec_idx;
	unsigned short rec_dat, crc16, data_len;
	unsigned int rec_time;

	//DE 05 52 00 08 51 1F 36 63 00 0A D0 1A EE 2B
	//Head SEQ PCB Len(2B)
	//51 1F 36 63 ==> UTC
	//00 0A D0 ==> record val
	//1A       ==> record item index
	
	if ( foreground ) {
		fprintf(stderr, "CMD is Record vehicle parameters ...\n");
	}
	
	if ( strlen(rokko->pro_sn) == 10 ) {

			//show the record detail
			data_len = ((rec_buf[3])<<8) | rec_buf[4];

			if ( foreground ) {
				fprintf(stderr, "Record len:%d\r\n",data_len);

				for ( crc16= 0 ; crc16*8 < (data_len-4) ; crc16++ ) {
					rec_idx = rec_buf[12+crc16*8];
					rec_time = (rec_buf[crc16*8+5]<<24)|(rec_buf[crc16*8+6]<<16)|(rec_buf[crc16*8+7]<<8)|rec_buf[crc16*8+8];					
					rec_dat = (rec_buf[crc16*8+9]<<16)|(rec_buf[crc16*8+10]<<8)|rec_buf[crc16*8+11];
					fprintf(stderr, "R[%02d] Val: %X, %s\r\n",rec_idx,rec_dat,ctime((const void *)&rec_time));
				}
			}

			bzero( snd_buf, BUFSIZE);

			//send respond
			crc16++;
			snd_buf[0] = GSM_HEAD ;
			snd_buf[1] = cmd->seq ;
			snd_buf[2] = cmd->pcb | 0x80 ;
			snd_buf[3] = 0 ;//len high
			snd_buf[4] = 2 ;//len low
			snd_buf[5] = 0 ;//return status:0���ɹ�
			snd_buf[6] = 0 ;//reserved
			
			data_len = ((snd_buf[3])<<8) | snd_buf[4] ;
	
			//Calc CRC16
			crc16 = 0xFFFF & (crc16tablefast(snd_buf , data_len+5));
	
			snd_buf[data_len+5] = (crc16)>>8 ;
			snd_buf[data_len+6] = (crc16)&0xFF ;
	
			if ( foreground ) {
				fprintf(stderr, "CMD: %c ok, reply: ",cmd->pcb);
				for ( crc16 = 0 ; crc16 < data_len+7 ; crc16++ ) {
					fprintf(stderr, "%02X ",snd_buf[crc16]);
				}
				fprintf(stderr, "to %s\n",rokko->pro_sn);
			}
		
			check_sndbuf( snd_buf );
			write(rokko->client_socket,snd_buf,data_len+7);
			//save transmit count
			rokko->tx_cnt += data_len+7 ;
			
			return 0 ;

	}//end of strlen(rokko->pro_sn) == 10
	else { //need upload sn first
		fprintf(stderr, "SN:%s, len:%d No SN! %s:%d\r\n",rokko->pro_sn,strlen(rokko->pro_sn),__FILE__, __LINE__);
		failure_cmd( rokko, snd_buf, cmd, ERR_RETURN_NO_LOGIN );
			
		return 1 ;
	}
}

//0: ok, others: error
int rec_cmd_upgrade( struct rokko_data *rokko, struct rokko_command * cmd,\
				unsigned char *rec_buf, unsigned char *snd_buf )
{//case GSM_CMD_UPGRADE://0x55, 'U', Upgrade firmware

	int fd;
	unsigned int i , fpos, fw_size, fw_rev;
	unsigned char *filename="./fw/stm32_v00/20130103.bin";
	unsigned char rev_info[MAX_FW_SIZE], *rev_pos;
	pid_t cloud_pid;
	unsigned char post_buf[BUFSIZE];
	unsigned char blk_cnt;
	unsigned short crc16, data_len;

	fprintf(stderr, "CMD is Upgrade fw...\n");

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

	if((fw_size%1024) > 0 ){
		blk_cnt = (fw_size >> 10) + 1;
	}
	else{
		blk_cnt = (fw_size >> 10);
	}
	//printf("Block count: %d\r\n",blk_cnt);


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
	if ( foreground ) {
	    fprintf(stderr,"File: %s size is:%d Rev:%d\n", filename,fw_size,fw_rev);
	}

	if ( strlen(rokko->pro_sn) == 10 ) {
		if ( rokko->fw_rev <  fw_rev ){//need upgrade
			fprintf(stderr, "SN: %s\t",rokko->pro_sn);
			fprintf(stderr, "rokko->pro_sn: %s\r\n",rokko->pro_sn);
	
			//if buf[5] > 0xF0, error report from STM32 for upgrade flash
			//TBD
	
			//Check input detail
			//C9 F9 55 00 04 00 00 00 5E
			//buf[5] = 0x00 ;//00: mean buf[6] is hw rev, others: block seq
	
			bzero( snd_buf, BUFSIZE);
			if ( rec_buf[5] == 0 ) { //HW,FW info
				if ( foreground ) {
					fprintf(stderr, "Current HW rev: %d, FW rev: %d\r\n",\
						rec_buf[6],rec_buf[7]<<8 | rec_buf[8]);
				}
	
				//send respond : HW,FW rev&size
	
				//C9 57 D5 00 05 00 data
				//05: data len, 00: data is latest firmware revision(u16) + size(u16)
				snd_buf[0] = GSM_HEAD ;
				snd_buf[1] = cmd->seq ;
				snd_buf[2] = cmd->pcb | 0x80 ;
	
				snd_buf[3] =  00;//len high
				snd_buf[4] =  10;//len low
				snd_buf[5] =  00;//status
				
				//00 ��ʾ�������������FW�汾��(2B)��FW����(4B)��FW CRC16���
				snd_buf[6] =  00 ;
	
				snd_buf[7] =  (fw_rev >> 8)&0xFF;//Rev high
				snd_buf[8] =  (fw_rev)&0xFF;//Rev low

				snd_buf[9] =  (fw_size >> 24)&0xFF;//Size high
				snd_buf[10]=  (fw_size >> 16)&0xFF;//Size high
				snd_buf[11]=  (fw_size >> 8 )&0xFF;//Size low
				snd_buf[12]=  (fw_size)&0xFF;//Size low

				//generate FW CRC:
				lseek( fd, 0, SEEK_SET );
				bzero( rev_info, BUFSIZE);
				fpos = read(fd, rev_info, MAX_FW_SIZE);
	
				//Calc CRC16
				crc16 = 0xFFFF & (crc16tablefast(rev_info , fw_size));
				
				snd_buf[13] = (crc16)>>8 ;
				snd_buf[14] = (crc16)&0xFF ;
			}//
			else {//others : block seq
				fprintf(stderr, "Ask Block %d, FW rev: %d  \t",\
						rec_buf[5],rec_buf[6]<<8 | rec_buf[7]);
	
				if ( rec_buf[5] > blk_cnt ) {//error
	
					snd_buf[0] = GSM_HEAD ;
					snd_buf[1] = cmd->seq ;
					snd_buf[2] = cmd->pcb | 0x80 ;
		
					snd_buf[3] =  00;//len high
					snd_buf[4] =  1;//len low
					snd_buf[5] =  4;//status: no this block
					
				}
				else {
					//send respond : Block data
					//C9 57 D5 00 xx yy FW_Rev(2 Bytes) + data + CRC
					//xx: data len, yy: block seq, data: block data
					snd_buf[0] = GSM_HEAD ;
					snd_buf[1] = cmd->seq ;
					snd_buf[2] = cmd->pcb | 0x80 ;
		
					snd_buf[5] =  00;//status
					snd_buf[6] =  rec_buf[5];//block seq
		
					snd_buf[7] =  (fw_rev >> 8)&0xFF;//Rev high
					snd_buf[8] =  (fw_rev)&0xFF;//Rev low
		
					//Block data, Max data len is 1K!!!
					//read fw according to block seq
					lseek( fd, (rec_buf[5]-1)*1024, SEEK_SET );
					fpos = read(fd, &snd_buf[9], 1*1024);
					data_len = fpos + 4 ;//include status, blk seq, rev info
		
					fprintf(stderr, "Read: %d, BLK: %d, DAT: ",fpos,rec_buf[5]);
		
					for ( crc16 = 0 ; crc16 < 8; crc16++) {
						fprintf(stderr, "%02X ",snd_buf[9+crc16]);
					}
					fprintf(stderr, "...\r\n");
					
					//Calc FW CRC16
					crc16 = 0xFFFF & (crc16tablefast(&snd_buf[9] , fpos));
		
					snd_buf[data_len+5] = (crc16 >> 8) & 0xFF ;
					snd_buf[data_len+6] = (crc16) & 0xFF ;
		
					fprintf(stderr, "BLK %d CRC16: %04X\r\n",rec_buf[5],crc16&0xFFFF);
					//update len: +2 for CRC16
					snd_buf[3] = ((data_len+2) >> 8) & 0xFF;
					snd_buf[4] = ((data_len+2) ) & 0xFF;
		/*
					//Create new process (non-block) for cloud post
					cloud_pid = fork();
					if (cloud_pid == 0) { //In child process
			
						sprintf(post_buf,"ip=%s&fid=42&subject=%s => Upgrade, Block %d&message=Sending block: %d,  data length: %d\r\n\
								\r\nNew firmware rev: %d,  size: %d \r\n\r\nip: %s",\
								(char *)inet_ntoa(rokko->client_addr.sin_addr),\
								rokko->pro_sn,rec_buf[5],rec_buf[5],data_len-3,fw_rev,fw_size,\
								(char *)inet_ntoa(rokko->client_addr.sin_addr));
			
						cloud_post( cloud_host, &post_buf, 80 );
						cloud_post( log_host, &post_buf, 86 );
						exit( 0 );
					}*/
				}
			}
			//Calc chk
			data_len = ((snd_buf[3])<<8) | snd_buf[4] ;
	
			//Calc CRC16
			crc16 = 0xFFFF & (crc16tablefast(snd_buf , data_len+5));
	
			snd_buf[data_len+5] = (crc16)>>8 ;
			snd_buf[data_len+6] = (crc16)&0xFF ;
	
			fprintf(stderr, "data_len: %d, CRC16: %04X\n",data_len,crc16);
			
			if ( foreground ) {
				fprintf(stderr, "CMD %c ok, will return: ",cmd->pcb);
				if ( data_len < 128 ) {
					for ( crc16 = 0 ; crc16 < data_len+7 ; crc16++ ) {
						fprintf(stderr, "%02X ",snd_buf[crc16]);
					}
				}
				else {
					for ( crc16 = 0 ; crc16 < 16 ; crc16++ ) {
						fprintf(stderr, "%02X ",snd_buf[crc16]);
					}
				}
				fprintf(stderr, "to %s\n",rokko->pro_sn);
			} 
	
			check_sndbuf( snd_buf );
			write(rokko->client_socket,snd_buf,data_len+7);
			//save transmit count
			rokko->tx_cnt += data_len+7 ;
		}
		else { //( no need upgrade )
			fprintf(stderr, "The client FW:%d is latest. %d\n",rokko->fw_rev,__LINE__);
			failure_cmd( rokko, snd_buf, cmd, ERR_RETURN_FW_LATEST );
		}
	}
	else { //no SN
		fprintf(stderr, "SN:%s, len:%d No SN! %s:%d\r\n",rokko->pro_sn,strlen(rokko->pro_sn),__FILE__, __LINE__);
		failure_cmd( rokko, snd_buf, cmd, ERR_RETURN_NO_LOGIN );
	}

	close(fd);
	return 0 ;
}

//0: ok, others: error
int rec_cmd_warn( struct rokko_data *rokko, struct rokko_command * cmd,\
				unsigned char *rec_buf, unsigned char *snd_buf )
{//case GSM_CMD_WARN://0x57, 'W', warn msg report

	unsigned int i, var_u32;
	pid_t cloud_pid;
	unsigned char post_buf[BUFSIZE];
	unsigned short crc16 , data_len;

	//DE 03 57 00 05 01 01 0A 20 01 54 96
	fprintf(stderr, "CMD is warn msg report...\n");

	if ( strlen(rokko->pro_sn) == 10 ) {
/*
		//record this command
		if ( record_command(rokko,rec_buf,"NO_ERR",8)) {
			if ( debug_flag ) {
				fprintf(stderr, "Record command err= %d: %s",\
					rokko->err_code,rokko->err_msg);
			}
		}
*/

			bzero( snd_buf, BUFSIZE);
			snd_buf[0] = GSM_HEAD ;
			snd_buf[1] = cmd->seq ;
			snd_buf[2] = cmd->pcb | 0x80 ;
			snd_buf[3] =  00;//len high
			snd_buf[4] =  02;//len low
			snd_buf[5] =  00;//return status:0���ɹ�
			snd_buf[6] =  rec_buf[9];//idx

			data_len = ((snd_buf[3])<<8) | snd_buf[4] ;
	
			//Calc CRC16
			crc16 = 0xFFFF & (crc16tablefast(snd_buf , data_len+5));
	
			snd_buf[data_len+5] = (crc16)>>8 ;
			snd_buf[data_len+6] = (crc16)&0xFF ;
	
			if ( foreground ) {
				fprintf(stderr, "CMD: %c ok, reply: ",cmd->pcb);
				for ( crc16 = 0 ; crc16 < data_len+7 ; crc16++ ) {
					fprintf(stderr, "%02X ",snd_buf[crc16]);
				}
				fprintf(stderr, "to %s\n",rokko->pro_sn);
			}
		
			check_sndbuf( snd_buf );
			write(rokko->client_socket,snd_buf,data_len+7);
			//save transmit count
			rokko->tx_cnt += data_len+7 ;

			return 0 ;


/*
		//Create new process (non-block) for cloud post
		cloud_pid = fork();
		if (cloud_pid == 0) { //In child process

			sprintf(post_buf,"ip=%s&fid=41&subject=%s => Warn MSG @ %02d&message=CMD: %s\r\n\r\nip: %s",\
					(char *)inet_ntoa(rokko->client_addr.sin_addr),\
					rokko->pro_sn,rec_buf[9],snd_buf,\
					(char *)inet_ntoa(rokko->client_addr.sin_addr));

			cloud_post( cloud_host, &post_buf, 80 );
			cloud_post( log_host, &post_buf, 86 );
			exit( 0 );
		}
		return 0; */
	}
	else { //need upload sn first
		fprintf(stderr, "SN:%s, len:%d No SN! %s:%d\r\n",rokko->pro_sn,strlen(rokko->pro_sn),__FILE__, __LINE__);
		failure_cmd( rokko, snd_buf, cmd, ERR_RETURN_NO_LOGIN );
			
		return 1 ;
	}
}

/****************** Below for server send command to client *********/
//0:ok, others: err
unsigned char snd_cmd_record( struct rokko_data *rokko, unsigned char *sequence, unsigned char *snd_buf )
{//send command record to client

	unsigned short data_len, crc16;

	bzero( snd_buf, BUFSIZE);

			snd_buf[0] = GSM_HEAD ;
			snd_buf[1] = *sequence ;

			if ( snd_buf[1] == 0xFF ) { //Server => client, seq: 0x80~FF
				*sequence = 0x80 ;
			}
			else {
				*sequence = snd_buf[1]+1;//increase seq
			}

			snd_buf[2] = GSM_CMD_RECORD ;
			snd_buf[3] =  00;//len high
			snd_buf[4] =  03;//len low

			snd_buf[5] =  10 ; //idx 1
			snd_buf[6] =  50 ; //idx 2
			snd_buf[7] =  90 ; //idx 3

			data_len = ((snd_buf[3])<<8) | snd_buf[4] ;
			
			//Calc CRC16
			crc16 = 0xFFFF & (crc16tablefast(snd_buf , data_len+5));
	
			snd_buf[data_len+5] = (crc16)>>8 ;
			snd_buf[data_len+6] = (crc16)&0xFF ;
	
			if ( foreground ) {
				fprintf(stderr, "Send CMD %c : ",snd_buf[2]);
				for ( crc16 = 0 ; crc16 < ((snd_buf[3]<<8)|(snd_buf[4]))+7 ; crc16++ ) {
					fprintf(stderr, "%02X ",snd_buf[crc16]);
				}
				fprintf(stderr, "to %s\n",rokko->pro_sn);
			}
		
			check_sndbuf( snd_buf );
			write(rokko->client_socket,snd_buf,data_len+7);
			//save transmit count
			rokko->tx_cnt += data_len+7 ;

			return 0 ;
}

//0:ok, others: err
unsigned char snd_cmd_upgrade( struct rokko_data *rokko, unsigned char *sequence, unsigned char *snd_buf )
{//send command upgrade to client

	int fd;
	unsigned int  i, fw_size, fw_rev, fpos;
	unsigned char rev_info[MAX_FW_SIZE], *rev_pos;
	unsigned char *filename="./fw/stm32_v00/20130103.bin";
	unsigned short crc16, data_len;
	
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
	if ( fw_size > MAX_FW_SIZE-1 ) {//must < 100KB
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

		if ( rokko->fw_rev <  fw_rev ){//need upgrade
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
			snd_buf[4] =  9;//len low

			//00 ��ʾ�������������Ӳ���汾��(2B)��FW�汾��(2B)��FW����(4B)
			snd_buf[5] =  00 ;

			snd_buf[6] =  00 ; //HW rev
			snd_buf[7] =  00 ;
			
			snd_buf[8] =  (fw_rev >> 8)&0xFF;//Rev high
			snd_buf[9] =  (fw_rev)&0xFF;//Rev low

			snd_buf[10]=  (fw_size >> 24)&0xFF;//Size high
			snd_buf[11]=  (fw_size >> 16)&0xFF;//Size high
			snd_buf[12]=  (fw_size >> 8 )&0xFF;//Size low
			snd_buf[13]=  (fw_size)&0xFF;//Size low

			data_len = ((snd_buf[3])<<8) | snd_buf[4] ;
	
			//Calc CRC16
			crc16 = 0xFFFF & (crc16tablefast(snd_buf , data_len+5));
	
			snd_buf[data_len+5] = (crc16)>>8 ;
			snd_buf[data_len+6] = (crc16)&0xFF ;
		
			if ( foreground ) {
				fprintf(stderr, "Send CMD %c : ",snd_buf[2]);
				for ( crc16 = 0 ; crc16 < ((snd_buf[3]<<8)|(snd_buf[4]))+7 ; crc16++ ) {
					fprintf(stderr, "%02X ",snd_buf[crc16]);
				}
				fprintf(stderr, "to %s\n",rokko->pro_sn);
			}

			check_sndbuf( snd_buf );
			write(rokko->client_socket,snd_buf,data_len+7);
			//save transmit count
			rokko->tx_cnt += data_len+7 ;

		}//end of ( rokko->fw_rev <  fw_rev )
		else { //( no need upgrade )
			fprintf(stderr, "The client FW:%d is latest. %d\n",rokko->fw_rev,__LINE__);
		}
}

//0:ok, others: err
unsigned char snd_cmd( struct rokko_data *rokko, unsigned char *seq, unsigned char *send_buf,\
						struct SENT_QUEUE *queue, unsigned char queue_cnt )
{//send command to client

	unsigned char cmd = 0 , index;			

	//get CMD from DB first, TBD

	//cmd = GSM_CMD_UPGRADE ;
	cmd = GSM_CMD_RECORD ;

	//check send timer first
	for ( index = 0 ; index < queue_cnt ; index++) {
		if ( queue[index].send_pcb == GSM_CMD_UPGRADE ) {//��������������30���ӳ�ʱ
			if ( (time(NULL)) - queue[index].send_timer > 360*CMD_TIMEOUT ) {

				fprintf(stderr, "Q: %d, CMD %c(0x%02X) timeout, clean. %s:%d\r\n",\
						index,(queue[index].send_pcb)&0x7F,(queue[index].send_pcb)&0x7F,\
						__FILE__, __LINE__);
				//����˼�¼
				queue[index].send_timer= 0 ;
				queue[index].confirm  = 0 ;
				queue[index].send_seq = 0;
				queue[index].send_pcb = 0 ;
			}
		}//end of ( cmd == GSM_CMD_UPGRADE )
		else { //��������
			if ( queue[index].send_pcb && \
				((time(NULL)) - queue[index].send_timer > 5*CMD_TIMEOUT) ) {

				fprintf(stderr, "Q: %d, CMD %c(0x%02X) timeout, clean. %s:%d\r\n",\
						index,(queue[index].send_pcb)&0x7F,(queue[index].send_pcb)&0x7F,\
						__FILE__, __LINE__);
				//����˼�¼
				queue[index].send_timer= 0 ;
				queue[index].confirm  = 0 ;
				queue[index].send_seq = 0;
				queue[index].send_pcb = 0 ;
			}
		}
	}

	//Check this cmd in SENT_QUEUE or not //check had sent or not?
	for ( index = 0 ; index < queue_cnt ; index++) {
		if ( queue[index].send_pcb == cmd ) { //had sent
			if ( queue[index].confirm ) {//�Ѿ�ȷ�Ϲ�
				
				//��ȷ�ϣ������ٷ���ֱ�ӷ��ؼ���
				return 0 ;
			}
			else { //��û�յ�ȷ��
				if ( (time(NULL)) - queue[index].send_timer < CMD_TIMEOUT ) {
					//��û��ʱ
					return 0 ;
				}
			}
		}
	}
	
	//find a un-use SENT_QUEUE to record the SEQ/CMD
	for ( index = 0 ; index < queue_cnt ; index++) {

		if ( queue[index].send_pcb == 0 ) { //no use

			//save pcb to queue
			queue[index].send_timer= (time(NULL)) ;
			queue[index].send_seq = *seq;
			queue[index].send_pcb = cmd ;
			queue[index].confirm  = 0 ;

			//handle the cmd
			switch ( cmd ) {
			
				case GSM_CMD_RECORD: //0x52, 'R', Record require
					snd_cmd_record( rokko, seq, send_buf ) ;
					break;
		
				case GSM_CMD_UPGRADE: //0x55, 'U', Upgrade firmware
					snd_cmd_upgrade( rokko, seq, send_buf ) ;
					break;

				default:
					fprintf(stderr, "Unknow command: %c(0x%X)\r\n",cmd,cmd);
					break;
			}//end of handle the cmd

			fprintf(stderr, "Q: %d is %c\r\n",index,queue[index].send_pcb);
			return 0 ;

		}//end of queue[index].send_pcb == 0
		else { //show what's the PCB
			fprintf(stderr, "Q: %d is %c\r\n",index,queue[index].send_pcb);
		}
	}

	fprintf(stderr, "No free queue: %d check %s:%d\r\n",index,__FILE__, __LINE__);
	return 1;
}
