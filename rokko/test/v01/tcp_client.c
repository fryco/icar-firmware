#include "config.h"

#define rokko_RELEASE "\nRokko client simulate v00, built by Jack at " __DATE__" "__TIME__ "\n"

void scan_args(int, char *[]);
int single_connect( unsigned int ) ;

static const unsigned char cmd_login[ ] = 
{ 
	0xDE, 0x00, 0x4C, 0x00, 0x1E, 0x00, 0x00, 0x08, 0xB6, 0x44, 
	0x45, 0x4D, 0x4F, 0x43, 0x34, 0x33, 0x45, 0x45, 0x39, 0x00,
	0x00, 0x00, 0xDE, 0x31, 0x30, 0x2E, 0x38, 0x31, 0x2E, 0x32,
	0x33, 0x37, 0x2E, 0x39, 0x36, 0xFF, 0xE8, 0xDE, 0x01, 0x45,
	0x00, 0x06, 0x00, 0x00, 0x00, 0x08, 0x30, 0x00, 0x81, 0x79
};


static const unsigned char cmd_time[ ] = 
{ 
	0xC9, 0x00, 0x54, 0x00, 0x0A, 0x30, 0x32, 0x50, 0x31, 0x31,
	0x41, 0x48, 0x30, 0x30, 0x30, 0xFC
};

unsigned char *dest="127.0.0.1", debug_flag = 0;
unsigned int server_port=23, max_client = 1 ;
unsigned int process_cnt = 0 , rokko_time ;
unsigned long long err_cnt = 0 ;
	
//return 0: failure, others: buffer length
int format_cmd( unsigned char cmd, unsigned int id, unsigned char *buf, unsigned int buf_len)
{
	unsigned int OSTime ;
	unsigned short crc16 ;
	
	OSTime = time(NULL) - rokko_time;
	bzero(buf, buf_len);
	
	buf[0]   = GSM_HEAD ;
	buf[1]   = 31 ;//Client => server, seq: 0~7F
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
		snprintf(&buf[9],11,"simu%06d",id);

		//Product HW rev, FW rev
		buf[19] =  0  ;//hw revision, 1 byte
		buf[20] =  0  ;//reverse
		buf[21] =  0  ;//FW rev. high
		buf[22] =  38  ;//FW rev. low

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
				
	int	client_sockfd, len=0;
	unsigned int var_int, run_cnt=0 ;
	struct sockaddr_in remote_addr;	//服务器端网络地址结构体
	unsigned char buf[BUFSIZE];  //数据传送的缓冲区
	struct timeval timeout = {3*60,0};//3 mins

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
	
/*	len = read(client_sockfd, buf, BUFSIZE); 
	if ( len <= 0 ) {
		fprintf(stderr,"Rec timeout: %d @ %d in PID %d\n",len,__LINE__, getpid());
	}
	else {
		buf[len]='\0';
		if ( debug_flag ) fprintf(stderr,"%s",buf); //打印服务器端信息
	}
*/	
	{
		fprintf(stderr,"Run @ %d\n",__LINE__);
		while ( 1 ) {
			//prepare cmd
			len = format_cmd(GSM_CMD_LOGIN, getpid(), buf, BUFSIZE);
			
			if ( len ) { //prepare cmd ok
				fprintf(stderr,"ID:%s => %d\n",&buf[9],run_cnt);
				run_cnt++;

				write(client_sockfd,buf,len);
				if ( debug_flag ) fprintf(stderr,"--> Login CMD: %02X, Send %d Bytes\n",buf[2],buf[4]+6);
			}
			bzero(buf, sizeof(buf));
			len = read(client_sockfd, buf, BUFSIZE);
			
			if ( len <= 0 ) {
				fprintf(stderr,"Rec timeout: %d @ %d in PID %d\n",len,__LINE__, getpid());
				return 1 ;
			}
			else {
				if ( debug_flag ) fprintf(stderr,"<-- %d Bytes:",len);

				if ( len < 5 ) {
					printf("<-- %d Bytes:",len);
					for ( var_int = 0 ; var_int < len ; var_int++ ) {
						printf(" %02X",buf[var_int]);
					}
					printf("\n\n");
					
					return 1 ;
				}
			}

			if ( debug_flag ) fprintf(stderr,"\n\n");
			
			sleep(1);
		}
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
