//$URL$ 
//$Rev$, $Date$

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

//send failure to client
unsigned char failure_cmd( struct rokko_data *rokko, unsigned char *snd_buf, struct rokko_command *cmd )
{
	unsigned char chk_count;
	
	//send failure respond 
	bzero( snd_buf, BUFSIZE);
	snd_buf[0] = GSM_HEAD ;
	snd_buf[1] = cmd->seq ;
	snd_buf[2] = cmd->pcb | 0x80 ;
	snd_buf[3] =  00;//len high
	snd_buf[4] =  01;//len low
	snd_buf[5] =  01;// 登陆失败

	//Calc CRC16
	cmd->crc16 = crc16tablefast(snd_buf , ((snd_buf[3]<<8)|(snd_buf[4]))+5);
	
	snd_buf[6] = (cmd->crc16)>>8 ;
	snd_buf[7] = (cmd->crc16)&0xFF ;

	if ( foreground ) {
		fprintf(stderr, "CMD is %c(0x%02X), but failure, reply: ",(cmd->pcb)&0x7F,cmd->pcb);
		for ( chk_count = 0 ; chk_count < ((snd_buf[3]<<8)|(snd_buf[4]))+7 ; chk_count++ ) {
			fprintf(stderr, "%02X ",snd_buf[chk_count]);
		}
		fprintf(stderr, "to %s\n",cmd->pro_sn);
	}

	write(rokko->client_socket,snd_buf,((snd_buf[3]<<8)|(snd_buf[4]))+7);
	
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
	//此处逻辑有 bug, 客户端会一直收不到响应，需改进
	
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
			snd_buf[4] =  17;//len low
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
			cmd->crc16 = crc16tablefast(snd_buf , ((snd_buf[3]<<8)|(snd_buf[4]))+5);
			
			snd_buf[22] = (cmd->crc16)>>8 ;
			snd_buf[23] = (cmd->crc16)&0xFF ;
		
			if ( foreground ) {
				fprintf(stderr, "CMD is Login, reply: ");
				for ( chk_count = 0 ; chk_count < ((snd_buf[3]<<8)|(snd_buf[4]))+7 ; chk_count++ ) {
					fprintf(stderr, "%02X ",snd_buf[chk_count]);
				}
				fprintf(stderr, "to %s\n",cmd->pro_sn);
			}
		
			write(rokko->client_socket,snd_buf,((snd_buf[3]<<8)|(snd_buf[4]))+7);
			
			return 0 ;
		} //end check DB ok
		else {//failure
			fprintf(stderr, "No SN! %s:%d\r\n",__FILE__, __LINE__);
			failure_cmd( rokko, snd_buf, cmd );
			
			return 1 ;
		}
	}
}

