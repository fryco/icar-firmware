/**
 *      log - Copyright (c) 2011-2012 cn0086 <cn0086.info@gmail.com>
 *
 *      This is NOT a freeware, use is subject to license terms
 *
 *      $URL: https://icar-firmware.googlecode.com/svn/rokko/daemon/v00/rokkod.c $
 *      $Rev: 284 $, $Date: 2012-11-01 15:08:23 +0800 (Thu, 01 Nov 2012) $
 */

#include "config.h"
#include "log.h"

extern int foreground, listen_port;

char logdir[EMAIL+1], logname[EMAIL+1];

FILE *logfile=NULL;

int log_init( char* path )
{
	DIR *d; 

	logdir[0] = logname[0] ='\0';

	if ( strlen(path) > EMAIL - 30 ) { //too long path
		printf("too long path: %s!!!\n",path);
		return -1;
	}
	
	strcat(logdir, path);
	strcat(logdir, "log");
	
	if(!(d = opendir(logdir))){
		//try to create dir
		//if ( mkdir(logdir, 0644) ) { //failure
		if ( mkdir(logdir, 0777) ) { //failure
			printf("error opendir %s !!!\n",logdir);
			return -1;
		}
		else { 
			printf("Create log dir %s ok\n",logdir);
		}
	}

	closedir(d);
	return 0 ;
}

int log_save(char *msg)
{
	struct timeval tv;
	char logtime[EMAIL+1] , newname[EMAIL+1];
	
	logtime[0] = newname[0] = '\0';

	gettimeofday(&tv,NULL);

	strftime(logtime,EMAIL,"/%Y%m%d",(const void *)localtime(&tv.tv_sec));
	
	strcat(newname,logdir);strcat(newname,logtime);
	snprintf(&newname[strlen(newname)],EMAIL,"_%d.txt",listen_port);

	strftime(logtime,EMAIL,"%T ",(const void *)localtime(&tv.tv_sec));
	
	//printf("Time:%s\n",logtime);printf("Logname:%s\n",logname);printf("newname:%s\n",newname);
	
	if ( logfile ) { 
		
		if ( strcmp(newname,logname) ) {//different
			fclose(logfile);
			logfile=NULL;
			strcat(logname, newname);

			//create new log file
			logfile = fopen(newname, "a");
		}

		fprintf(logfile, "%s", logtime);		
		fprintf(logfile, "%s", msg);
	}
	else { //create log file first
		logfile = fopen(newname, "a");
		if ( logfile ) {
			fprintf(logfile, "%s", logtime);
			fprintf(logfile, "%s", msg);
			strcat(logname, newname);
		}
		else {
			return -1;
		}
	}
	if ( foreground ) printf("%s %s",logtime,msg);
		
	return 0;
	
}
