#include "config.h"

#define console_RELEASE "\nRokko console $Rev$, "__DATE__" "__TIME__"\n"

void scan_args(int, char *[]);

unsigned char *dest="127.0.0.1", debug_flag = 0;
unsigned int server_port=23, console_time ;
	
//return 0: failure, others: buffer length
int format_cmd( unsigned char cmd, unsigned char *serial_number, \
				unsigned char *buf, unsigned int buf_len, unsigned char seq)
{
	unsigned int OSTime ;
	unsigned short crc16 ;
	
	OSTime = time(NULL) - console_time;
	bzero(buf, buf_len);
	
	buf[0]   = GSM_HEAD ;
	buf[1]   = seq ;//Client => server, seq: 0~7F
	buf[2]   = cmd ;//command
	buf[3]   = 0 ;//length high
	buf[4]   = 0 ;//length low

	switch ( cmd ) {

	case GSM_CMD_LOGIN :
		//if ( debug_flag ) fprintf(stderr,"Prepare Login CMD, SN:%s\n",serial_number);

		//simulat OSTime, 4 bytes
		buf[5] = (OSTime>>24)&0xFF  ;//OSTime high
		buf[6] = (OSTime>>16)&0xFF  ;//OSTime high
		buf[7] = (OSTime>>8)&0xFF  ;//OSTime low
		buf[8] =  OSTime&0xFF  ;//OSTime low
		//fprintf(stderr,"OSTime=%X %X\t",buf[7],buf[8]);
		//fprintf(stderr,"OSTime=%X\n",OSTime);
		//Product serial number
		snprintf(&buf[9],11,"%s",serial_number);

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
		//simulat OSTime, 4 bytes
		buf[5] = 'L'  ;//list
		
		if ( serial_number == NULL ) {
			buf[6] = 0xFF ; //SN_1
			buf[7] = 0xFF ; //SN_2
			buf[8] = 0xFF ; //SN_3
			buf[9] = 0xFF ; //SN_4
			buf[10]= 0xFF ; //SN_5
			buf[11]= 0xFF ; //SN_6
			buf[12]= 0xFF ; //SN_7
			buf[13]= 0xFF ; //SN_8
			buf[14]= 0xFF ; //SN_9
			buf[15]= 0xFF ; //SN_10
			//if ( debug_flag ) fprintf(stderr,"Prepare Console CMD, SN:%X...%X\n",buf[6],buf[15]);
		}
		else {		
			snprintf(&buf[6],11,"%s",serial_number);
			//if ( debug_flag ) fprintf(stderr,"Prepare Console CMD, SN:%s\n",serial_number);
		}
		
			
		
		//update data length
		buf[3]   = 0 ;//length high
		buf[4]   = 11 ;//length low

		//Calc CRC16
		crc16 = crc16tablefast(buf , ((buf[3]<<8)|(buf[4]))+5);

		buf[16] = (crc16)>>8 ;
		buf[17] = (crc16)&0xFF ;

		return 18;//buffer length
		//break;

	default:
		fprintf(stderr,"Unknow CMD! @%d\n",__LINE__);
		return 0 ;
		//break;
	}	
}

