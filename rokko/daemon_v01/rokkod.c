/**
 *      rokkod - Copyright (c) 2011-2012 cn0086 <cn0086.info@gmail.com>
 *
 *      This is NOT a freeware, use is subject to license terms
 *
 *      $URL$
 *      $Rev$, $Date$
 */
//http://myswirl.blog.163.com/blog/static/513186422010102495152843/
//http://bbs.csdn.net/topics/390165941
//Rev: 59 ==> 单线程实现多连接
//Rev: 358 SN, 10-->8
//http://blog.csdn.net/guowake/article/details/6615728
//http://blog.csdn.net/ljx0305/article/details/4065058
//http://www.cppblog.com/ifeng/archive/2011/09/29/157141.html

//zc zC zo zO zj zk set nonumber

#include "config.h"
#include "rokkod.h"

#define rokkod_RELEASE "\nRokko daemon $Rev$, "__DATE__" "__TIME__"\n"

//for default parameters
static unsigned int mail_timer, db_timer;
unsigned int  listen_port=23, daemon_time;//record daemon run time
unsigned char foreground=0;
struct server_struct rokko_srv;

static int sock_server = -1;
static struct sockaddr_in server_addr;
	
static char pidfile[EMAIL], host_name[EMAIL];

int main(int argc, char *argv[])
{
	void handler(int);
	void child_exit(int);
	unsigned int i=0, maxsock, conn_amount = 0;//current active connection

	int read_size, new_fd;
	fd_set fdsr; //文件描述符集的定义
	struct rokko_data rokko[MAXCLIENT] ;
	
	struct timeval tv, run_tv;
	time_t ticks=time(NULL), loop_timer;
	char msg[BUFSIZE];
		
	struct sockaddr_in client_addr;
/*
	unsigned char crc_src[] = { 0xDE, 0x80, 0xD2, 0x00, 0x01, 0x00 };
	//for verify CRC16: DE 80 D2 00 01 00 F6 39
	fprintf(stderr, "CRC result is: %X\r\n",crc16tablefast(crc_src,6));
*/

	fprintf(stderr, "rokko size: %d\n", sizeof(rokko_srv));
	bzero( rokko, sizeof(rokko)); //bzero( rokko_srv, sizeof(rokko_srv));
	bzero( pidfile, sizeof(pidfile)); bzero( msg, sizeof(msg));
	
	daemon_time = time(NULL);//for report daemon running time
	
	/* Scan arguments. */
	scan_args(argc, argv);
 	
 	if(!pidfile[0])
 	{
		snprintf(pidfile,EMAIL,"/var/run/rokkod_%d",listen_port);
 		pidfile[EMAIL] = '\0';
 	}
 
 	{
 		FILE *pidf;
 		pid_t pid = 0;
 
 		pidf = fopen(pidfile, "r");
 		if(pidf) {
 			/* The pidfile exists. Now see if the process is still running */
 			if(!fscanf(pidf, "%d", &pid))
 				pid = 0;
 			fclose(pidf);
 			if(pid && !kill(pid, 0)) {
 				fprintf(stderr, "rokkod is already running, PID: %d.\n",pid);
 				exit(1);
 			}
 		}
 	}		

 	if ( log_init( LOG_DIR ) ) { //error
		fprintf(stderr, "Create log dir error! %s:%d\n",__FILE__,__LINE__);
		exit(1);
	}

 	/* Add signal handler for hangups/terminators. */
 	signal(SIGTERM,handler); //exit when run kill...

	signal(SIGINT, handler); //exit when Crtl-C

	signal(SIGCHLD, SIG_IGN); /* 忽略子进程结束信号，防止出现僵尸进程 */ 
	//signal(SIGCHLD, child_exit);
	
	fprintf(stderr, "Listen port: %d\n\n",listen_port);
	
	/* Now run in the background. */
	if (!foreground) {
		fprintf(stderr,"Daemon running in background ...\n\n");
		bg();
	}

	//save pid to system
	FILE *pidf;

	pidf = fopen(pidfile, "w");
	if(pidf) {
		fprintf(pidf, "%d", getpid());
		fclose(pidf);
	}
	else {
		pidfile[0] = '\0';
	}

	/* Main program, create log file first */
	snprintf(msg,sizeof(msg),"==> Start rokko daemon, PID: %d, port: %d\n",getpid(),listen_port);
	log_save(msg, FORCE_SAVE_FILE );
	
	if ( sock_init( listen_port ) ) {//err
		fprintf(stderr, "Sock init err!\r\n");
		snprintf(msg,sizeof(msg),"Sock init err, port: %d, exit.\n",listen_port);
		log_save(msg, FORCE_SAVE_FILE );
		exit(1);
	}

	bzero( host_name, sizeof(host_name));
	gethostname(host_name, sizeof(host_name));
	snprintf(&host_name[strlen(host_name)],16,"_%d",listen_port);

	/* Main loop, accept TCP socket connect */
	maxsock = sock_server;
	//unsigned int idle_timer = time(NULL) ;

	//initial database, connect mysql
	rokko_srv.db.host = DB_HOST ;
	rokko_srv.db.name = DB_NAME ;
	rokko_srv.db.user = DB_USER ;
	rokko_srv.db.pwd =  DB_PWD ;
	
	if ( db_check(&rokko_srv.db) ) {//no ready
		fprintf(stderr,  "Database no ready, exit.\n");
		fprintf(stderr,  "Check: 1, Have installed mysql?\n");
		fprintf(stderr,  "       2, host, user, password, database are correct?\n");
		exit(1);
	}

	loop_timer = time(NULL) ;
	
	while (1)
	{
/*		//Monitor the loop time, if too long, need check
		if ((time(NULL)) - loop_timer > 1) {
			bzero( msg, sizeof(msg));
			snprintf(msg,sizeof(msg),"loop_time: %u too long@ %u\n",(time(NULL)) - loop_timer,__LINE__);
			log_save(msg, FORCE_SAVE_FILE );
			if ( foreground ) fprintf(stderr,"%s",msg);			
		}
*/		
		loop_timer = time(NULL) ;
		
		if ( conn_amount > MAXCLIENT ) { //err
			bzero( msg, sizeof(msg));
			snprintf(msg,sizeof(msg),"ERR! conn_amount= %u, but Max. is %u\n%s:%d\n",\
					conn_amount, MAXCLIENT,__FILE__,__LINE__);
			log_save(msg, FORCE_SAVE_FILE );
			if ( foreground ) fprintf(stderr,"%s",msg);			
		}

		//gettimeofday(&run_tv,NULL);
		//fprintf(stderr,"\n%02d.%d -> %d\n",(int)(run_tv.tv_sec)&0xFF,(int)run_tv.tv_usec,__LINE__);
		
		//period check
		//if ((time(NULL)) - db_timer > PERIOD_CHECK_DB-1) {
		if ((time(NULL)) - db_timer > 3) {
			fprintf(stderr,"Con: %u\n",conn_amount);
			period_check( rokko, conn_amount);
			db_timer = time(NULL);
		}
		
		FD_ZERO(&fdsr); //每次进入循环都重建描述符集
		FD_SET(sock_server, &fdsr);

		//for debug only
		//snprintf(msg,sizeof(msg),"Run: %d\n",__LINE__);
		//log_save(msg, FORCE_SAVE_FILE );

		for (i = 0; i < MAXCLIENT; i++) { //将存在的套接字加入描述符集
			if (rokko[i].client_socket != 0) {//exist connection

				FD_SET(rokko[i].client_socket, &fdsr);

				if (((time(NULL)) - rokko[i].idle_timer > CMD_TIMEOUT*6 )) {//5*6=30 seconds
					if ( foreground ) {
						fprintf(stderr, "R[%d] %s idle too long, close.\n",\
							i,(char *)inet_ntoa(rokko[i].client_addr.sin_addr));
					}


					//post to cloud
					if (fork() == 0) { //In child process
						unsigned char post_buf[BUFSIZE], host_info[BUFSIZE], logtime[EMAIL];
						struct timeval log_tv;
		
						bzero( post_buf, sizeof(post_buf));bzero( host_info, sizeof(host_info));
						get_sysinfo( host_info, BUFSIZE) ;

						bzero(logtime, sizeof(logtime));
						gettimeofday(&log_tv,NULL);
						strftime(logtime,EMAIL,"%Y-%m-%d %T",(const void *)localtime(&log_tv.tv_sec));
	
						sprintf(post_buf,"ip=%s&fid=43&subject=%s, %s:%d @ [%d] idle too long, force disconnect!&message=%s\r\n%s\r\nTotal iCar: %u",\
								"127.0.0.1", host_name,\
								(char *)inet_ntoa(rokko[i].client_addr.sin_addr),\
								ntohs(rokko[i].client_addr.sin_port), i,\
								logtime, host_info, conn_amount-1);
				
						cloud_post( CLOUD_HOST, &post_buf, 80 );
						exit( 0 );
					}

					write(rokko[i].client_socket,"Idle too long, bye.\n",25);
					//save transmit count and disconnect reason, TBD
						
					close(rokko[i].client_socket);
					FD_CLR(rokko[i].client_socket, &fdsr);
						
					bzero( &rokko[i], sizeof(rokko[i]));
					if ( conn_amount ) conn_amount--;
				}
			}
		}
		//idle_timer = time(NULL) ;

		tv.tv_sec = 60;
		tv.tv_usec = 0;
		read_size = select(maxsock + 1, &fdsr, NULL, NULL,&tv); 

		if (read_size < 0) { //failure
			snprintf(msg,sizeof(msg),"Exit: %s. %s:%d\n",strerror(errno),__FILE__,__LINE__);
			log_save(msg, FORCE_SAVE_FILE );
			if ( foreground ) fprintf(stderr,"%s",msg);
		    exit(1);
		}
		else {
			if (read_size == 0) { //=0 timeout
				//fprintf(stderr, "Timeout select\n");
				continue;
			}
			//else read_size > 0 is normal return
		}

		//gettimeofday(&run_tv,NULL);
		//fprintf(stderr,"%02d.%d -> %d\n",(int)(run_tv.tv_sec)&0xFF,(int)run_tv.tv_usec,__LINE__);

		// 如果select发现有异常，循环判断各活跃连接是否有数据到来
		for (i = 0; i < MAXCLIENT; i++) {//this loop will consume lots of time
			if (FD_ISSET(rokko[i].client_socket, &fdsr)) {
				bzero( msg, sizeof(msg));
				read_size = read(rokko[i].client_socket, msg, BUFSIZE );
				if (read_size <= 0 || read_size > BUFSIZE) {//Client close or err

					if ( foreground ) {
						fprintf(stderr,"R[%d], %s, %s close@ %d.\n", i,\
							(char *)inet_ntoa(rokko[i].client_addr.sin_addr),\
							rokko[i].sn_long,__LINE__);
					}

					//post to cloud
					if (fork() == 0) { //In child process
						unsigned char post_buf[BUFSIZE], host_info[BUFSIZE], logtime[EMAIL];
						struct timeval log_tv;
			
						bzero( post_buf, sizeof(post_buf));bzero( host_info, sizeof(host_info));
						get_sysinfo( host_info, BUFSIZE) ;

						bzero(logtime, sizeof(logtime));
						gettimeofday(&log_tv,NULL);
						strftime(logtime,EMAIL,"%Y-%m-%d %T",(const void *)localtime(&log_tv.tv_sec));
						
						sprintf(post_buf,"ip=%s&fid=43&subject=%s, %s:%d @ [%d] disconnect! %s&message=%s\r\nTotal iCar: %d",\
								"127.0.0.1", host_name,\
								(char *)inet_ntoa(rokko[i].client_addr.sin_addr),\
								ntohs(rokko[i].client_addr.sin_port), i,\
								logtime, host_info, conn_amount-1);
				
						cloud_post( CLOUD_HOST, &post_buf, 80 );
						exit( 0 );
					}

					//save transmit count and disconnect reason, TBD
					close(rokko[i].client_socket);
					FD_CLR(rokko[i].client_socket, &fdsr);
					
					bzero( &rokko[i], sizeof(rokko[i]));
					if ( conn_amount ) conn_amount--;
				}
				else { //client sent data to server
					if ( daemon_server(&rokko[i],msg,read_size, rokko, conn_amount) \
							|| rokko[i].cmd_err_cnt > 3 ) { //failure, close

						if ( foreground ) {
							fprintf(stderr,"Rokko[%d], %s, %s close@ %d.\n", i,\
								(char *)inet_ntoa(rokko[i].client_addr.sin_addr),\
								rokko[i].sn_long,__LINE__);
						}

						//post to cloud
						if (fork() == 0) { //In child process
							unsigned char post_buf[BUFSIZE], host_info[BUFSIZE], logtime[EMAIL];
							struct timeval log_tv;
				
							bzero( post_buf, sizeof(post_buf));bzero( host_info, sizeof(host_info));
							get_sysinfo( host_info, BUFSIZE) ;
	
							bzero(logtime, sizeof(logtime));
							gettimeofday(&log_tv,NULL);
							strftime(logtime,EMAIL,"%Y-%m-%d %T",(const void *)localtime(&log_tv.tv_sec));
							
							sprintf(post_buf,"ip=%s&fid=43&subject=%s, %s:%d @ [%d] login failure, force disconnect! %s&message=%s\r\nTotal iCar: %d",\
									"127.0.0.1", host_name,\
									(char *)inet_ntoa(rokko[i].client_addr.sin_addr),\
									ntohs(rokko[i].client_addr.sin_port), i,\
									logtime,host_info, conn_amount-1);
					
							cloud_post( CLOUD_HOST, &post_buf, 80 );
							exit( 0 );
						}

						//save transmit count and disconnect reason, TBD
						close(rokko[i].client_socket);
						FD_CLR(rokko[i].client_socket, &fdsr);
						
						bzero( &rokko[i], sizeof(rokko[i]));
						if ( conn_amount ) conn_amount--;
					}
					else {
						rokko[i].idle_timer = time(NULL); //reset idle timer
						/*
						if ( foreground ) {
							fprintf(stderr,"R[%d] %s RST Act @ %d.\n", i,\
								(char *)inet_ntoa(rokko[i].client_addr.sin_addr),__LINE__);
						}*/
					}
				}
			}
		}
		//gettimeofday(&run_tv,NULL);
		//fprintf(stderr,"%02d.%d -> %d\n",(int)(run_tv.tv_sec)&0xFF,(int)run_tv.tv_usec,__LINE__);

		//New connection require
		int addrlen = sizeof(struct sockaddr);
		if (FD_ISSET(sock_server, &fdsr)) {

			new_fd = accept(sock_server, (struct sockaddr *)&client_addr, &addrlen);

			if (new_fd <= 0) {
				snprintf(msg,sizeof(msg),"Accept new connection err:%s. %s:%d\n",\
							strerror(errno),__FILE__,__LINE__);
				log_save(msg, FORCE_SAVE_FILE );
				if ( foreground ) fprintf(stderr,"%s",msg);
				continue; //error, stop this connection
			}
		
			// if < MAXCLIENT, save to client_socket
			if (conn_amount < MAXCLIENT) {
				for(i = 0;i < MAXCLIENT;i++) {
					if(rokko[i].client_socket == 0) {
						bzero( &rokko[i], sizeof(rokko[i]));
						rokko[i].client_socket = new_fd;
						memcpy(&rokko[i].client_addr,&client_addr,sizeof(client_addr));
						rokko[i].idle_timer = time(NULL);
						/*
						if ( foreground ) {
							fprintf(stderr,"R[%d] %s RST Act @ %d.\n", i,\
								(char *)inet_ntoa(rokko[i].client_addr.sin_addr),__LINE__);
						}
						*/
						rokko[i].con_time = time(NULL);
						break;
					}
				}

				//update connection amounts
				conn_amount++;
/*
				if ( foreground ) {
					fprintf(stderr, "New Rokko[%d] %s:%d\n", i,\
						(char *)inet_ntoa(rokko[i].client_addr.sin_addr), \
						ntohs(rokko[i].client_addr.sin_port));
				}
*/
				//post to cloud
				if (fork() == 0) { //In child process
					unsigned char post_buf[BUFSIZE], host_info[BUFSIZE], logtime[EMAIL];
					struct timeval log_tv;
				
					bzero( post_buf, sizeof(post_buf));bzero( host_info, sizeof(host_info));
					get_sysinfo( host_info, BUFSIZE) ;
	
					bzero(logtime, sizeof(logtime));
					gettimeofday(&log_tv,NULL);
					strftime(logtime,EMAIL,"%Y-%m-%d %T",(const void *)localtime(&log_tv.tv_sec));

					sprintf(post_buf,"ip=%s&fid=43&subject=%s, %s:%d @ [%d] connect, %s&message=%s\r\nTotal iCar: %u",\
							"127.0.0.1", host_name,\
							(char *)inet_ntoa(rokko[i].client_addr.sin_addr),\
							ntohs(rokko[i].client_addr.sin_port), i,\
							logtime, host_info, conn_amount);
			
					cloud_post( CLOUD_HOST, &post_buf, 80 );
					exit( 0 );
				}
					
				if (new_fd > maxsock) {
					maxsock = new_fd;
					snprintf(msg,sizeof(msg),"Maxsock: %d\n",maxsock);
					log_save(msg, 0 );
					if ( foreground ) fprintf(stderr,"%s",msg);
				}
			}
			else {
				snprintf(msg,sizeof(msg),"Over Max client! Close %s\n",\
							(char *)inet_ntoa(client_addr.sin_addr));
				log_save(msg, 0);

				if ( foreground ) fprintf(stderr, "%s",msg);
				
				write(new_fd,"Over Max client, bye.\n",25);
				//save transmit count TBD
				
				close(new_fd);
				continue;
			}
		}
	}
}

