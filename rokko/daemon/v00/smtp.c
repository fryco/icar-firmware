#include "config.h"

#include "smtp.h"

//#define DEBUG_SMTP

#ifdef DEBUG_SMTP
	#define debug_smtp(x, args...)  printf(##args);
#else
	#define debug_smtp(x, args...)  ;
#endif

static const int FSHIFT = 16;           /* nr of bits of precision */
#define FIXED_1         (1<<FSHIFT)     /* 1.0 as fixed-point */
#define LOAD_INT(x) ((x) >> FSHIFT)
#define LOAD_FRAC(x) LOAD_INT(((x) & (FIXED_1-1)) * 100)

//return 0 : ok, others: err
unsigned char smtp_send(char *smtp_server, unsigned int smtp_port, char *mail_to, char *mail_subject, char *mail_body)
{
	char recv_buf[BUFSIZE];
	char send_buf[BUFSIZE];

	int sockfd;
	struct sockaddr_in server_addr;
	struct hostent *host;
	
	/*取得主机IP地址*/
	if((host=gethostbyname(smtp_server))==NULL)
	{
		printf("Gethostname error, %s\n", strerror(errno));
		return 1 ;
	}

	/*建立SOCKET连接*/
	if((sockfd=socket(AF_INET,SOCK_STREAM,0))==-1)
	{
		printf("Socket Error:%s\r\n",strerror(errno));
		return 2 ;
	}
	
	/* 客户程序填充服务端的资料 */
	bzero(&server_addr,sizeof(server_addr));
	server_addr.sin_family=AF_INET;
	server_addr.sin_port=htons(smtp_port);
	server_addr.sin_addr=*((struct in_addr *)host->h_addr);

	/* 客户程序发起连接请求 */
	if(connect(sockfd,(struct sockaddr *)(&server_addr),sizeof(struct sockaddr))==-1)/*连接网站*/
	{
		printf("Connect Error:%s\r\n",strerror(errno));
		return 3 ;
	}

	debug_smtp(stderr,"SMTP connect to %s, port: %d\r\n",smtp_server,smtp_port);
	

	// Wait for a reply
	//
	memset( recv_buf,0,BUFSIZE);
	recv(sockfd, recv_buf,sizeof( recv_buf),0);
	debug_smtp(stderr,"<-- %s\r\n", recv_buf);

	memset( send_buf, 0, BUFSIZE);
	strcat(send_buf, "EHLO localhost\r\n");
	send(sockfd, send_buf, strlen(send_buf), 0);
	debug_smtp(stderr,"--> %s",  send_buf);
	
	memset( recv_buf,0,BUFSIZE);
	recv(sockfd, recv_buf,sizeof( recv_buf),0);
	debug_smtp(stderr,"<-- %s\r\n",  recv_buf);
	
	//发送准备登陆信息
	memset( send_buf, 0, BUFSIZE);
	strcat(send_buf, "AUTH PLAIN ADEzODY5Njg5NDQwAHNtdHBAMTM5\r\n");
	send(sockfd, send_buf, strlen(send_buf), 0);
	debug_smtp(stderr,"--> %s",  send_buf);

	memset( recv_buf,0,BUFSIZE);
	recv(sockfd, recv_buf,sizeof( recv_buf),0);
	debug_smtp(stderr,"<-- %s\r\n",  recv_buf);

	//MAIL FROM:<13869689440@139.com>
	memset( send_buf, 0, BUFSIZE);
	strcat(send_buf, "MAIL FROM:<13869689440@139.com>\r\n");
	send(sockfd, send_buf, strlen(send_buf), 0);
	debug_smtp(stderr,"--> %s",  send_buf);

	memset( recv_buf,0,BUFSIZE);
	recv(sockfd, recv_buf,sizeof( recv_buf),0);
	debug_smtp(stderr,"<-- %s\r\n",  recv_buf);

	//RCPT TO:<13869689440@139.com>
	memset( send_buf, 0, BUFSIZE);
	strcat(send_buf, "RCPT TO:<");
	strcat(send_buf, mail_to);
	strcat(send_buf, ">\r\n");
	send(sockfd, send_buf, strlen(send_buf), 0);
	debug_smtp(stderr,"--> %s",  send_buf);

	memset( recv_buf,0,BUFSIZE);
	recv(sockfd, recv_buf,sizeof( recv_buf),0);
	debug_smtp(stderr,"<-- %s\r\n",  recv_buf);

	//DATA	
	memset( send_buf, 0, BUFSIZE);
	strcat(send_buf, "DATA\r\n");
	send(sockfd, send_buf, strlen(send_buf), 0);
	debug_smtp(stderr,"--> %s",  send_buf);

	memset( recv_buf,0,BUFSIZE);
	recv(sockfd, recv_buf,sizeof( recv_buf),0);
	debug_smtp(stderr,"<-- %s\r\n",  recv_buf);
	
	//Mail subject	
	memset( send_buf, 0, BUFSIZE);
	strcat(send_buf, "subject:");
	strcat(send_buf, mail_subject);
	strcat(send_buf, "\r\n");
	send(sockfd, send_buf, strlen(send_buf), 0);
	debug_smtp(stderr,"--> %s",  send_buf);

	//Mail body
	memset( send_buf, 0, BUFSIZE);
	strcat(send_buf, mail_body);
	send(sockfd, send_buf, strlen(send_buf), 0);
	debug_smtp(stderr,"--> %s",  send_buf);

	get_sysinfo( send_buf, BUFSIZE) ;
	send(sockfd, send_buf, strlen(send_buf), 0);
	send(sockfd, "\r\n", strlen("\r\n"), 0);
	debug_smtp(stderr,"--> %s",  send_buf);
		
	//Mail end
	memset( send_buf, 0, BUFSIZE);
	strcat(send_buf, ".\r\n");
	send(sockfd, send_buf, strlen(send_buf), 0);
	debug_smtp(stderr,"--> %s",  send_buf);

	memset( recv_buf,0,BUFSIZE);
	recv(sockfd, recv_buf,sizeof( recv_buf),0);
	debug_smtp(stderr,"<-- %s\r\n",  recv_buf);

	if ( strstr(recv_buf,"250") == NULL ) { //no found
		printf("Err rec! check %s:%d\n",__FILE__,__LINE__);
		close(sockfd); 
		return 3 ;
	}
	
	//QUIT
	memset( send_buf, 0, BUFSIZE);
	strcat(send_buf, "QUIT\r\n");
	send(sockfd, send_buf, strlen(send_buf), 0);
	debug_smtp(stderr,"--> %s",  send_buf);

	memset( recv_buf,0,BUFSIZE);
	recv(sockfd, recv_buf,sizeof( recv_buf),0);
	debug_smtp(stderr,"<-- %s\r\n",  recv_buf);

	if ( strstr(recv_buf,"221") == NULL ) { //no found
		printf("Err rec! check %s:%d\n",__FILE__,__LINE__);
		close(sockfd); 
		return 4 ;
	}

	close(sockfd); 
	return 0 ;
}
