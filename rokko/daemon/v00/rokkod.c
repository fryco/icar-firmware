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

#define rokkod_RELEASE "\nRokko daemon v00, built by cn0086@139.com at " __DATE__" "__TIME__ "\n"

int update_interval=5, foreground=0, listen_port=23;

char pidfile[EMAIL+1];

static time_t last_time;

extern char logdir[EMAIL+1], logname[EMAIL+1];

FILE *logfile=NULL;

int main(int argc, char *argv[])
{
	void handler(int);
	unsigned int i=0;
	char log_buffer[BUFSIZE+1];
	FILE *fh=NULL;

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
 				printf("rokkod is already running, PID: %d.\n",pid);
 				exit(1);
 			}
 		}
 	}		

 	if ( log_init( "/tmp/" ) ) { //error
		printf("Create log dir error! %s:%d\n",__FILE__,__LINE__);
		exit(1);
	}

 	/* Add signal handler for hangups/terminators. */
 	signal(SIGTERM,handler); //exit when run kill...

	signal(SIGINT, handler); //exit when Crtl-C

	//signal(SIGCHLD, SIG_IGN); /* 忽略子进程结束信号，防止出现僵尸进程 */ 
	
	printf("Listen port: %d\n\n",listen_port);
	
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
			printf("In child:%d for period_check\n",getpid());
			last_time=time(NULL);
			
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
			log_save1(child_log, logtime);

			while ( 1 ) {
				sleep( PERIOD_CHECK_DB ) ;

				memset(logtime, '\0', sizeof(logtime));	
				memset(newname, '\0', sizeof(newname));

				gettimeofday(&tv,NULL);
				strftime(logtime,EMAIL,"/%Y%m%d",(const void *)localtime(&tv.tv_sec));
				
				strcat(newname,logdir);strcat(newname,logtime);
				snprintf(&newname[strlen(newname)],16,"_%d.txt",listen_port);

				if ( strcmp(newname,logname) ) {//different
					fflush(child_log);
					fclose(child_log);
					strcat(logname, newname);
		
					//create new log file
					sleep( 10 ) ;
					child_log = fopen(newname, "a");

					if ( !child_log ) {
						memset(logtime, '\0', sizeof(logtime));	
						snprintf(logtime,EMAIL,"open %s err! %d\n",newname,__LINE__);
						log_err(logtime);
					}					
				}

				period_check( child_log );
			}
			break;
			
		case -1:
			perror("fork failed"); exit(1);
		default:
			break;
	}


		fh = fopen("/tmp/log/fh.txt", "a");

		if ( !fh ) {
			exit(1);
		}

	/* The main loop. */
	while (1)
	{

		/* Save valueable CPU cycles. */
		sleep(update_interval);

		if ( fh ) {
			
			memset(log_buffer, '\0', sizeof(log_buffer));

			get_sysinfo( log_buffer, BUFSIZE) ;
			log_save1(fh, log_buffer ) ;

			snprintf(log_buffer,EMAIL,"%d PID:%d\n",i,getpid());

			log_save1(fh, log_buffer);
		
			i++;
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

	//printf("In child:%d\n",getpid());

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
					printf("Bad port number, the port must: 1 ~ 65535\n");
					exit(1);
				}
				break;

		}
	}
	
	printf("%s\n", rokkod_RELEASE );
}

void handler(int s)
{
	
	//fprintf(stderr,"PID:%d %d\n",getpid(),__LINE__);
	/* Exit gracefully. */
	if(pidfile[0]) {
		unlink(pidfile);
	}
	
	if ( logfile ) { 
		fclose(logfile);
	}
	
	exit(0);
}

void print_help(char *argv[])
{
	printf("%s\n", rokkod_RELEASE );
	printf("usage: %s [OPTION]...\n", argv[0]);
	printf("commandline options override settings from configuration file\n\n");
	printf("  -?             this help\n");
	printf("  -b             create bootid and exit [ignored on FreeBSD]\n");
	printf("  -f             run in foreground [debug]\n");
	printf("  -d             log the detail\n");
	printf("  -p             port, default is 23\n\n");
	exit(0);
}

void print_version(void)
{
	printf("%s\n", rokkod_RELEASE );
	printf("enhanced and maintained by cn0086 <cn0086.info@gmail.com>\n\n");
	exit(0);
}

// 定时检查服务器并发邮件
void period_check( FILE *fp)
{
	time_t now_time=time(NULL);
	char mail_buf[BUFSIZE+1], log_buf[BUFSIZE+1], err;
	
	printf(" --> Period check, port: %d\n",listen_port);
	
	if ( now_time - last_time > PERIOD_SEND_MAIL ) {
		mail_buf[0] = '\0';
		snprintf(mail_buf,BUFSIZE,"Daemon CHK %.24s\r\n",(char *)ctime((&now_time)));
	
		//err = smtp_send("smtp.139.com", 25, mail_notice, mail_buf, "\r\n");
		if ( err ) {
			;//log_save("Send mail failure!\r\n");
			//exit(1);
		}
		else {
			//log_save("Send mail ok.\r\n");
			last_time = now_time;
		}
		
		if ( fp ) {
			memset(log_buf, '\0', sizeof(log_buf));
	
			get_sysinfo( log_buf, BUFSIZE) ;
			log_save1(fp, log_buf ) ;
		}
	}
}
