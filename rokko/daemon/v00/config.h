/**
 *      config - Copyright (c) 2011-2012 cn0086 <cn0086.info@gmail.com>
 *
 *      This is NOT a freeware, use is subject to license terms
 *
 *      $URL$
 *      $Rev$, $Date$

 */

#ifndef _CONFIG_H 
#define _CONFIG_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <utmp.h>
#include <time.h>

#include <netdb.h>

#include <sys/param.h>
#include <sys/sysinfo.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>


#define	EMAIL				128
#define BUFSIZE 			2048

#define	LOG_DIR				"/tmp/"

//#define PERIOD_CHECK_DB		3*60	//check database every 3*60 seconds
#define PERIOD_SEND_MAIL	1*60*60	//send mail every 1*60*60 seconds

#define PERIOD_CHECK_DB		3	//check database every 3*60 seconds
//#define PERIOD_SEND_MAIL	10	//send mail every 1*60*60 seconds

#define NOTICER_ADDR		"cn0086@139.com"
#define EMERGENCY_ADDR		"cn0086@139.com"


#endif /* _CONFIG_H */