void bg(void)
{
	int i;
	/* Simple fork to run proces in the background. */
	switch(fork())
	{
		case 0:
			break;
		case -1:
			perror("fork failed"); exit(1);
		default:
			exit(0);
	}

	if (-1==setsid()) {
		perror("setsid failed"); exit(1);
	}

	/* Close probably all file descriptors */
	for (i = 0; i<NOFILE; i++) close(i);

	i=open("/dev/null",O_RDWR); dup(i); dup(i); /* handle standart I/O */

	/* Be nice to umount */
	chdir("/");
	
	umask(077); /* 重设文件创建掩码 */ 
}

void scan_args(int argc, char *argv[])
{
	int index;

	while((index = getopt(argc, argv, "hvbfp:")) != EOF)
	{
		switch(index)
		{
			case '?':
			case 'h':
				print_help(argv);
				break;
			case 'v':
				print_version();
				break;
			case 'f':
				foreground = 1;
				break;

			case 'p':
				listen_port = a2port(optarg);
				if (listen_port <= 0) {
					fprintf(stderr, "Bad port number, the port must: 1 ~ 65535\n");
					exit(1);
				}
				break;

		}
	}
	
	fprintf(stderr, "%s\n", rokkod_RELEASE );
}

void handler(int s)
{
	
	//fprintf(stderr, "PID:%d %d\n",getpid(),__LINE__);
	/* Exit gracefully. */
	if(pidfile[0]) {
		unlink(pidfile);
	}
	
	//Close mysql
	mysql_close(&rokko_srv.db.mysql);
	
	log_save("==> Exit.\n", FORCE_SAVE_FILE );
	
	exit(0);
}

