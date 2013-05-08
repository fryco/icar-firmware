//TBD 键盘输入SN处理
//l SN 命令
#include "config.h"

#define console_RELEASE "\nRokko console $Rev$, "__DATE__" "__TIME__"\n"

void scan_args(int, char *[]);

unsigned char *dest="127.0.0.1", debug_flag = 0;
unsigned int server_port=23, console_time ;
	
void decode_list_all(unsigned char *buf, unsigned int buf_len)
{
	unsigned short sn_cnt, sn_total;

	sn_total = buf[6]<<8 | buf[7] ;
	
	if ( sn_total*8 > buf_len ) {//err
		fprintf(stderr,"Buf err! %s:%d\n",__FILE__,__LINE__);
		return ;
	}
	
	fprintf(stderr,"Total %d SN\n",sn_total);
	for ( sn_cnt = 0 ; sn_cnt < sn_total ; sn_cnt++ ) {
		fprintf(stderr, RED "%03d:" NONE " %01X%02X %02X%02X %02X%02X %02X%02X\t  ",sn_cnt+1,\
							buf[sn_cnt*8+8],buf[sn_cnt*8+9],\
							buf[sn_cnt*8+10],buf[sn_cnt*8+11],\
							buf[sn_cnt*8+12],buf[sn_cnt*8+13],\
							buf[sn_cnt*8+14],buf[sn_cnt*8+15]);
		//if ( !((sn_cnt+1) % 3) ) fprintf(stderr,BLUE "SN:"NONE"%d\n",sn_cnt+1);
		if ( !((sn_cnt+1) % 3) ) fprintf(stderr,"\n");
	}
	fprintf(stderr,"\n");
	return ;
}

void decode_list_spe(unsigned char *buf, unsigned int buf_len)
{	
	time_t login_time;
	
	login_time = *(unsigned int *)&buf[18];
	
	fprintf(stderr,"HW rev %X, FW rev %X\n",*(unsigned short *)&buf[6],*(unsigned short *)&buf[8]);
	fprintf(stderr,"TX %X B, RX %X B\n",*(unsigned short *)&buf[10],*(unsigned int *)&buf[14]);
	fprintf(stderr,"Login time: %s\n",(char *)ctime(&login_time));	
	return ;
}

