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
char log_msg[BUFSIZE+1];

static time_t last_time;

extern char logname[EMAIL+1];
extern FILE *logfile;

int main(int argc, char *argv[])
{
	void handler(int);

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
	
		snprintf(log_msg,BUFSIZE,"==> Start rokko daemon, PID: %d, port: %d\n",getpid(),listen_port);
		if (log_save(log_msg)) {
			printf("Save log error! %s:%d\n",__FILE__,__LINE__);
			exit(1);
		}

	/* Now run in the background. */
	if (!foreground) {
		fclose(logfile);
		logfile=NULL;
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
			while ( 1 ) {
				sleep( PERIOD_CHECK_DB ) ;
				period_check( );					
			}
			break;
			
		case -1:
			perror("fork failed"); exit(1);
		default:
			break;
	}


	/* The main loop. */
	while (1)
	{

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
	printf("Listen port: %d\n",listen_port);
}

void handler(int s)
{
	
	//fprintf(stderr,"PID:%d %d\n",getpid(),__LINE__);
	/* Exit gracefully. */
	if(pidfile[0])
		unlink(pidfile);
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
void period_check(  )
{
	time_t now_time=time(NULL);
	char mail_buf[BUFSIZE], err;
	
	printf(" --> Period check, port: %d\n",listen_port);
	
	if ( now_time - last_time > PERIOD_SEND_MAIL ) {
		mail_buf[0] = '\0';
		snprintf(mail_buf,BUFSIZE,"Daemon CHK %.24s\r\n",(char *)ctime((&now_time)));
	
		//err = smtp_send("smtp.139.com", 25, mail_notice, mail_buf, "\r\n");
		if ( err ) {
			log_save("Send mail failure!\r\n");
			//exit(1);
		}
		else {
			log_save("Send mail ok.\r\n");
			last_time = now_time;
		}
		
		snprintf(log_msg,BUFSIZE,"PID: %d, port: %d\n",getpid(),listen_port);
		if (log_save(log_msg)) {
			printf("Save log error! %s:%d\n",__FILE__,__LINE__);
			exit(1);
		}

		get_sysinfo( log_msg, BUFSIZE) ;
		log_save( log_msg ) ;

	}
}
