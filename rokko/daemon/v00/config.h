/**
 *      config - Copyright (c) 2011-2012 cn0086 <cn0086.info@gmail.com>
 *
 *      This is NOT a freeware, use is subject to license terms
 *
 *      $URL$
 *      $Rev$, $Date$
#include <time.h>
#include <sys/time.h>
#include <sys/param.h>
#include <sys/utsname.h>
#include <syslog.h>

#include <sys/types.h>
#include <sys/wait.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>

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
#include <sys/param.h>
#include <sys/sysinfo.h>

//#include <sys/stat.h>

//#include "log.h"

#define	EMAIL	128
#define BUFSIZE 1024*2

//#define PERIOD_CHECK_DB		3*60	//check database every 3*60 seconds
//#define PERIOD_SEND_MAIL	5*60*60	//send mail every 1*60*60 seconds

#define PERIOD_CHECK_DB		3	//check database every 3*60 seconds
#define PERIOD_SEND_MAIL	5	//send mail every 1*60*60 seconds

#endif /* _CONFIG_H */