void child_exit(int num)
{
	//Received SIGCHLD signal
	wait3(NULL,WNOHANG,NULL);
/*
	int status;
	FILE *fp;

	int pid = waitpid(-1, &status, WNOHANG);
	
	wait3(NULL,WNOHANG,NULL);
	
	if (WIFEXITED(status)) {
		fp = fopen("/tmp/log/log_err.txt", "a");
		if ( fp ) { 
			fprintf(fp, "Child %d exit with code %d\n", pid, WEXITSTATUS(status));
			fclose(fp) ;
		}
	}
*/
}

void print_help(char *argv[])
{
	fprintf(stderr, "%s\n", rokkod_RELEASE );
	fprintf(stderr, "usage: %s [OPTION]...\n", argv[0]);
	fprintf(stderr, "commandline options override settings from configuration file\n\n");
	fprintf(stderr, "  -?             this help\n");
	fprintf(stderr, "  -f             run in foreground [debug]\n");
	fprintf(stderr, "  -p             port, default is 23\n\n");
	exit(0);
}

void print_version(void)
{
	fprintf(stderr, "%s\n", rokkod_RELEASE );
	fprintf(stderr, "enhanced and maintained by cn0086 <cn0086.info@gmail.com>\n\n");
	exit(0);
}


// 定时检查服务器并发邮件
void period_check( struct rokko_data *rokko, unsigned int conn_amount )
{
	char mail_subject[BUFSIZE], mail_body[BUFSIZE*10], client_buf[EMAIL], err;
	unsigned int updays, uphours, upminutes, seconds, up_time;
	unsigned int var_u32, con_cnt = 0;
	unsigned char save_record = 0 ;

	//Check system
	bzero( mail_body, sizeof(mail_body));
	get_sysinfo( mail_body, sizeof(mail_body)) ;

	//fprintf(stderr,"%d -> ",__LINE__);
	
	if ( conn_amount ) {
		strcat(mail_body, "iCar detail:\r\n");
		for (var_u32 = 0; var_u32 < MAXCLIENT; var_u32++) {
			//if ((rokko[var_u32].client_socket != 0) && (strlen(rokko[var_u32].sn_long) == 10)) {//exist connection
			if ((rokko[var_u32].client_socket != 0) && rokko[var_u32].login_cnt ) {//exist connection and login
				con_cnt++;
				
				if ( conn_amount > 20 ) {//only list 20, prevent buffer overflow
					if ( con_cnt < 10 || (conn_amount - con_cnt) < 10 ) {//show first/last 10 unit
						save_record = 1 ;
					}
					else { 
						save_record = 0 ;
					}
				}
				else {
					save_record = 1 ;
				}

				if ( save_record ) {
					bzero( client_buf, sizeof(client_buf));			
		
					//rokko[var_u32].rx_cnt = client tx cnt
					//rokko[var_u32].tx_cnt = client rx cnt
					if ( rokko[var_u32].rx_cnt < 9999 ) {
						snprintf(client_buf,EMAIL,"%02d, R[%02d] %s\tTX:%d B\tRX:%d B\tup",\
							con_cnt,var_u32,rokko[var_u32].sn_long,rokko[var_u32].rx_cnt,rokko[var_u32].tx_cnt);
					}
					else {
						snprintf(client_buf,EMAIL,"%02d, R[%02d] %s\tTX:%d.%03d KB\tRX:%d.%03d KB\tup",\
							con_cnt,var_u32,rokko[var_u32].sn_long,\
							rokko[var_u32].rx_cnt>>10,rokko[var_u32].rx_cnt%1024,\
							rokko[var_u32].tx_cnt>>10,rokko[var_u32].tx_cnt%1024);
					}
					//fprintf(stderr, "rokko[%d] = %X\r\n",var_u32,rokko[var_u32].client_socket);
					strcat(mail_body, client_buf);
					
					//format up time
					up_time = (time(NULL) - rokko[var_u32].con_time);
					updays = up_time / (60*60*24);
					if (updays) {
						sprintf(client_buf,"%d day%s, ", updays, (updays != 1) ? "s" : "");
						strcat(mail_body, client_buf);
					}
					upminutes = up_time / 60;
					uphours = (upminutes / 60) % 24;
					upminutes %= 60;
					seconds = up_time - (updays*60*60*24) - uphours*60*60 - upminutes*60;
					
					sprintf(client_buf,"%2d:%02d:%02d\t", uphours, upminutes,seconds);
					strcat(mail_body, client_buf);
					
					sprintf(client_buf,"Idle: %ld\n", time(NULL)-(rokko[var_u32].idle_timer));
					strcat(mail_body, client_buf);
				}
				else {
					strcat(mail_body, ".");
				}
			}
		}
		strcat(mail_body, "\r\n");
	}

	//fprintf(stderr,"%d\n",__LINE__);
	
	log_save( mail_body ) ;
	
	bzero( mail_subject, sizeof(mail_subject));
	if ( conn_amount > MAXCLIENT ) { //err
		snprintf(mail_subject,BUFSIZE,"%s con err: conn_amount= %u, but Max. is %d\r\n",host_name,conn_amount, MAXCLIENT);
	}
	else {
		snprintf(mail_subject,BUFSIZE,"%s Total connection: %u\r\n",host_name,conn_amount);
	}

	log_save( mail_subject ) ; //mail_subject will be sent later, don't modify it.
		
	//post to cloud
	if (fork() == 0) { //In child process
		unsigned char post_buf[BUFSIZE*20], logtime[EMAIL];
		struct timeval log_tv;

		bzero( post_buf, sizeof(post_buf));	bzero(logtime, sizeof(logtime));
		gettimeofday(&log_tv,NULL);
		strftime(logtime,EMAIL,"%Y-%m-%d %T",(const void *)localtime(&log_tv.tv_sec));
		
		sprintf(post_buf,"ip=%s&fid=43&subject=%s, Total: %u, %s&message=%s\r\n%s",\
				"127.0.0.1",\
				host_name,conn_amount,logtime,\
				mail_body,mail_subject);

		cloud_post( CLOUD_HOST, &post_buf, 80 );
		exit( 0 );
	}

	//if ( time(NULL) - mail_timer > PERIOD_SEND_MAIL-1 ) {
	if ( time(NULL) - mail_timer > 900 ) {

		mail_timer = time(NULL);
		
		if ( foreground ) {
			fprintf(stderr, "==> Period send mail, port: %d\n",listen_port);
		}
		
		char err_buf[BUFSIZE];
		bzero( err_buf, sizeof(err_buf));
	
		err = smtp_send("smtp.139.com", 25, NOTICER_ADDR, mail_subject, mail_body, err_buf);

		if ( err ) {
			if ( foreground ) fprintf(stderr, "Send mail err: %s",err_buf);
			log_save(err_buf);
		}
		else {
			log_save("Send mail ok.\r\n");
		}

		if ( foreground ) fprintf(stderr, "==> End send mail, return %d\r\n",err);
			
		return ;
	}
}

