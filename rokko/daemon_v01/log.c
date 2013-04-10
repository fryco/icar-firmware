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

static char logdir[EMAIL+1], log_buf[BUFSIZE*20];
static unsigned int save_timer;

unsigned char log_init( char* path )
{
	DIR *d; 

	bzero(logdir, sizeof(logdir));

	if ( strlen(path) > EMAIL - 30 ) { //too long path
		fprintf(stderr, "too long path: %s!!!\n",path);
		return -1;
	}
	
	strcat(logdir, path);
	strcat(logdir, "log");

	if(!(d = opendir(logdir))){
		//try to create dir
		if ( mkdir(logdir, 0600) ) { //failure
			perror("Open dir error!");
			fprintf(stderr, "dir:%s %s:%d\n",logdir,__FILE__,__LINE__);
			return -1;
		}
		else { 
			fprintf(stderr, "Create log dir %s ok\n\n",logdir);
		}
	}

	closedir(d);
	return 0 ;
}

int log_err( char *msg)
{
	struct timeval log_tv;
	char logtime[EMAIL+1], logname[EMAIL+1];
	FILE *fp;
	
	bzero(logtime, sizeof(logtime));bzero(logname, sizeof(logname));
	
	gettimeofday(&log_tv,NULL);
	strftime(logtime,EMAIL,"%Y%m%d %T ",(const void *)localtime(&log_tv.tv_sec));
	
	//Create log file
	strcat(logname,logdir);strcat(logname,"/err.txt");

	fp = fopen(logname, "a");
	if ( fp ) { 
		fprintf(fp, "%s", logtime);
		fprintf(fp, "%s", msg);
		fclose(fp) ;

		return 0;
	}
	else {
		return 1;
	}
}

//return 0: ok, others: err
unsigned char save_to_disk( )
{
	struct timeval log_tv;
	char logtime[EMAIL+1], logname[EMAIL+1];

	FILE *fp;
	
	bzero(logtime, sizeof(logtime));bzero(logname, sizeof(logname));
	
	//Create log file
	gettimeofday(&log_tv,NULL);
	strftime(logtime,EMAIL,"/%Y%m%d",(const void *)localtime(&log_tv.tv_sec));
	strcat(logname,logdir);strcat(logname,logtime);
	snprintf(&logname[strlen(logname)],16,"_%d.txt",listen_port);
	//==> /tmp/log/20130115_23.txt

	fp = fopen(logname, "a");

	if ( !fp ) {
		perror("File open error!");
		fprintf(stderr, "%s:%d\n",__FILE__,__LINE__);
		return 1 ;
	}

	fprintf(fp, "%s", log_buf);
	fclose(fp);
	bzero( log_buf, sizeof(log_buf));//clean buffer
	
	return 0 ;
}

unsigned char log_save( char *msg , unsigned char force_save)
{
	unsigned int var_u32;
	unsigned char ret;
	char logtime[EMAIL+1];	
	time_t t;
	struct tm *tmp;
	
	//return 0 ;//for test only
	
	t = time(NULL);	tmp = localtime(&t);
	if ( tmp ) {
		bzero(logtime, sizeof(logtime));
		strftime(logtime,EMAIL,"%T ",tmp);	
		//if ( foreground ) fprintf(stderr, "logtime= %s",logtime);
	}
	
	//check msg length	
	var_u32 = sizeof(log_buf) - strlen(log_buf) ;
	
	if ( var_u32 > 	strlen( msg ) ) { //enough buffer
		strcat(log_buf, logtime); strcat(log_buf, msg);
	}
	else { //buffer no enough
		if ( save_to_disk( ) ){//save file failure
			fprintf(stderr, "Save file error! %s:%d\n",__FILE__,__LINE__);
			bzero( log_buf, sizeof(log_buf));//force clean buffer to avoid overflow
		}

		//copy msg to log_buf
		strcat(log_buf, logtime); strcat(log_buf, msg);
	}
	
	//if ( foreground ) fprintf(stderr, "%s",msg);
	
	if ( force_save || ( (time(NULL) - save_timer) > PERIOD_SAVE_FILE) || \
		 (strlen(log_buf)*2 > sizeof(log_buf)) ) {

			save_timer = time(NULL);
			return save_to_disk( );
	}
	
	return 0 ;
}
