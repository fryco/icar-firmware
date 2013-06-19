#include "config.h"

#define rokko_RELEASE "\nRokko client simulate v00, built by Jack at " __DATE__" "__TIME__ "\n"

void scan_args(int, char *[]);
int single_connect( unsigned int ) ;

unsigned char *dest="127.0.0.1", debug_flag = 0;
unsigned int server_port=23, max_client = 1 ;
unsigned int process_cnt = 0 , rokko_time ;
unsigned long long err_cnt = 0 ;
	
//return 0: failure, others: buffer length
int format_cmd( unsigned char cmd, unsigned int id, unsigned char *buf, unsigned int buf_len,\
				unsigned char seq, struct gps_struct *gps)
{
	unsigned int OSTime ;
	unsigned short crc16 ;
	unsigned char sn_buf[PRODUCT_SN_LEN*2], var_u8;
	
	OSTime = time(NULL) - rokko_time;
	bzero(buf, buf_len);
	
	buf[0]   = GSM_HEAD ;
	buf[1]   = seq ;//Client => server, seq: 0~7F
	buf[2]   = cmd ;//command
	buf[3]   = 0 ;//length high
	buf[4]   = 0 ;//length low

	switch ( cmd ) {

	case GSM_CMD_LOGIN :
		if ( debug_flag ) fprintf(stderr,"Prepare Login CMD, id:%d\n",id);

		//simulat OSTime, 4 bytes
		buf[5] = (OSTime>>24)&0xFF  ;//OSTime high
		buf[6] = (OSTime>>16)&0xFF  ;//OSTime high
		buf[7] = (OSTime>>8)&0xFF  ;//OSTime low
		buf[8] =  OSTime&0xFF  ;//OSTime low
		//fprintf(stderr,"OSTime=%X %X\t",buf[7],buf[8]);
		//Product serial number
		snprintf(sn_buf,sizeof(sn_buf),"9977553311%05d",id);
		//fprintf(stderr,"SN:%s\n",sn_buf);
		
		buf[9] = (sn_buf[0]-0x30)&0x0F ;
		for ( var_u8 = 0 ; var_u8 < PRODUCT_SN_LEN-1 ; var_u8++ ) {
			buf[var_u8+10] = ((sn_buf[var_u8*2+1]-0x30)<<4)&0xF0 | (sn_buf[var_u8*2+2]-0x30)&0x0F;
		}

		//Product HW rev, FW rev
		buf[19] =  0x12  ;//hw revision, 1 byte
		buf[20] =  0x34  ;//reverse
		buf[21] =  0x56  ;//FW rev. high
		buf[22] =  0x78  ;//FW rev. low

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
		if ( debug_flag ) fprintf(stderr,"Prepare Err log CMD, id:%d\n",id);

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

	case GSM_CMD_GPS :
		if ( debug_flag ) fprintf(stderr,"Prepare GPS data, id:%d\n",id);
		//format: UTC+SAT(1B)+LAT(4B)+LONG(4B)+speed(1B)+status(2B)

		//UTC, 4 bytes
		buf[5] = (time(NULL)>>24)&0xFF  ;
		buf[6] = (time(NULL)>>16)&0xFF  ;//time(NULL) high
		buf[7] = (time(NULL)>>8)&0xFF  ;//time(NULL) low
		buf[8] =  time(NULL)&0xFF  ;	//time(NULL) low

		//SAT: fix satellite
		buf[9] =  ((time(NULL)&0x70)>>4)+3 ;

		//LAT
		*(unsigned int *)&buf[10] = gps->lat;

		//LON
		*(unsigned int *)&buf[14] = gps->lon;
		
		//speed(1B)
		buf[18]=  time(NULL)&0xBF;//random data, < 191

		//status(2B)
		//bit6~7, 定位方式: 1: 未定位，2：2D定位，3：3D定位
		//bit5: 0, bit4: rsv
		//bit 3: 东西经：0：东经，1：西经
		//bit 2: 南北纬：0：南纬，1：北纬
		//bit0~1: Track angle in degrees True, high
		buf[19] =  0xC0 | 0x00 | 0x04 | 0x01 ;
		
		//Track angle in degrees True, low
		buf[20]=  (OSTime>>8)&0xFF ;//random 

		//update data length
		buf[3]   = 0 ;//length high
		buf[4]   = 16 ;//length low

		//Calc CRC16
		crc16 = crc16tablefast(buf , ((buf[3]<<8)|(buf[4]))+5);

		buf[((buf[3]<<8)|(buf[4]))+5] = (crc16)>>8 ;
		buf[((buf[3]<<8)|(buf[4]))+6] = (crc16)&0xFF ;

		return (((buf[3]<<8)|(buf[4]))+7);//buffer length

	case GSM_CMD_RECORD :
		if ( debug_flag ) fprintf(stderr,"Prepare Record CMD, id:%d\n",id);
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

		//example3: TP1 voltage
		//simulat OSTime, 4 bytes
		buf[21] = (OSTime>>24)&0xFF  ;//OSTime high
		buf[22] = (OSTime>>16)&0xFF  ;//OSTime high
		buf[23] = (OSTime>>8)&0xFF  ;//OSTime low
		buf[24] =  OSTime&0xFF  ;//OSTime low

		//val, v_tp1, /100, %100, 3.29V
		buf[25] =  0 ;
		buf[26]=  0x01 ;
		buf[27]=  0x40 | (OSTime&0x0F);//random data
		buf[28]=  REC_IDX_V_TP1 ;

		//example4: TP2 voltage
		//simulat OSTime, 4 bytes
		buf[29] = (OSTime>>24)&0xFF  ;//OSTime high
		buf[30] = (OSTime>>16)&0xFF  ;//OSTime high
		buf[31] = (OSTime>>8)&0xFF  ;//OSTime low
		buf[32] =  OSTime&0xFF  ;//OSTime low

		//val, v_tp2, /100, %100, 11.80V
		buf[33] =  0 ;
		buf[34]=  0x04 ;
		buf[35]=  0xA0 | (OSTime&0x0F);//random data
		buf[36]=  REC_IDX_V_TP2 ;

		//example5: TP3 voltage
		//simulat OSTime, 4 bytes
		buf[37] = (OSTime>>24)&0xFF  ;//OSTime high
		buf[38] = (OSTime>>16)&0xFF  ;//OSTime high
		buf[39] = (OSTime>>8)&0xFF  ;//OSTime low
		buf[40] =  OSTime&0xFF  ;//OSTime low

		//val, v_tp3, /100, %100, 1.78V
		buf[41] =  0 ;
		buf[42]=  0x00 ;
		buf[43]=  0xB0 | (OSTime&0x0F);//random data
		buf[44]=  REC_IDX_V_TP3 ;

		//example6: TP4 voltage
		//simulat OSTime, 4 bytes
		buf[45] = (OSTime>>24)&0xFF  ;//OSTime high
		buf[46] = (OSTime>>16)&0xFF  ;//OSTime high
		buf[47] = (OSTime>>8)&0xFF  ;//OSTime low
		buf[48] =  OSTime&0xFF  ;//OSTime low

		//val, v_tp4, /100, %100, 3.87V
		buf[49] =  0 ;
		buf[50]=  0x01 ;
		buf[51]=  0x70 | (OSTime&0x0F);//random data
		buf[52]=  REC_IDX_V_TP4 ;

		//example7: TP5 voltage
		//simulat OSTime, 4 bytes
		buf[53] = (OSTime>>24)&0xFF  ;//OSTime high
		buf[54] = (OSTime>>16)&0xFF  ;//OSTime high
		buf[55] = (OSTime>>8)&0xFF  ;//OSTime low
		buf[56] =  OSTime&0xFF  ;//OSTime low

		//val, v_tp5, /100, %100, 5.00V
		buf[57] =  0 ;
		buf[58]=  0x01 ;
		buf[59]=  0xF0 | (OSTime&0x0F);//random data
		buf[60]=  REC_IDX_V_TP5 ;

		//example8: ADC1
		//simulat OSTime, 4 bytes
		buf[61] = (OSTime>>24)&0xFF  ;//OSTime high
		buf[62] = (OSTime>>16)&0xFF  ;//OSTime high
		buf[63] = (OSTime>>8)&0xFF  ;//OSTime low
		buf[64] =  OSTime&0xFF  ;//OSTime low

		//val, ADC1, 0x789
		buf[65] =  0 ;
		buf[66]=  0x07 ;
		buf[67]=  0x89 ;
		//buf[59]=  0xF0 | (OSTime&0x0F);//random data
		buf[68]=  REC_IDX_ADC1 ;

		//update data length
		buf[3]   = 0 ;//length high
		buf[4]   = 64 ;//length low

		//Calc CRC16
		crc16 = crc16tablefast(buf , ((buf[3]<<8)|(buf[4]))+5);

		buf[((buf[3]<<8)|(buf[4]))+5] = (crc16)>>8 ;
		buf[((buf[3]<<8)|(buf[4]))+6] = (crc16)&0xFF ;

		return (((buf[3]<<8)|(buf[4]))+7);//buffer length

	default:
		fprintf(stderr,"Unknow CMD! @%d\n",__LINE__);
		return 0 ;
		//break;
	}	
}