//return 0: failure, others: buffer length
int format_cmd( unsigned char cmd, unsigned char *serial_number, \
				unsigned char *buf, unsigned int buf_len, unsigned char seq, unsigned char console_cmd)
{
	unsigned int OSTime ;
	unsigned short crc16 ;
	unsigned char var_u8;
	
	OSTime = time(NULL) - console_time;
	bzero(buf, buf_len);
	
	buf[0]   = GSM_HEAD ;
	buf[1]   = seq ;//Client => server, seq: 0~7F
	buf[2]   = cmd ;//command
	buf[3]   = 0 ;//length high
	buf[4]   = 0 ;//length low

	switch ( cmd ) {

	case GSM_CMD_LOGIN :
		//product serial number, IMEI(15),1234=>0x12 0x34

		//simulat OSTime, 4 bytes
		buf[5] = (OSTime>>24)&0xFF  ;//OSTime high
		buf[6] = (OSTime>>16)&0xFF  ;//OSTime high
		buf[7] = (OSTime>>8)&0xFF  ;//OSTime low
		buf[8] =  OSTime&0xFF  ;//OSTime low
		//fprintf(stderr,"OSTime=%X %X\t",buf[7],buf[8]);
		//fprintf(stderr,"OSTime=%X\n",OSTime);
		//Product serial number
		for ( var_u8 = 0 ; var_u8 < PRODUCT_SN_LEN ; var_u8++ ) {
			buf[var_u8+9] = serial_number[var_u8] ;
		}

		//Product HW rev, FW rev
		buf[19] =  0xFA  ;//hw revision, 1 byte
		buf[20] =  0xFA  ;//reverse
		buf[21] =  0xAF  ;//FW rev. high
		buf[22] =  0xAF  ;//FW rev. low

		//Local IP
		snprintf(&buf[23],10,"127.0.0.1");
		
		//update data length
		buf[3]   = 0 ;//length high
		buf[4]   = 27 ;//length low

		//Calc CRC16
		crc16 = crc16tablefast(buf , ((buf[3]<<8)|(buf[4]))+5);

		buf[32] = (crc16)>>8 ;
		buf[33] = (crc16)&0xFF ;

		return 34;//buffer length
		//break;

	case GSM_CMD_ERROR :
		if ( debug_flag ) fprintf(stderr,"Prepare Err log CMD, SN:%s\n",serial_number);

		//simulat OSTime, 4 bytes
		buf[5] = (OSTime>>24)&0xFF  ;//OSTime high
		buf[6] = (OSTime>>16)&0xFF  ;//OSTime high
		buf[7] = (OSTime>>8)&0xFF  ;//OSTime low
		buf[8] =  OSTime&0xFF  ;//OSTime low

		//Err msg
		//BKP_DR1, ERR index: 	15~12:MCU reset 
		//						11~8:upgrade fw failure code
		//						7~4:GPRS disconnect reason
		//						3~0:RSV
		buf[9] =  0xF0 ;// 1:POR, 2:Software rst, 4:IWDG, 8:Low power rst
		buf[10]=  0    ;//
		
		//update data length
		buf[3]   = 0 ;//length high
		buf[4]   = 6 ;//length low

		//Calc CRC16
		crc16 = crc16tablefast(buf , ((buf[3]<<8)|(buf[4]))+5);

		buf[11] = (crc16)>>8 ;
		buf[12] = (crc16)&0xFF ;

		return 13;//buffer length
		//break;

	case GSM_CMD_RECORD :
		if ( debug_flag ) fprintf(stderr,"Prepare Record CMD, SN:%s\n",serial_number);
		//format: UTC+Val(high 24 bits)+idx(low 8 bits)

		//example1: GSM singal
		//simulat OSTime, 4 bytes
		buf[5] = (OSTime>>24)&0xFF  ;//OSTime high
		buf[6] = (OSTime>>16)&0xFF  ;//OSTime high
		buf[7] = (OSTime>>8)&0xFF  ;//OSTime low
		buf[8] =  OSTime&0xFF  ;//OSTime low

		//val
		buf[9] =  0 ;
		buf[10]=  0 ;
		buf[11]=  0x1C ; //0x1C = 28, GSM signal
		buf[12]=  REC_IDX_GSM ;

		//example2: MCU temperature
		//simulat OSTime, 4 bytes
		buf[13] = (OSTime>>24)&0xFF  ;//OSTime high
		buf[14] = (OSTime>>16)&0xFF  ;//OSTime high
		buf[15] = (OSTime>>8)&0xFF  ;//OSTime low
		buf[16] =  OSTime&0xFF  ;//OSTime low

		//val, temperature/100,temperature%100
		buf[17] =  0 ;
		buf[18]=  0x0B ;
		buf[19]=  OSTime&0xFF;//random data
		buf[20]=  REC_IDX_MCU ;

		//update data length
		buf[3]   = 0 ;//length high
		buf[4]   = 16 ;//length low

		//Calc CRC16
		crc16 = crc16tablefast(buf , ((buf[3]<<8)|(buf[4]))+5);

		buf[21] = (crc16)>>8 ;
		buf[22] = (crc16)&0xFF ;

		return 23;//buffer length
		//break;

	case GSM_CMD_CONSOLE :
		
		switch ( console_cmd ) {
		
			case CONSOLE_CMD_LIST_ALL : //list all active client
				buf[5] = 'L'  ;//list all
				//update data length
				buf[3]   = 0 ;//length high
				buf[4]   = 1 ;//length low
				break;
				
			case CONSOLE_CMD_LIST_SPE : //list special active client
				buf[5] = 'l'  ;//list special
				//Product serial number
				for ( var_u8 = 0 ; var_u8 < PRODUCT_SN_LEN ; var_u8++ ) {
					buf[var_u8+6] = serial_number[var_u8] ;
				}

				//update data length
				buf[3]   = 0 ;//length high
				buf[4]   = 11 ;//length low
				break;
				
			default:
				fprintf(stderr,"Unknow CMD! @%d\n",__LINE__);
				return 0 ;
				break;
		}
		
		//Calc CRC16
		crc16 = crc16tablefast(buf , ((buf[3]<<8)|(buf[4]))+5);

		buf[((buf[3]<<8)|(buf[4]))+5] = (crc16)>>8 ;
		buf[((buf[3]<<8)|(buf[4]))+6] = (crc16)&0xFF ;

		return (((buf[3]<<8)|(buf[4]))+7);//buffer length
		//break;

	default:
		fprintf(stderr,"Unknow CMD! @%d\n",__LINE__);
		return 0 ;
		//break;
	}	
}