//return 0: ok, others: err
unsigned char sock_init( unsigned int port )
{	
	int flag=1, err, flag_len=sizeof(int);
	struct timeval timeout = {CMD_TIMEOUT*120,0};//120*5=10 mins
	
	//Create sotcket
	sock_server = socket(AF_INET, SOCK_STREAM, 0);
	if (sock_server < 0) {
		fprintf(stderr, "Socket error, check %s:%d\n",__FILE__, __LINE__);
		return 10;
	}

	//set server info
	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(port);

	/* Set socket options: Allow local port reuse in TIME_WAIT.	 */
	if (setsockopt(sock_server, SOL_SOCKET, SO_REUSEADDR,
			&flag, flag_len) == -1) {
		fprintf(stderr, "setsockopt SO_REUSEADDR error, check %s:%d\n",\
										__FILE__, __LINE__);
	}

	//设置发送超时
	//setsockopt(socket, SOL_SOCKET,SO_SNDTIMEO, (char *)&timeout,sizeof(struct timeval));
	//设置接收超时
	setsockopt(sock_server, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,sizeof(struct timeval));

	err = bind(sock_server, (struct sockaddr *)&server_addr, sizeof(server_addr));
	if (err < 0) {
		fprintf(stderr, "Bind port: %d error, please try other port.\n",port);
		return 20;
	}

	err = listen(sock_server, BACKLOG);
	if (err < 0) {
		fprintf(stderr, "Listen error, check %s:%d\n",__FILE__, __LINE__);
		return 30;
	}
	
	return 0;
}

