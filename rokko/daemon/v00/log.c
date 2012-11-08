/**
 *      log - Copyright (c) 2011-2012 cn0086 <cn0086.info@gmail.com>
 *
 *      This is NOT a freeware, use is subject to license terms
 *
 *      $URL$
 *      $Rev$, $Date$
 */

#include "config.h"
#include "log.h"

extern int foreground, listen_port;

char logdir[EMAIL+1], logname[EMAIL+1];

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
		if ( mkdir(logdir, 0644) ) { //failure
			printf("error opendir %s !!!\n",logdir);
			return -1;
		}
		else { 
			printf("Create log dir %s ok\n\n",logdir);
		}
	}

	closedir(d);
	return 0 ;
}

int log_save1(FILE *log, char *msg)
{
	struct timeval tv;
	char logtime[EMAIL+1] , newname[EMAIL+1];

	gettimeofday(&tv,NULL);

	strftime(logtime,EMAIL,"/%Y%m%d",(const void *)localtime(&tv.tv_sec));
	
	strcat(newname,logdir);strcat(newname,logtime);
	snprintf(&newname[strlen(newname)],EMAIL,"_%d.txt",listen_port);

	strftime(logtime,EMAIL,"%T ",(const void *)localtime(&tv.tv_sec));
	
	
	if ( log ) { 
		fprintf(log, "%s", logtime);
		fprintf(log, "%s", msg);
		fflush(log);
	}		

	if ( foreground ) printf("%s %s",logtime,msg);

	return 0;	
}

int log_err( char *msg)
{
	struct timeval tv;
	char logtime[EMAIL+1];
	FILE *fp;
	
	gettimeofday(&tv,NULL);

	strftime(logtime,EMAIL,"/%Y%m%d %T ",(const void *)localtime(&tv.tv_sec));
	
	fp = fopen("/tmp/log/log_err.txt", "a");
	if ( fp ) { 
		fprintf(fp, "%s", logtime);
		fprintf(fp, "%s", msg);
		fflush(fp);
		fclose(fp) ;
	}

	return 0;	
}