void send_notice( char * mail_body) {

	char mail_subject[EMAIL], err_buf[EMAIL];
	//time_t ticks=time(NULL);

	bzero( mail_subject, sizeof(mail_subject));
	bzero( err_buf, sizeof(err_buf));
	
	snprintf(mail_subject,sizeof(mail_subject),"%s:%d down!\r\n",dest,server_port);
	
	smtp_send("smtp.139.com", 25, NOTICER_ADDR, mail_subject, mail_body, err_buf);
	
	return ;
}

int kbhit(void)
{
	struct termios oldt, newt;
	int ch;
	int oldf;
	
	tcgetattr(STDIN_FILENO, &oldt);
	newt = oldt;
	newt.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &newt);
	oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
	fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
	
	ch = getchar();
	
	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
	fcntl(STDIN_FILENO, F_SETFL, oldf);
	
	if(ch != EOF)
	{
		ungetc(ch, stdin);
		return 1;
	}
	
	return 0;
}

void print_help(char *argv[])
{
	fprintf(stderr, "%s\n", console_RELEASE);
	fprintf(stderr, "usage: %s [OPTION]...\n", argv[0]);
	fprintf(stderr, "  -?             this help\n");
	fprintf(stderr, "  -d             Debug\n");
	fprintf(stderr, "  -s             Server\n");
	fprintf(stderr, "  -p             Port, default is 23\n\n");
	exit(0);
}

void scan_args(int argc, char *argv[])
{
	int index;

	while((index = getopt(argc, argv, "dhs:p:")) != EOF)
	{
		switch(index)
		{
			case '?':
			case 'h':
				print_help(argv);
				break;

			case 'd': //debug
				debug_flag = 1;
				break;

			case 's': //server
				dest = strdup(optarg);
				break;
				
			case 'p': //server port
				server_port = a2port(optarg);
				if (server_port <= 0) {
					fprintf(stderr, "Bad port number, the port must: 1 ~ 65535\n");
					exit(1);
				}
				//fprintf(stderr,"Port: %d\n",server_port);
				break;
		}
	}
	
	fprintf(stderr, "%s\n", console_RELEASE );
}