unsigned char daemon_server(struct rokko_data *rokko, unsigned char *recv_buf, unsigned short size,\
							struct rokko_data *rokko_all, unsigned int conn_amount)
{
	unsigned char var_u8;
	unsigned char send_buf[BUFSIZE];
	unsigned short buf_index, var_u16 ;
	struct rokko_command cmd ;
	time_t ticks=time(NULL);
	
/*
	if ( foreground ) {
		fprintf(stderr, "\nRec:%d bytes from: %s:%d, %s",\
				size,(char *)inet_ntoa(rokko->client_addr.sin_addr),\
				ntohs(rokko->client_addr.sin_port),(char *)ctime(&ticks));
	}
*/

	//save to receive count
	rokko->rx_cnt += size;// =Client tx cnt
	//fprintf(stderr, "\nTotal rec %d bytes\n", rokko->rx_cnt);
	
	//find the HEAD flag: GSM_HEAD
	for ( buf_index = 0 ; buf_index < size ; buf_index++ ) {
		if ( *(recv_buf+buf_index) == GSM_HEAD ) { //found first HEAD : GSM_HEAD

			cmd.seq = *(recv_buf+buf_index+1);
			cmd.pcb = *(recv_buf+buf_index+2);
			cmd.len = *(recv_buf+buf_index+3) << 8 | *(recv_buf+buf_index+4);
			//if ( cmd.len > 1200 ) {
			if ( cmd.len > size ) {
				snprintf(send_buf,sizeof(send_buf),"!!!ERR!!! %02X,%02X,%d,%d @ %s:%d\n",\
						*(recv_buf+buf_index+0),cmd.pcb,cmd.len,size,__FILE__,__LINE__);
				log_save(send_buf, FORCE_SAVE_FILE );
				if ( foreground ) fprintf(stderr,"%s",send_buf);
			//	exit(1);
				cmd.len = 0 ;
			}
			var_u16 = ((*(recv_buf+buf_index+cmd.len+5))<<8)|(*(recv_buf+buf_index+cmd.len+6));

			//calc the CRC :
			cmd.crc16 = crc16tablefast((recv_buf+buf_index) , cmd.len+5);

			if ( (buf_index + cmd.len) > (size-7) || cmd.crc16 != var_u16 ) { //illegal package
				
				fprintf(stderr, "\r\nErr package: ");
				for ( var_u8 = 0 ; var_u8 < cmd.len+7 ; var_u8++) {
					fprintf(stderr, "%02X ",*(recv_buf+buf_index+var_u8));
				}
				fprintf(stderr, "\r\nCMD: %c(0x%02X)\tLen= %d\tRec CRC= 0x%04X\tCal CRC= 0x%04X\r\n",\
						cmd.pcb&0x7F,cmd.pcb,cmd.len,var_u16,cmd.crc16);
				fprintf(stderr, "Check %s: %d\r\n",__FILE__, __LINE__);
			}//End err package

			else {//correct package	
				
				if ( cmd.pcb < 0x80 ) {//处理客户端发来的命令

					//fprintf(stderr, "Rec CMD: %c(0x%02X) SEQ:0x%02X Len:%d at %d\r\n",\
						cmd.pcb,cmd.pcb,cmd.seq,cmd.len,buf_index);

					//handle the input cmd from clien
					switch (cmd.pcb) {

					case GSM_CMD_CONSOLE: //0x43,'C' console command
						rec_cmd_console( rokko,&cmd,\
							recv_buf+buf_index, send_buf, rokko_all, conn_amount );
						buf_index = buf_index + cmd.len ;//update index
						break;

					case GSM_CMD_ERROR: //0x45,'E' Error, upload error log
						rec_cmd_errlog( rokko,&cmd,\
							recv_buf+buf_index, send_buf );
						buf_index = buf_index + cmd.len ;//update index
						break;

					case GSM_CMD_GPS: //0x47,'G' upload GPS information to server
						rec_cmd_gps( rokko,&cmd,\
							recv_buf+buf_index, send_buf );
						buf_index = buf_index + cmd.len ;//update index
						break;

					//DE 6A 4C 00 1D 00 04 19 DA 44 45 4D 4F 44 41 33 30 42 
					//32 00 00 00 0D 31 30 2E 31 31 31 2E 32 36 2E 36 75 5C
					case GSM_CMD_LOGIN: //0x4C, 'L', Login to server

						if ( rec_cmd_login( rokko,&cmd,\
							recv_buf+buf_index, send_buf ) == 1 ) {

							//login failure, close immediately
							return 1 ;
						}
						buf_index = buf_index + cmd.len ;//update index
						break;

					case GSM_CMD_RECORD://'R', record gsm/adc data
						rec_cmd_record( rokko,&cmd,\
							recv_buf+buf_index, send_buf );
						buf_index = buf_index + cmd.len ;//update index
						break;

					case GSM_CMD_UPGRADE://0x55, 'U', Upgrade firmware
						rec_cmd_upgrade( rokko,&cmd,\
							recv_buf+buf_index, send_buf );
						buf_index = buf_index + cmd.len ;//update index
						break;

					case GSM_CMD_WARN://0x57, 'W', warn msg, report to server
						rec_cmd_warn( rokko,&cmd,\
							recv_buf+buf_index, send_buf );
						buf_index = buf_index + cmd.len ;//update index
						break;

					default:
						fprintf(stderr, "Unknow command: 0x%X\r\n",cmd.pcb);
						//cmd_unknow_cmd( mycar,&cmd,\
							recv_buf+buf_index, send_buf );

						rokko->cmd_err_cnt++;
						break;
					}//end of handle the input cmd
				}
				else {//处理客户端的响应

					fprintf(stderr, "Rec respond: %c(0x%02X) Len:%d at %d\r\n",\
						cmd.pcb&0x7F,cmd.pcb,cmd.len,buf_index);

					switch (cmd.pcb&0x7F) {

					case GSM_CMD_RECORD	: //0xD2 = 0x52 | 0x80
					case GSM_CMD_UPGRADE: //0xD5 = 0x55 | 0x80

						queue_free( rokko->send_q,&cmd ) ;
//是否需要？ buf_index = buf_index + cmd.len ;//update index
						break;

					default:
						fprintf(stderr, "Unknow command: 0x%X\r\n",cmd.pcb);		
						rokko->cmd_err_cnt++;
						break;
					}//end of handle the input cmd
				}
			}

			buf_index = buf_index + 6 ;//take HEAD(1),SEQ(1),PCB(1),LEN(2)...+CRC16
		}//end of *(recv_buf+buf_index) == GSM_HEAD )
	}

	return 0;
}
