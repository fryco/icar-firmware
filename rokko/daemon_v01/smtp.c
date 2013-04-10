#include "config.h"

#include "smtp.h"

//#define DEBUG_SMTP

#ifdef DEBUG_SMTP
	#define debug_smtp(args...)  fprintf(stderr, ##args);
#else
	#define debug_smtp(x, args...)  ;
#endif

static const int FSHIFT = 16;           /* nr of bits of precision */
#define FIXED_1         (1<<FSHIFT)     /* 1.0 as fixed-point */
#define LOAD_INT(x) ((x) >> FSHIFT)
#define LOAD_FRAC(x) LOAD_INT(((x) & (FIXED_1-1)) * 100)

//return 0 : ok, others: err
unsigned char smtp_send(char *smtp_server, 
						unsigned int smtp_port, char *mail_to, 
						char *mail_subject, char *mail_body, char *err_str)
{
	char recv_buf[BUFSIZE*20], send_buf[BUFSIZE*20];
	char retry = 0;

	ssize_t size = 0;
	int sockfd;
	struct sockaddr_in server_addr;
	struct hostent *host;
	
	/*取得主机IP地址*/
	if((host=gethostbyname(smtp_server))==NULL)
	{
		printf("Gethostname error, %s\n", strerror(errno));
		return 10 ;
	}

	/*建立SOCKET连接*/
	if((sockfd=socket(AF_INET,SOCK_STREAM,0))==-1)
	{
		printf("Socket Error:%s\r\n",strerror(errno));
		return 20 ;
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
		return 30 ;
	}

	debug_smtp("SMTP connect to %s, port: %d\r\n",smtp_server,smtp_port);
	

	// Wait for a reply
	//
	bzero( recv_buf, sizeof(recv_buf));
	size = read( sockfd, recv_buf, sizeof( recv_buf));
	debug_smtp("<-- %s\r\n", recv_buf);

	bzero( send_buf, sizeof(send_buf));
	strcat(send_buf, "EHLO localhost\r\n");
	write( sockfd, send_buf, strlen(send_buf));
	debug_smtp("--> %s",  send_buf);
	
	bzero( recv_buf, sizeof(recv_buf));
	size = read( sockfd, recv_buf, sizeof( recv_buf));
	debug_smtp("<-- %s\r\n",  recv_buf);
	
	//发送准备登陆信息
	bzero( send_buf, sizeof(send_buf));
	//strcat(send_buf, "AUTH PLAIN ADEzODY5Njg5NDQwAHNtdHBAMTM5\r\n");
	strcat(send_buf, "AUTH PLAIN ADEzODI4NDMxMTA2AG1vdG8zOTg=\r\n");
	write( sockfd, send_buf, strlen(send_buf));
	debug_smtp("--> %s",  send_buf);

	retry = 10 ;
	bzero( recv_buf, sizeof(recv_buf));
	size = read( sockfd, recv_buf, sizeof( recv_buf));

	while ( retry & size < 15 ) {
		bzero( recv_buf, sizeof(recv_buf));
		size = read( sockfd, recv_buf, sizeof( recv_buf));
		if (size == 0 || size > sizeof( recv_buf)) { //no data or overflow
			write( sockfd, "QUIT\r\n", strlen("QUIT\r\n"));
			close(sockfd);
			
			printf("Rec CNT err! check %s:%d\n",__FILE__,__LINE__);
			return 35 ;
		}
		retry--;
	}

	debug_smtp("<-- %s\r\n",  recv_buf);
	if ( strstr(recv_buf,"failed") ) { //454 Authentication failed
		write( sockfd, "QUIT\r\n", strlen("QUIT\r\n"));
		close(sockfd);

		printf("Authentication failed! check %s:%d\n",__FILE__,__LINE__);
		bzero( err_str, BUFSIZE);
		strcat(err_str, recv_buf);
		//strcat(err_str, "Err code: 40, %s:%d\r\n",__FILE__,__LINE__);
		strcat(err_str, "Err code: 40\r\n");
		return 40 ;
	}

	//MAIL FROM:<13869689440@139.com>
	bzero( send_buf, sizeof(send_buf));
	//strcat(send_buf, "MAIL FROM:<13869689440@139.com>\r\n");
	strcat(send_buf, "MAIL FROM:<13828431106@139.com>\r\n");
	write( sockfd, send_buf, strlen(send_buf));
	printf("--> %s",  send_buf);

	bzero( recv_buf, sizeof(recv_buf));
	size = read( sockfd, recv_buf, sizeof( recv_buf));
	debug_smtp("<-- %s\r\n",  recv_buf);

	if ( strstr(recv_buf,"250") == NULL ) { //shoud return: 250 ok
		write( sockfd, "QUIT\r\n", strlen("QUIT\r\n"));
		close(sockfd); 

		printf("No 250! check %s:%d\n",__FILE__,__LINE__);
		bzero( err_str, BUFSIZE);
		strcat(err_str, recv_buf);
		strcat(err_str, "\r\nErr code: 45\r\n");
		return 45 ;
	}

	//RCPT TO:<13869689440@139.com>
	bzero( send_buf, sizeof(send_buf));
	strcat(send_buf, "RCPT TO:<");
	strcat(send_buf, mail_to);
	strcat(send_buf, ">\r\n");
	write( sockfd, send_buf, strlen(send_buf));
	printf("--> %s",  send_buf);

	bzero( recv_buf, sizeof(recv_buf));
	size = read( sockfd, recv_buf, sizeof( recv_buf));
	debug_smtp("<-- %s\r\n",  recv_buf);

	if ( strstr(recv_buf,"Invalid rcpt") ) { //No receiver address
		write( sockfd, "QUIT\r\n", strlen("QUIT\r\n"));
		close(sockfd);

		printf("Invalid mail address! check %s:%d\n",__FILE__,__LINE__);
		bzero( err_str, BUFSIZE);
		strcat(err_str, recv_buf);
		strcat(err_str, "\r\nErr code: 50\r\n");
		return 50 ;
	}

	if ( strstr(recv_buf,"250") == NULL ) { //shoud return: 250 ok
		write( sockfd, "QUIT\r\n", strlen("QUIT\r\n"));
		close(sockfd); 

		printf("No 250! check %s:%d\n",__FILE__,__LINE__);
		bzero( err_str, BUFSIZE);
		strcat(err_str, recv_buf);
		strcat(err_str, "\r\nErr code: 55\r\n");
		return 55 ;
	}

	//DATA	
	bzero( send_buf, sizeof(send_buf));
	strcat(send_buf, "DATA\r\n");
	write( sockfd, send_buf, strlen(send_buf));
	printf("--> %s",  send_buf);

	bzero( recv_buf, sizeof(recv_buf));
	size = read( sockfd, recv_buf, sizeof( recv_buf));
	debug_smtp("<-- %s\r\n",  recv_buf);

	//Mail to
	bzero( send_buf, sizeof(send_buf));
	strcat(send_buf, "to:");
	strcat(send_buf, mail_to);
	strcat(send_buf, "\r\n");
	write( sockfd, send_buf, strlen(send_buf));
	debug_smtp("--> %s",  send_buf);

	//Mail from
	bzero( send_buf, sizeof(send_buf));
	strcat(send_buf, "from:");
	strcat(send_buf, "Cloud server");
	strcat(send_buf, "\r\n");
	write( sockfd, send_buf, strlen(send_buf));
	debug_smtp("--> %s",  send_buf);

	//Mail subject	
	bzero( send_buf, sizeof(send_buf));
	strcat(send_buf, "subject:");
	strcat(send_buf, mail_subject);
	strcat(send_buf, "\r\n");
	write( sockfd, send_buf, strlen(send_buf));
	debug_smtp("--> %s",  send_buf);

	//Mail body
	bzero( send_buf, sizeof(send_buf));
	strcat(send_buf, mail_body);
	write( sockfd, send_buf, strlen(send_buf));
	debug_smtp("--> %s",  send_buf);

	
	//get_sysinfo( send_buf, BUFSIZE) ;
	//write( sockfd, send_buf, strlen(send_buf));
	write( sockfd, "\r\n", strlen("\r\n"));
	debug_smtp("--> %s",  send_buf);
	
	//Mail end
	bzero( send_buf, sizeof(send_buf));
	strcat(send_buf, ".\r\n");
	write( sockfd, send_buf, strlen(send_buf));
	debug_smtp("--> %s",  send_buf);

	bzero( recv_buf, sizeof(recv_buf));
	size = read( sockfd, recv_buf, sizeof( recv_buf));
	debug_smtp("<-- %s\r\n",  recv_buf);

	if ( strstr(recv_buf,"250") == NULL ) { //no found
		printf("Err rec! check %s:%d\n",__FILE__,__LINE__);
		bzero( err_str, BUFSIZE);
		strcat(err_str, recv_buf);
		strcat(err_str, "\r\nErr code: 60\r\n");

		write( sockfd, "QUIT\r\n", strlen("QUIT\r\n"));
		close(sockfd); 
		return 60 ;
	}
	
	//QUIT
	write( sockfd, "QUIT\r\n", strlen("QUIT\r\n"));
	debug_smtp("--> %s",  "QUIT\r\n");

	bzero( recv_buf, sizeof(recv_buf));
	size = read( sockfd, recv_buf, sizeof( recv_buf));
	debug_smtp("<-- %s\r\n",  recv_buf);

	if ( strstr(recv_buf,"221") == NULL ) { //no found
		printf("Err rec! check %s:%d\n",__FILE__,__LINE__);
		bzero( err_str, BUFSIZE);
		strcat(err_str, recv_buf);
		strcat(err_str, "\r\nErr code: 70\r\n");

		write( sockfd, "QUIT\r\n", strlen("QUIT\r\n"));
		close(sockfd);
		return 70 ;
	}

	close(sockfd); 
	return 0 ;
}