int	main(int argc, char	*argv[])
{
	unsigned char ch, seq=0;
	unsigned char buf[(MAXCLIENT*PRODUCT_SN_LEN)+20];  //数据传送的缓冲区
	//for IMEI: 123456789012345
	unsigned char console_sn[]={0x01, 0x23, 0x45, 0x67, 0x89, 0x01, 0x23, 0x45};
	unsigned char dest_sn[]={0x01, 0x23, 0x45, 0x67, 0x89, 0x01, 0x23, 0x45};
	//unsigned char dest_sn[]={0x00, 0x98, 0x76, 0x54, 0x32, 0x10, 0x12, 0x34};
	unsigned char input_sn[PRODUCT_SN_LEN*2], var_u8;
	unsigned short var_short, len;
	unsigned int usecs=10000;
	char mail_body[EMAIL*2];
	time_t last_time, server_time;

	int	client_sockfd;
	struct sockaddr_in remote_addr;
	struct timeval timeout = {1*60,0};//1 mins	

	console_time = time(NULL);
	last_time = time(NULL);

	bzero( mail_body, sizeof(mail_body));

	/* Scan arguments. */
	scan_args(argc, argv);
	fprintf(stderr,"Connect to %s:%d ... ",dest,server_port);

	bzero(&remote_addr, sizeof(remote_addr));
	remote_addr.sin_family=AF_INET;	//设置为IP通信
	remote_addr.sin_addr.s_addr=inet_addr( dest );//服务器IP地址
	remote_addr.sin_port=htons( server_port ); //服务器端口号

	if((client_sockfd=socket(PF_INET,SOCK_STREAM,0))<0)
	{
		perror("socket");
		exit(1);
	}
		
	setsockopt(client_sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,sizeof(struct timeval));
	
	if(connect(client_sockfd,(struct sockaddr *)&remote_addr,sizeof(struct sockaddr))<0)
	{
		perror("connect");
		exit(1);
	}

	fprintf(stderr,"ok\n");

	//Login to daemon
	//prepare login cmd, use "CONSOLE_01" as SN, SN len must = 10 Bytes
	len = format_cmd(GSM_CMD_LOGIN, console_sn, buf, sizeof(buf), seq, CONSOLE_CMD_NONE);
	seq++; if ( seq >= 0x80 ) seq=0;
	if ( len ) { //prepare cmd ok

		if ( debug_flag ) {
			fprintf(stderr,"--> ");
			for ( var_short = 0 ; var_short < len ; var_short++ ) {
				fprintf(stderr,"%02X ",buf[var_short]);
			}
			fprintf(stderr,"\nCMD: %c(0x%02X), Send %d Bytes\n",buf[2],buf[2],buf[4]+6);
		}
		write(client_sockfd,buf,len);
		
		bzero(buf, sizeof(buf));
		len = read(client_sockfd, buf, sizeof(buf));
			
		if ( len <= 0 ) {
			fprintf(stderr,"Rec timeout: %d @ %d in PID %d\n",len,__LINE__, getpid());
			fprintf(stderr,"Exit!\n");
			exit(1);
		}
		
		if ( buf[5] != 0 ) { //login err
			fprintf(stderr,"Login err: %d @ %d\n",buf[5],__LINE__);
			exit(1);
		}
		else {//normal return
			server_time = buf[6]<<24 | buf[7]<<16 | buf[8]<<8 | buf[9];
			fprintf(stderr,"Login succeed, server time: %s\n",(char *)ctime(&server_time));
		}
	}
	else {
		fprintf(stderr,"Can't parpare CMD @ %d Exit!\n",__LINE__);
		exit(1);
	}		

	while ( 1 ) {
		if(kbhit()) {
			last_time = time(NULL);
			ch = getchar();
			
			switch( ch ) {
				case '?':
				case 'h':
				case 'H':
					fprintf(stderr, "%s", console_RELEASE );
					fprintf(stderr, "Support CMD: H,L\n\n", console_RELEASE );
					break;
					
				case 'e':
				case 'E':
				case 'q':
				case 'Q':
					fprintf(stderr, "Exit!\n" );
					if ( debug_flag ) fprintf(stderr,"Close socket and quit.\n");
					close(client_sockfd);//关闭套接字
					exit(0);
					//break;
					
				case 'l':
					fprintf(stderr, "List special device, please input SN: ");
					bzero( input_sn, sizeof(input_sn));
					var_short = scanf("%15s",input_sn);
					if ( strlen(input_sn) == 15 ) {//len is ok						
						for ( var_short = 0; var_short < 15; var_short++ ) {
							if ( (input_sn[var_short] > 0x39) || (input_sn[var_short] < 0x30)) {
								fprintf(stderr,"Illegal %c(0x%02X), use default SN\n",\
										input_sn[var_short],input_sn[var_short]);								
								break ;
							}
							else input_sn[var_short] = input_sn[var_short] - 0x30;
						}
	                    if ( var_short == 15 ) {//all char ok
							dest_sn[0] = input_sn[0];
							for ( var_u8 = 1; var_u8 < PRODUCT_SN_LEN; var_u8++ ) {
								dest_sn[var_u8] = (input_sn[((var_u8-1)*2+1)]<<4) | input_sn[((var_u8-1)*2+2)];
								//fprintf(stderr, "%02X ", dest_sn[var_u8]);
							 }	                    	
	                    }
					}
					else {//err SN
						fprintf(stderr,"Input SN: %s ERR! len:%d, Must 15 digitals, use default SN\n",\
								input_sn,strlen(input_sn));
					}

					len = format_cmd(GSM_CMD_CONSOLE, dest_sn, buf, sizeof(buf), seq, CONSOLE_CMD_LIST_SPE);
					seq++; if ( seq >= 0x80 ) seq=0;

					if ( len ) { //prepare cmd ok

						if ( debug_flag ) {
							fprintf(stderr,"--> ");
							for ( var_short = 0 ; var_short < len ; var_short++ ) {
								fprintf(stderr,"%02X ",buf[var_short]);
							}
							fprintf(stderr,"\nCMD: %c(0x%02X), Send %d Bytes\n",buf[2],buf[2],buf[4]+6);
						}

						write(client_sockfd,buf,len);
				
						bzero(buf, sizeof(buf));
						len = read(client_sockfd, buf, sizeof(buf));
							
						if ( len <= 0 ) {
							fprintf(stderr,"Rec timeout or disconnect: %d @ %d Exit!\n",len,__LINE__);
							exit(1);
						}

						if ( buf[5] != 0 ) { //return err
							fprintf(stderr,"Return err: %d @ %d\n",buf[5],__LINE__);
							exit(1);
						}

						fprintf(stderr,"SN: ");
						fprintf(stderr,"%1X",dest_sn[0]);
						for ( var_short = 1 ; var_short < PRODUCT_SN_LEN ; var_short++ ) {
							fprintf(stderr,"%02X",dest_sn[var_short]);
						}
						fprintf(stderr," detail:\n");
						decode_list_spe(buf, len);
					}
					else {
						fprintf(stderr,"Can't parpare CMD @ %d Exit!\n",__LINE__);
						exit(1);
					}
					break;
					
				case 'L':
					fprintf(stderr, "List all device\n");

					len = format_cmd(GSM_CMD_CONSOLE, NULL, buf, sizeof(buf), seq, CONSOLE_CMD_LIST_ALL);
					seq++; if ( seq >= 0x80 ) seq=0;

					if ( len ) { //prepare cmd ok

						if ( debug_flag ) {
							fprintf(stderr,"--> ");
							for ( var_short = 0 ; var_short < len ; var_short++ ) {
								fprintf(stderr,"%02X ",buf[var_short]);
							}
							fprintf(stderr,"\nCMD: %c(0x%02X), Send %d Bytes\n",buf[2],buf[2],buf[4]+6);
						}

						write(client_sockfd,buf,len);
				
						bzero(buf, sizeof(buf));
						len = read(client_sockfd, buf, sizeof(buf));
							
						if ( len <= 0 ) {
							fprintf(stderr,"Rec timeout or disconnect: %d @ %d Exit!\n",len,__LINE__);
							exit(1);
						}

						if ( buf[5] != 0 ) { //return err
							fprintf(stderr,"Return err: %d @ %d\n",buf[5],__LINE__);
							exit(1);
						}

						fprintf(stderr,"Rec : %d @ %d\n",len,__LINE__);
						decode_list_all(buf, len);
					}
					else {
						fprintf(stderr,"Can't parpare CMD @ %d Exit!\n",__LINE__);
						exit(1);
					}
					break;

				default:
					fprintf(stderr,"Unknow CMD: %c (0x%X) @%d\n",ch,ch,__LINE__);
					break;
			}
		}
		
		if ( (time(NULL) - last_time) > 3 ) {//update every 3 seconds
			last_time = time(NULL);
			fprintf(stderr,"==> %s\n",(char *)ctime(&last_time));
			
			len = format_cmd(GSM_CMD_CONSOLE, NULL, buf, sizeof(buf), seq, CONSOLE_CMD_LIST_ALL);
			seq++; if ( seq >= 0x80 ) seq=0;

			if ( len ) { //prepare cmd ok

				if ( debug_flag ) {
					fprintf(stderr,"--> ");
					for ( var_short = 0 ; var_short < len ; var_short++ ) {
						fprintf(stderr,"%02X ",buf[var_short]);
					}
					fprintf(stderr,"\nCMD: %c(0x%02X), Send %d Bytes\n",buf[2],buf[2],buf[4]+6);
				}

				write(client_sockfd,buf,len);
		
				bzero(buf, sizeof(buf));
				len = read(client_sockfd, buf, sizeof(buf));
					
				if ( len <= 0 ) {
					bzero(mail_body, sizeof(mail_body));
					snprintf(mail_body,sizeof(mail_body),"Read sock return: %d\r\n%s\r\n",\
							len, (char *)ctime(&last_time));							
					send_notice( mail_body );
					fprintf(stderr,"%s @ %d Exit!\n",mail_body,__LINE__);
					
					exit(1);
				}

				if ( buf[5] != 0 ) { //return err
					bzero(mail_body, sizeof(mail_body));
					for ( var_short = 0 ; var_short < 8 ; var_short++ ) {
						snprintf(&mail_body[var_short*3],4,"%02X ",buf[var_short]);
					}
					snprintf(&mail_body[var_short*3],sizeof(mail_body)-var_short*3,\
							"\nCMD return err: %d at %s\r\n",\
							buf[5], (char *)ctime(&last_time));	
					send_notice( mail_body );
					fprintf(stderr,"%s @ %d Exit!\n",mail_body,__LINE__);
					//exit(1);
				}
				else {
					fprintf(stderr,"Rec : %d @ %d\n",len,__LINE__);
					decode_list_all(buf, len);
				}
			}
			else {
				snprintf(mail_body,sizeof(mail_body),"Can't parpare CMD @ %d\r\n%s\r\n",\
						__LINE__,(char *)ctime(&last_time));							
				send_notice( mail_body );
				fprintf(stderr,"%s @ %d Exit!\n",mail_body,__LINE__);
				exit(1);
			}
		}
		usleep(usecs);		
	}
}