void send_notice( char * mail_body) {

	char mail_subject[BUFSIZE+1], err_buf[BUFSIZE+1];
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
	unsigned char buf[BUFSIZE];  //数据传送的缓冲区
	unsigned short var_short, len;
	unsigned int usecs=10000;
	char mail_body[BUFSIZE*2];
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
	len = format_cmd(GSM_CMD_LOGIN, "CONSOLE_01", buf, BUFSIZE, seq);
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
		len = read(client_sockfd, buf, BUFSIZE);
			
		if ( len <= 0 ) {
			fprintf(stderr,"Rec timeout: %d @ %d in PID %d\n",len,__LINE__, getpid());
			fprintf(stderr,"Exit!\n");
			exit(1);
		}
		else {
			if ( len < 20 ) {//Login CMD return TIME+MCU_ID, 24 Bytes
				fprintf(stderr,"Rec error: %d @ %d Exit!\n",len,__LINE__);
				exit(1);
			}
			else {//normal return
				server_time = buf[6]<<24 | buf[7]<<16 | buf[8]<<8 | buf[9];
				fprintf(stderr,"Login succeed, server time: %s\n",(char *)ctime(&server_time));
			}
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
				case 'L':
					fprintf(stderr, "List device\n");

					len = format_cmd(GSM_CMD_CONSOLE, "CONSOLE_01", buf, BUFSIZE, seq);
					//len = format_cmd(GSM_CMD_CONSOLE, NULL, buf, BUFSIZE, seq);
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
						len = read(client_sockfd, buf, BUFSIZE);
							
						if ( len <= 0 ) {
							fprintf(stderr,"Rec timeout or disconnect: %d @ %d Exit!\n",len,__LINE__);
							exit(1);
						}
						else {
							fprintf(stderr,"Rec : %d @ %d\n",len,__LINE__);
						}
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
			
					//len = format_cmd(GSM_CMD_CONSOLE, "CONSOLE_01", buf, BUFSIZE, seq);
					len = format_cmd(GSM_CMD_CONSOLE, NULL, buf, BUFSIZE, seq);
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
						len = read(client_sockfd, buf, BUFSIZE);
							
						if ( len <= 0 ) {
							snprintf(mail_body,sizeof(mail_body),"Read sock return: %d\r\n%s\r\n",\
									len, (char *)ctime(&last_time));							
							send_notice( mail_body );
							fprintf(stderr,"%s @ %d Exit!\n",mail_body,__LINE__);
							
							exit(1);
						}
						else {
							fprintf(stderr,"Rec : %d @ %d\n",len,__LINE__);
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

/*//Return 0: ok, others err
int single_connect( unsigned int simu_id ) {
				
	{
		//fprintf(stderr,"Run @ %d\n",__LINE__);
		{
			else {
				if ( debug_flag ) fprintf(stderr,"<-- %d Bytes:",len);

				if ( len < 20 ) {//Login CMD return TIME+MCU_ID, 24 Bytes
					printf("<-- %d Bytes:",len);
					for ( var_int = 0 ; var_int < len ; var_int++ ) {
						printf(" %02X",buf[var_int]);
					}
					printf("\nError and exit @ %d\n",__LINE__);
					
					return 1 ;
				}
				else {//normal return, send err log msg
					//DE 01 45 00 06 00 00 00 08 30 00 81 79, time+reason
					len = format_cmd(GSM_CMD_ERROR, 0, buf, BUFSIZE, seq);
					seq++; if ( seq >= 0x80 ) seq=0;
					
					if ( len ) { //prepare cmd ok		
						write(client_sockfd,buf,len);
						if ( debug_flag ) fprintf(stderr,"--> Err log: %02X, Send %d Bytes\n",buf[2],buf[4]+6);
					}
					bzero(buf, sizeof(buf));
					len = read(client_sockfd, buf, BUFSIZE);
					//DE 01 C5 00 02 00 04 08 4D
					if ( debug_flag ) fprintf(stderr,"<-- %c, len: %d \n",buf[2]&0x7F,len);
						
					//Send Record: vehicle parameters
					//DE 04 52 00 08 51 1F 4C 4E 00 0C 30 1A A5 1C
					len = format_cmd(GSM_CMD_RECORD, 0, buf, BUFSIZE, seq);
					seq++; if ( seq >= 0x80 ) seq=0;

					if ( len ) { //prepare cmd ok		
						write(client_sockfd,buf,len);
						if ( debug_flag ) fprintf(stderr,"--> Record: %02X, Send %d Bytes\n",buf[2],buf[4]+6);
					}
					bzero(buf, sizeof(buf));
					len = read(client_sockfd, buf, BUFSIZE);
					//DE 04 D2 00 02 00 00 68 46
					if ( debug_flag ) fprintf(stderr,"<-- %c, len: %d \n",buf[2]&0x7F,len);

				}
			}
		}
	}

	//sleep(1);

	return 0;
}
*/