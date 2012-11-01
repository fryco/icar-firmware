/**
 *      rokkod - Copyright (c) 2011-2012 cn0086 <cn0086.info@gmail.com>
 *
 *      This is NOT a freeware, use is subject to license terms
 *
 *      $URL: https://icar-firmware.googlecode.com/svn/ubuntu/iCar_v03/tcp_server.c $
 *      $Rev: 99 $, $Date: 2012-02-29 09:32:56 +0800 (Wed, 29 Feb 2012) $
 */

#include "config.h"
#include "rokkod.h"

#define rokkod_RELEASE "\nRokko daemon v00, built by cn0086@139.com at " __DATE__" "__TIME__ "\n"

int update_interval=5, foreground=0, listen_port=23;

char pidfile[EMAIL+1];

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
 				printf("rokkod is already running.\n");
 				exit(1);
 			}
 		}
 	}		


 	/* Add signal handler for hangups/terminators. */
 	signal(SIGTERM,handler); //exit when run kill...

	//signal(SIGCHLD, handler); exit when ???
	signal(SIGINT, handler); //exit when Crtl-C


	/* Now run in the background. */
	if (!foreground) bg();

	fprintf(stderr,"PID:%d %d\n",getpid(),__LINE__);

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

void handler(int s)
{
	
	//fprintf(stderr,"PID:%d %d\n",getpid(),__LINE__);
	/* Exit gracefully. */
	if(pidfile[0])
		unlink(pidfile);
	exit(0);
}