//0: ok, others: error
int rec_cmd_upgrade( struct rokko_data *rokko, struct rokko_command * cmd,\
				unsigned char *rec_buf, unsigned char *snd_buf )
{//case GSM_CMD_UPGRADE://0x55, 'U', Upgrade firmware

	int fd;
	unsigned int i, chk_count , data_len, fpos, fw_size, fw_rev;
	unsigned char *filename="./fw/stm32_v00/20130103.bin";
	unsigned char rev_info[MAX_FW_SIZE], *rev_pos;
	pid_t cloud_pid;
	unsigned char post_buf[BUFSIZE];
	unsigned char blk_cnt;

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

	if ( strlen(rokko->sn) == 10 ) {

		fprintf(stderr, "SN: %s\t",rokko->sn);
		fprintf(stderr, "cmd->pro_sn: %s\r\n",cmd->pro_sn);

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
			snd_buf[4] =  8;//len low
			snd_buf[5] =  00;//status
			
			//00 表示后面跟的数据是FW版本号(2B)、FW长度(2B)、FW CRC16结果
			snd_buf[6] =  00 ;

			snd_buf[7] =  (fw_rev >> 8)&0xFF;//Rev high
			snd_buf[8] =  (fw_rev)&0xFF;//Rev low

			snd_buf[9]=  (fw_size >> 8)&0xFF;//Size high
			snd_buf[10]=  (fw_size)&0xFF;//Size low

			//generate FW CRC:
			lseek( fd, 0, SEEK_SET );
			bzero( rev_info, BUFSIZE);
			fpos = read(fd, rev_info, MAX_FW_SIZE);

			//Calc CRC16
			chk_count = 0xFFFF & (crc16tablefast(rev_info , fw_size));
			
			snd_buf[11] = (chk_count)>>8 ;
			snd_buf[12] = (chk_count)&0xFF ;
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
	
				for ( chk_count = 0 ; chk_count < 8; chk_count++) {
					fprintf(stderr, "%02X ",snd_buf[9+chk_count]);
				}
				fprintf(stderr, "...\r\n");
				
				//Calc FW CRC16
				chk_count = 0xFFFF & (crc16tablefast(&snd_buf[9] , fpos));
	
				snd_buf[data_len+5] = (chk_count >> 8) & 0xFF ;
				snd_buf[data_len+6] = (chk_count) & 0xFF ;
	
				fprintf(stderr, "BLK %d CRC16: %04X\r\n",rec_buf[5],chk_count&0xFFFF);
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
							rokko->sn,rec_buf[5],rec_buf[5],data_len-3,fw_rev,fw_size,\
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
		chk_count = 0xFFFF & (crc16tablefast(snd_buf , data_len+5));

		snd_buf[data_len+5] = (chk_count)>>8 ;
		snd_buf[data_len+6] = (chk_count)&0xFF ;

		fprintf(stderr, "data_len: %d, CRC16: %04X\n",data_len,chk_count);
		
		if ( foreground ) {
			fprintf(stderr, "CMD %c ok, will return: ",cmd->pcb);
			if ( data_len < 128 ) {
				for ( chk_count = 0 ; chk_count < data_len+7 ; chk_count++ ) {
					fprintf(stderr, "%02X ",snd_buf[chk_count]);
				}
			}
			else {
				for ( chk_count = 0 ; chk_count < 16 ; chk_count++ ) {
					fprintf(stderr, "%02X ",snd_buf[chk_count]);
				}
			}
			fprintf(stderr, "to %s\n",cmd->pro_sn);
		} 

		write(rokko->client_socket,snd_buf,data_len+7);
	}
	else { //no SN
		fprintf(stderr, "SN:%s, len:%d No SN! %s:%d\r\n",rokko->sn,strlen(rokko->sn),__FILE__, __LINE__);		
		failure_cmd( rokko, snd_buf, cmd );
	}

	close(fd);
	return 0 ;
}

/****************** Below for server send command to client *********/
//0:ok, others: err
unsigned char snd_cmd_upgrade( struct rokko_data *rokko, unsigned char *sequence, unsigned char *snd_buf )
{//send command upgrade to client

	int fd;
	unsigned int  i, fw_size, fw_rev, fpos;
	unsigned char rev_info[MAX_FW_SIZE], *rev_pos;
	unsigned char *filename="./fw/stm32_v00/20130103.bin";
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
unsigned char snd_cmd( struct rokko_data *rokko, unsigned char *seq, unsigned char *send_buf,\
						struct SENT_QUEUE *queue, unsigned char queue_cnt )
{//send command to client

	unsigned char cmd = 0 , index;			

	//get CMD from DB first, TBD

	cmd = GSM_CMD_UPGRADE ;

	//Check this cmd in SENT_QUEUE or not
	for ( index = 0 ; index < queue_cnt ; index++) {

		if ( queue[index].send_pcb == cmd ) { //had sent

			if ( queue[index].confirm ) {//已经确认过
				
				if ( cmd == GSM_CMD_UPGRADE ) {//如果是升级命令，则30分钟超时
					if ( (time(NULL)) - queue[index].send_timer > 360*CMD_TIMEOUT ) {
						//清除此记录
						queue[index].send_timer= 0 ;
						queue[index].confirm  = 0 ;
						queue[index].send_seq = 0;
						queue[index].send_pcb = 0 ;
						
						return 0 ;
					}
				}//end of ( cmd == GSM_CMD_UPGRADE )
				else { //其它命令
					if ( (time(NULL)) - queue[index].send_timer > 5*CMD_TIMEOUT ) {
						//清除此记录
						queue[index].send_timer= 0 ;
						queue[index].confirm  = 0 ;
						queue[index].send_seq = 0;
						queue[index].send_pcb = 0 ;
						
						return 0 ;
					}
				}
				
				//已确认，无需再发，直接返回即可
				return 0 ;
			}
			else { //还没收到确认
				if ( (time(NULL)) - queue[index].send_timer < CMD_TIMEOUT ) {
					//还没超时
					return 0 ;
				}
			}
		}//end of ( queue[index].send_pcb == cmd )		
	}
	
	//find a no use SENT_QUEUE to record the SEQ/CMD
	for ( index = 0 ; index < queue_cnt ; index++) {

		if ( queue[index].send_pcb == 0 ) { //no use

			//save pcb to queue
			queue[index].send_timer= (time(NULL)) ;
			queue[index].send_seq = *seq;
			queue[index].send_pcb = cmd ;
			queue[index].confirm  = 0 ;

			//handle the cmd
			switch ( cmd ) {
			
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

