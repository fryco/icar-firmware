#include "config.h"

int single_connect( void ) ;

//#define DEBUG_CONN

#ifdef DEBUG_CONN
	#define debug_conn		printf
#else
	#define debug_conn		//
#endif

static const unsigned char cmd_login[ ] = 
{ 
	0xC9, 0x01, 0x53, 0x00, 0x1B, 0x00, 0x00, 0x0D, 0x99, 0x30,
	0x32, 0x50, 0x31, 0x43, 0x30, 0x44, 0x32, 0x41, 0x37, 0x31,
	0x30, 0x2E, 0x32, 0x30, 0x31, 0x2E, 0x31, 0x33, 0x37, 0x2E,
	0x32, 0x37, 0x28
};


static const unsigned char cmd_time[ ] = 
{ 
	0xC9, 0x00, 0x54, 0x00, 0x0A, 0x30, 0x32, 0x50, 0x31, 0x31,
	0x41, 0x48, 0x30, 0x30, 0x30, 0xFC
};

unsigned int process_cnt = 0 ;
unsigned long long err_cnt = 0 ;
	
int	main(int argc, char	*argv[])
{
	void child_exit(int);
	//unsigned int var_int = 0 ;
	unsigned long long chk_cnt = 0 ;
	time_t last_time = 0;
	
	printf("Parent: %d\n",getpid());
	signal(SIGCHLD, child_exit);
	while ( 1 ) {
		while ( process_cnt < PROCESS_CNT ) {
			
			switch(fork())
			{
				case 0:
					debug_conn(stderr,"In child: %d\n",getpid());
					if ( single_connect( ) ) {
						debug_conn("Test failure!\n");
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

int single_connect( void ) {
				
	int	client_sockfd;
	unsigned int var_int, cmd_cnt, len;
	struct sockaddr_in remote_addr;	//服务器端网络地址结构体
	unsigned char buf[BUFSIZE];  //数据传送的缓冲区

	memset(&remote_addr,0,sizeof(remote_addr));	//数据初始化--清零
	remote_addr.sin_family=AF_INET;	//设置为IP通信
	remote_addr.sin_addr.s_addr=inet_addr( SERVER_ADDR );//服务器IP地址
	remote_addr.sin_port=htons( SERVER_PORT ); //服务器端口号


	if((client_sockfd=socket(PF_INET,SOCK_STREAM,0))<0)
	{
		perror("socket");
		return 1;
	}


	if(connect(client_sockfd,(struct sockaddr *)&remote_addr,sizeof(struct sockaddr))<0)
	{
		perror("connect");
		return 1;
	}
	debug_conn(stderr,"connected to server: %s:%d\n\n",SERVER_ADDR,SERVER_PORT);
	//len=recv(client_sockfd,buf,BUFSIZE,0);//接收服务器端信息
	len = read(client_sockfd, buf, BUFSIZE); buf[len]='\0';
	debug_conn(stderr,"%s",buf); //打印服务器端信息

	if ( CMD_CNT == 0 ) {
		
		while ( 1 ) {
			write(client_sockfd,cmd_login,cmd_login[4]+6);
			debug_conn(stderr,"--> Login CMD: %02X, Send %d Bytes\n",cmd_login[2],cmd_login[4]+6);
			
			len = read(client_sockfd, buf, BUFSIZE);
			if ( len < 5 ) {
				printf("<-- %d Bytes:",len);
				for ( var_int = 0 ; var_int < len ; var_int++ ) {
					printf(" %02X",buf[var_int]);
				}
				printf("\n\n");
				
				return 1 ;
			}
			debug_conn(stderr,"<-- %d Bytes:",len);
			for ( var_int = 0 ; var_int < len ; var_int++ ) {
				debug_conn(stderr," %02X",buf[var_int]);
			}
			debug_conn(stderr,"\n\n");
			
		
			write(client_sockfd,cmd_time,cmd_time[4]+6);
			debug_conn(stderr,"--> Time CMD: %02X, Send %d Bytes\n",cmd_login[2],cmd_login[4]+6);
			
			len = read(client_sockfd, buf, BUFSIZE);
			if ( len < 5 ) {
				printf("<-- %d Bytes:",len);
				for ( var_int = 0 ; var_int < len ; var_int++ ) {
					printf(" %02X",buf[var_int]);
				}
				printf("\n\n");
				
				return 1 ;
			}

			debug_conn(stderr,"<-- %d Bytes:",len);
			for ( var_int = 0 ; var_int < len ; var_int++ ) {
				debug_conn(stderr," %02X",buf[var_int]);
			}
			debug_conn(stderr,"\n\n");
			sleep(10);
		}
		
	}
	else {
		
		for ( cmd_cnt = 0 ; cmd_cnt < CMD_CNT ; cmd_cnt++ ) {
			write(client_sockfd,cmd_login,cmd_login[4]+6);
			debug_conn(stderr,"--> Login CMD: %02X, Send %d Bytes\n",cmd_login[2],cmd_login[4]+6);
			
			len = read(client_sockfd, buf, BUFSIZE);
			if ( len < 5 ) return 1 ;
			debug_conn(stderr,"<-- %d Bytes:",len);
			for ( var_int = 0 ; var_int < len ; var_int++ ) {
				debug_conn(stderr," %02X",buf[var_int]);
			}
			debug_conn(stderr,"\n\n");
			
		
			write(client_sockfd,cmd_time,cmd_time[4]+6);
			debug_conn(stderr,"--> Time CMD: %02X, Send %d Bytes\n",cmd_login[2],cmd_login[4]+6);
			
			len = read(client_sockfd, buf, BUFSIZE);
			if ( len < 5 ) return 1 ;
			debug_conn(stderr,"<-- %d Bytes:",len);
			for ( var_int = 0 ; var_int < len ; var_int++ ) {
				debug_conn(stderr," %02X",buf[var_int]);
			}
			debug_conn(stderr,"\n\n");
			sleep(10);
		}
		
	}

	//sleep(1);
	debug_conn(stderr,"Close socket and quit.\n");
	close(client_sockfd);//关闭套接字

	return 0;
}

void child_exit(int num)
{
	//Received SIGCHLD signal
	int status;

	int pid = waitpid(-1, &status, WNOHANG);
	if (WIFEXITED(status)) {
		debug_conn(stderr,"Child %d exit with code %d\n", pid, WEXITSTATUS(status));
		if ( WEXITSTATUS(status) ) {
			err_cnt++;
		}
	}
	
	if ( process_cnt > 0 ) process_cnt--;
}
