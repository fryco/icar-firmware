#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>
#include <netinet/in.h>
#include <limits.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <ctype.h>

#include <pwd.h>
#include <unistd.h>
#include <utmp.h>

#include "smtp.h"

#ifndef BUFSIZE
	#define BUFSIZE 1024
#endif

//#define DEBUG_SMTP

#ifdef DEBUG_SMTP
	#define debug_smtp(x, args...)  fprintf(x, ##args);
#else
	#define debug_smtp(x, args...)  ;
#endif

static const int FSHIFT = 16;           /* nr of bits of precision */
#define FIXED_1         (1<<FSHIFT)     /* 1.0 as fixed-point */
#define LOAD_INT(x) ((x) >> FSHIFT)
#define LOAD_FRAC(x) LOAD_INT(((x) & (FIXED_1-1)) * 100)

// Function prototype
void get_sysinfo( char *buf, int buf_len )
{
	/* 用于进制转换的常量。*/
	const double megabyte = 1024 * 1024;
	
	char tmp[BUFSIZE] ;
	int user_cnt = 0;
	struct utmp* entry;
 
	int updays, uphours, upminutes;
	struct sysinfo info;
	struct tm *current_time;
	time_t current_secs;

	time(&current_secs);
	current_time = localtime(&current_secs);

	sysinfo(&info);

	sprintf(buf, "%2d:%02d, up ", current_time->tm_hour, current_time->tm_min);
	
	updays = (int) info.uptime / (60*60*24);
	if (updays) {
		sprintf(tmp,"%d day%s, ", updays, (updays != 1) ? "s" : "");
		strcat(buf, tmp);
	}
	upminutes = (int) info.uptime / 60;
	uphours = (upminutes / 60) % 24;
	upminutes %= 60;
	if(uphours) {
		sprintf(tmp,"%2d:%02d, ", uphours, upminutes);
		strcat(buf, tmp);
	}
	else {
		sprintf(tmp,"%d min, ", upminutes);
		strcat(buf, tmp);
	}
	
	sprintf(tmp,"load avg: %ld.%02ld, %ld.%02ld, %ld.%02ld, ", 
			LOAD_INT(info.loads[0]), LOAD_FRAC(info.loads[0]), 
			LOAD_INT(info.loads[1]), LOAD_FRAC(info.loads[1]), 
			LOAD_INT(info.loads[2]), LOAD_FRAC(info.loads[2]));
	strcat(buf, tmp);
	
	sprintf (tmp,"free RAM: %5.1f MB, ", info.freeram / megabyte);
	strcat(buf, tmp);
	sprintf (tmp,"process cnt: %d\n", info.procs);
	strcat(buf, tmp);

	if ( strlen(buf) > (buf_len<<1) ) {//buf too long
		sprintf (buf,"Buf too long, check %s:%d\n", __FILE__,__LINE__);
	}

	setutent();
    while ((entry = getutent()) != NULL)
    {
        if ( (entry->ut_type == USER_PROCESS) && (entry->ut_name[0] != '\0') ) {
        	user_cnt++;
        }
    }
    sprintf(tmp,"%d user%s\n", user_cnt, (user_cnt > 1 ? "s" : ""));
	strcat(buf, tmp);
	endutent();
		
	setutent();
	user_cnt = 0;
    while ((entry = getutent()) != NULL)
    {

        if ( (entry->ut_type == USER_PROCESS) && (entry->ut_name[0] != '\0') ) {
        	user_cnt++;
	        sprintf(tmp,"%d: %s, from: %s, time: %s", user_cnt, entry->ut_user,
               entry->ut_host, ctime((const time_t *)&entry->ut_tv));
			strcat(buf, tmp);
        }
    }
	endutent();

	if ( strlen(buf) > (buf_len - 1) ) {//buf too long
		sprintf (buf,"Buf too long, check %s:%d\n", __FILE__,__LINE__);
	}
}

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
		fprintf(stderr,"Gethostname error, %s\n", strerror(errno));
		return 1 ;
	}

	/*建立SOCKET连接*/
	if((sockfd=socket(AF_INET,SOCK_STREAM,0))==-1)
	{
		fprintf(stderr,"Socket Error:%s\r\n",strerror(errno));
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
		fprintf(stderr,"Connect Error:%s\r\n",strerror(errno));
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
		fprintf(stderr,"Err rec! check %s:%d\n",__FILE__,__LINE__);
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
		fprintf(stderr,"Err rec! check %s:%d\n",__FILE__,__LINE__);
		close(sockfd); 
		return 4 ;
	}

	close(sockfd); 
	return 0 ;
}
