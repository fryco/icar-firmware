/**
 *      rokkod - Copyright (c) 2011-2012 cn0086 <cn0086.info@gmail.com>
 *
 *      This is NOT a freeware, use is subject to license terms
 *
 *      $URL$
 *      $Rev$, $Date$
 */

#include "config.h"
#include "rokkod.h"

#define rokkod_RELEASE "\nRokko daemon v00, built by cn0086.info@gmail.com at " __DATE__" "__TIME__ "\n"

//for default parameters
unsigned int update_interval=5, foreground=0, listen_port=23;

int sock_server = -1;
struct sockaddr_in server_addr;
	
char pidfile[EMAIL+1];

static time_t last_time = 0;

extern char logdir[EMAIL+1], logname[EMAIL+1];

FILE *logfile=NULL;

int main(int argc, char *argv[])
{
	void handler(int);
	void child_exit(int);
	unsigned int i=0;

	/* Properly initialize these to an empty string */
	pidfile[0] = '\0';

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
		bg();
	}

	{
		FILE *pidf;

		pidf = fopen(pidfile, "w");
		if(pidf)
		{
			fprintf(pidf, "%d", getpid());
			fclose(pidf);
		}
		else
		{
			pidfile[0] = '\0';
		}
	}


	//Create new process (non-block) for period_check
	switch(fork())
	{
		case 0://In child process
			fprintf(stderr, "In child:%d for period_check\n",getpid());
			
			FILE *child_log;

			struct timeval tv;
			char logtime[EMAIL+1] , newname[EMAIL+1];
		
			gettimeofday(&tv,NULL);
		
			strftime(logtime,EMAIL,"/%Y%m%d",(const void *)localtime(&tv.tv_sec));
			
			strcat(newname,logdir);strcat(newname,logtime);
			snprintf(&newname[strlen(newname)],16,"_%d.txt",listen_port);
			strcat(logname, newname);
	
			child_log = fopen(newname, "a");

			if ( !child_log ) {
				log_err("file open err!");
			}

			snprintf(logtime,EMAIL,"==> Start rokko daemon, PID: %d, port: %d\n",getpid(),listen_port);
			log_save(child_log, logtime);

			//while ( 1 ) {
			while ( 0 ) {
				sleep( PERIOD_CHECK_DB ) ;

				bzero( logtime, sizeof(logtime));	
				bzero( newname, sizeof(newname));

				gettimeofday(&tv,NULL);
				strftime(logtime,EMAIL,"/%Y%m%d",(const void *)localtime(&tv.tv_sec));
				
				strcat(newname,logdir);strcat(newname,logtime);
				snprintf(&newname[strlen(newname)],16,"_%d.txt",listen_port);
		
				child_log = fopen(newname, "a");

				if ( child_log ) {
					period_check( child_log );
					fclose(child_log);
				}
				else {
					bzero( logtime, sizeof(logtime));	
					snprintf(logtime,EMAIL,"open %s err! %d\n",newname,__LINE__);
					log_err(logtime);
				}
			}
			exit(0);
			
		case -1:
			perror("fork failed"); exit(1);
		default:
			break;
	}

	/* Main program, create log file first */
	char log_buffer[BUFSIZE+1];
	struct rokko_data rokko ;

	if ( sock_init( listen_port ) ) {//err
		fprintf(stderr, "sock init err!\r\n");
		exit(1);
	}
	
	/* Main loop, accept TCP socket connect */
	while (1)
	{

		int addrlen = sizeof(struct sockaddr);

		//Accept new connection
		rokko.client_socket = accept(sock_server, (struct sockaddr *)&rokko.client_addr, &addrlen);
		if (rokko.client_socket < 0) {
			fprintf(stderr, "Accept new connection error, check %s:%d",__FILE__, __LINE__);
			continue; //error, stop this connection
		}

		//Create new process for this connection
		switch(fork())
		{
			case 0://In child process
				fprintf(stderr, "\nChild:%d for new connection\t",getpid());
				
				close(sock_server);
				
				daemon_server(&rokko);

				bzero( log_buffer, sizeof(log_buffer));
				snprintf(log_buffer,BUFSIZE,"Close  connection, PID: %d, from: %s:%d\n",\
									getpid(),(char *)inet_ntoa(rokko.client_addr.sin_addr),\
									ntohs(rokko.client_addr.sin_port));

				close(rokko.client_socket);

				exit(0);
				
			case -1:
				perror("fork failed"); exit(1);
			default:
				close(rokko.client_socket);
				break;
		}

		/* Save valueable CPU cycles. */
		sleep(update_interval);
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

	//fprintf(stderr, "In child:%d\n",getpid());

	if (-1==setsid()) {
		perror("setsid failed"); exit(1);
	}

	/* Close probably all file descriptors */
	for (i = 0; i<NOFILE; i++)
		close(i);

	/* Be nice to umount */
	chdir("/");
	
	umask(0); /* 重设文件创建掩码 */ 
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
	
	if ( logfile ) { 
		fclose(logfile);
	}
	
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
	fprintf(stderr, "  -b             create bootid and exit [ignored on FreeBSD]\n");
	fprintf(stderr, "  -f             run in foreground [debug]\n");
	fprintf(stderr, "  -d             log the detail\n");
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
void period_check( FILE *fp)
{
	time_t now_time=time(NULL);
	char mail_buf[BUFSIZE+1], log_buf[BUFSIZE+1],err;
	
	fprintf(stderr, "now: %d\tlast: %d\t%d\r\n",(int)now_time,(int)last_time,\
			(int)(now_time - last_time));
	if ( now_time - last_time > PERIOD_SEND_MAIL ) {
		//bzero( log_buf, sizeof(log_buf));
		//snprintf( log_buf, sizeof(log_buf),"last_time = %d\n\n",last_time);
		//log_save(fp, log_buf);

		last_time = now_time;
		fprintf(stderr, "==> Period send mail, port: %d\n",listen_port);
		
		mail_buf[0] = '\0';
		snprintf(mail_buf,BUFSIZE,"C0 %.24s\r\n",(char *)ctime((&now_time)));
	
		err = smtp_send("smtp.139.com", 25, NOTICER_ADDR, mail_buf, "\r\n",log_buf);

		if ( err ) {
			//fprintf(stderr, "Err: %s",log_buf);
			log_save(fp, log_buf);
		}
		else {
			log_save(fp, "Send mail ok.\r\n");
		}
		fprintf(stderr, "==> End send mail, return %d\r\n",err);
		
		if ( 0 ) {
			bzero( log_buf, sizeof(log_buf));
	
			get_sysinfo( log_buf, BUFSIZE) ;
			log_save(fp, log_buf ) ;
		}
	}
}

//return 0: ok, others: err
unsigned char sock_init( unsigned int port )
{	
	int flag=1, err, flag_len=sizeof(int);
	
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

void daemon_server(struct rokko_data *rokko)
{
	ssize_t size = 0;
	unsigned char recv_buf[BUFSIZE+1];
	unsigned char send_buf[BUFSIZE+1];
	unsigned int buf_index ;
	unsigned short recv_crc16;
	unsigned int chk_count ;
	time_t ticks=time(NULL);
	struct tm *tblock;
	int cmd_err_cnt = 0 ;//command error count
	struct rokko_command cmd ;

	unsigned char var_u8 ;
	pid_t cloud_pid;
	unsigned char post_buf[BUFSIZE];

	memset(cmd.pro_sn, 0x0, 10);
	rokko->sn = cmd.pro_sn ;

	if ( foreground ) {
		fprintf(stderr, "%sFrom: %s:%d\n",\
				(char *)ctime(&ticks),(char *)inet_ntoa(rokko->client_addr.sin_addr),\
				ntohs(rokko->client_addr.sin_port));
	}

	//connect mysql
/*
	if(db_connect(&(rokko->mydb)))
	{//failure
		fprintf(stderr, "Database no ready, exit.\n");
		fprintf(stderr, "Check: 1, Have install mysql?\n");
		fprintf(stderr, "       2, host, user, password, database are correct?\n");
		return ;
	}
*/
	while ( 0 )
	{//for test only, send current time continue
		ticks=time(NULL);
		memset(send_buf, '\0', BUFSIZE);
		snprintf(send_buf,100,"%.24s, From %s:%d\r\n",(char *)ctime(&ticks),\
				(char *)inet_ntoa(rokko->client_addr.sin_addr),ntohs(rokko->client_addr.sin_port));
		write(rokko->client_socket,send_buf,strlen(send_buf));
		sleep( 3 );
	}

	while ( 1 ) {

		//read the content from remote site
		memset(recv_buf, '\0', BUFSIZE);

		//以下基于假设：每次读取1个或更多包，不会收到不完整包
		//HEAD+SEQ+PCB+Length, please refer to: rokko_protocol_通讯协议
		size = read(rokko->client_socket, recv_buf, BUFSIZE);
		if (size == 0 || size > BUFSIZE) { //no data or overflow
			return;
		}

		ticks=time(NULL);
		fprintf(stderr, "\r\n%.24s  Rec:%02d Bytes\r\n",(char *)ctime(&ticks),size-1);

		//find the HEAD flag: GSM_HEAD
		for ( buf_index = 0 ; buf_index < size ; buf_index++ ) {
			if ( recv_buf[buf_index] == GSM_HEAD ) { //found first HEAD : GSM_HEAD

				cmd.pcb = recv_buf[buf_index+2];
				cmd.len = recv_buf[buf_index+3] << 8 | recv_buf[buf_index+4];
				recv_crc16 = ((recv_buf[buf_index+cmd.len+5])<<8)|(recv_buf[buf_index+cmd.len+6]);

				//calc the CRC :
				cmd.crc16 = crc16tablefast(&recv_buf[buf_index] , cmd.len+5);
			
				if ( (buf_index + cmd.len) > (size-7) || cmd.crc16 != recv_crc16 ) { //illegal package

					fprintf(stderr, "\r\nErr package: ");
					for ( chk_count = 0 ; chk_count < cmd.len+7 ; chk_count++) {
						fprintf(stderr, "%02X ",recv_buf[buf_index+chk_count]);
					}
					fprintf(stderr, "\r\nCMD: %c\tLen= %d\tRec CRC= 0x%04X\tCal CRC= 0x%04X\r\n",\
							cmd.pcb,cmd.len,recv_crc16,cmd.crc16);
					fprintf(stderr, "Check %s, line: %d\r\n",__FILE__, __LINE__);
				}//End err package
				else {//correct package
					fprintf(stderr, "at %d CMD: %c Len:%d\r\n",buf_index,cmd.pcb,cmd.len);
				}

				buf_index = buf_index + 6 ;//take HEAD(1),SEQ(1),PCB(1),LEN(2)...+CRC16
			}//end of if ( recv_buf[buf_index] == GSM_HEAD )
		}
	}//End of while( 1 )
		
exit_process_conn_server:

	mysql_close(&(rokko->mydb.mysql));
}