void print_help(char *argv[])
{
	fprintf(stderr, "%s\n", rokko_RELEASE);
	fprintf(stderr, "usage: %s [OPTION]...\n", argv[0]);
	fprintf(stderr, "commandline options override settings from configuration file\n\n");
	fprintf(stderr, "  -?             this help\n");
	fprintf(stderr, "  -c             Max. Client\n");
	fprintf(stderr, "  -d             Debug\n");
	fprintf(stderr, "  -s             Server\n");
	fprintf(stderr, "  -p             Port, default is 23\n\n");
	exit(0);
}

void scan_args(int argc, char *argv[])
{
	int index;

	while((index = getopt(argc, argv, "c:dhs:p:")) != EOF)
	{
		switch(index)
		{
			case '?':
			case 'h':
				print_help(argv);
				break;
			case 'c': //max client
				max_client = a2port(optarg);
				if (max_client <= 0) {
					fprintf(stderr, "Bad process_cnt number, must: 1 ~ 3000\n");
					exit(1);
				}
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
	
	fprintf(stderr, "%s\n", rokko_RELEASE );
}

int	main(int argc, char	*argv[])
{
	void child_exit(int);
	//unsigned int var_int = 0 ;
	unsigned long long chk_cnt = 0 ;
	time_t last_time = 0;

	rokko_time = time(NULL);//for report simulator running time
	
	/* Scan arguments. */
	scan_args(argc, argv);

	fprintf(stderr,"Max. client: %d\n",max_client);
	fprintf(stderr,"Server %s:%d\n",dest,server_port);
	//printf("Parent: %d\n",getpid());
	signal(SIGCHLD, child_exit);

	while ( 1 ) {
		while ( process_cnt < max_client ) {
			
			switch(fork())
			{
				case 0:
					if ( debug_flag ) fprintf(stderr,"In child: %d\n",getpid());
					if ( single_connect( process_cnt+1 ) ) {
						if ( debug_flag ) fprintf(stderr, "Test failure!\n");
						exit(1);
					}
					exit(0);
				case -1:
					perror("fork failed"); exit(1);
				default:
					process_cnt++;
					chk_cnt++;
					break;
			}
		}

		if ( time(NULL) - last_time > 5 ) {
			printf("Test %lld\tpass:%lld\tfailure:%lld\n",chk_cnt,chk_cnt-err_cnt,err_cnt);
			last_time = time(NULL);
		}
		else{
			sleep( 1 ) ;		
		}
	}
}

int single_connect( unsigned int simu_id ) {
				
	unsigned char var_u8 , seq=0;
	int	client_sockfd, len=0;
	unsigned int run_cnt=0;
	unsigned int lat_ori=0, lon_ori=0, lat_offset=0, lon_offset=0 ;
	struct sockaddr_in remote_addr;	//服务器端网络地址结构体
	unsigned char buf[BUFSIZE];  //数据传送的缓冲区
	struct timeval timeout = {3*60,0};//3 mins
	struct gps_struct gps;

	bzero(&remote_addr, sizeof(remote_addr));

	remote_addr.sin_family=AF_INET;	//设置为IP通信
	remote_addr.sin_addr.s_addr=inet_addr( dest );//服务器IP地址
	remote_addr.sin_port=htons( server_port ); //服务器端口号

	if((client_sockfd=socket(PF_INET,SOCK_STREAM,0))<0)
	{
		perror("socket");
		return 1;
	}

	setsockopt(client_sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,sizeof(struct timeval));

	if(connect(client_sockfd,(struct sockaddr *)&remote_addr,sizeof(struct sockaddr))<0)
	{
		perror("connect");
		return 1;
	}


	if ( debug_flag ) fprintf(stderr,"connected to server: %s:%d\n\n",dest,server_port);
	
	//prepare login cmd, use pid as SN
	len = format_cmd(GSM_CMD_LOGIN, getpid(), buf, BUFSIZE, seq, NULL);
	seq++; if ( seq >= 0x80 ) seq=0;
	if ( len ) { //prepare cmd ok
		fprintf(stderr,"ID:%05d => %d\n",getpid(),run_cnt);
		run_cnt++;

		write(client_sockfd,buf,len);
		if ( debug_flag ) fprintf(stderr,"--> Login CMD: %02X, Send %d Bytes\n",buf[2],buf[4]+7);
	}
	bzero(buf, sizeof(buf));
	len = read(client_sockfd, buf, BUFSIZE);
	
	if ( len <= 0 ) {
		fprintf(stderr,"Rec timeout: %d @ %d in PID %d\n",len,__LINE__, getpid());
		return 1 ;
	}

	if ( buf[5] != 0 ) { //login err
		fprintf(stderr,"Login err: %d @ %d\n",buf[5],__LINE__);
		return 1 ;
	}		

	//while( 1 ) sleep(1);

	while ( 1 ) {//login ok

		//normal return, send err log msg
		//DE 01 45 00 06 00 00 00 08 30 00 81 79, time+reason
		len = format_cmd(GSM_CMD_ERROR, 0, buf, BUFSIZE, seq, NULL);
		seq++; if ( seq >= 0x80 ) seq=0;
		
		if ( len ) { //prepare cmd ok		
			write(client_sockfd,buf,len);
			if ( debug_flag ) fprintf(stderr,"--> Err log: %02X, Send %d Bytes\n",buf[2],buf[4]+7);
		}
		bzero(buf, sizeof(buf));
		len = read(client_sockfd, buf, BUFSIZE);
		//DE 01 C5 00 02 00 04 08 4D
		if ( debug_flag ) fprintf(stderr,"<-- %c, len: %d \n",buf[2]&0x7F,len);
		if ( buf[5] != 0 ) { //err
			fprintf(stderr,"Return err: %d @ %d\n",buf[5],__LINE__);
			return 1 ;
		}		
			
		//Send Record: vehicle parameters
		//DE 04 52 00 08 51 1F 4C 4E 00 0C 30 1A A5 1C
		len = format_cmd(GSM_CMD_RECORD, 0, buf, BUFSIZE, seq, NULL);
		seq++; if ( seq >= 0x80 ) seq=0;

		if ( len ) { //prepare cmd ok		
			write(client_sockfd,buf,len);
			if ( debug_flag ) fprintf(stderr,"--> Record: %02X, Send %d Bytes\n",buf[2],buf[4]+7);
		}
		bzero(buf, sizeof(buf));
		len = read(client_sockfd, buf, BUFSIZE);
		//DE 04 D2 00 02 00 00 68 46
		if ( debug_flag ) fprintf(stderr,"<-- %c, len: %d \n\n",buf[2]&0x7F,len);
		if ( buf[5] != 0 ) { //err
			fprintf(stderr,"Return err: %d @ %d\n",buf[5],__LINE__);
			return 1 ;
		}
		
		//Send GPS information
		//DE 
		//LAT: 22 °32.7658 ==>(22*60+32.7658)*30000=40582974=0x02 0x6B 0x3F 0x3E
		//Use PID as seed
		//Orginal point
		lat_ori = ((getpid())&0x3F)*60*30000;
		lon_ori = (((getpid())>>8)&0x3F)*60*30000;
		
		for ( var_u8 = 0 ; var_u8 < 10 ; var_u8++ ) {//upload 10 points each time
			gps.lat = lat_ori + lat_offset ;	lat_offset = lat_offset + 33 ;
			gps.lon = lon_ori + lon_offset ;	lon_offset = lon_offset + 47 ;
			len = format_cmd(GSM_CMD_GPS, getpid(), buf, BUFSIZE, seq, &gps);
			seq++; if ( seq >= 0x80 ) seq=0;
	
			if ( len ) { //prepare cmd ok		
				write(client_sockfd,buf,len);
				if ( debug_flag ) {
					fprintf(stderr,"--> Record: %02X, Send %d Bytes\n--> ",buf[2],buf[4]+7);
					for ( len = 0 ; len < buf[4]+7 ; len++ ){
						fprintf(stderr,"%02X ",buf[len]);
					}
					fprintf(stderr,"\n");
				}
			}
			bzero(buf, sizeof(buf));
			len = read(client_sockfd, buf, BUFSIZE);
			//DE 04 D2 00 02 00 00 68 46
			if ( debug_flag ) fprintf(stderr,"<-- %c, len: %d \n\n",buf[2]&0x7F,len);
			if ( buf[5] != 0 ) { //err
				fprintf(stderr,"Return err: %d @ %d\n",buf[5],__LINE__);
				return 1 ;
			}
			sleep(1);
		}
		sleep(1);
	}

	//sleep(1);
	if ( debug_flag ) fprintf(stderr,"Close socket and quit.\n");
	close(client_sockfd);//关闭套接字

	return 0;
}

void child_exit(int num)
{
	//Received SIGCHLD signal
	int status;

	int pid = waitpid(-1, &status, WNOHANG);
	if (WIFEXITED(status)) {
		if ( debug_flag ) fprintf(stderr,"Child %d exit with code %d\n", pid, WEXITSTATUS(status));
		if ( WEXITSTATUS(status) ) {
			err_cnt++;
		}
	}
	
	if ( process_cnt > 0 ) process_cnt--;
}
