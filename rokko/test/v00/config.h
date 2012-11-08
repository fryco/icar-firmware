/**
 *      config - Copyright (c) 2011-2012 cn0086 <cn0086.info@gmail.com>
 *
 *      This is NOT a freeware, use is subject to license terms
 *
 *      $URL: https://icar-firmware.googlecode.com/svn/rokko/daemon/v00/config.h $
 *      $Rev: 285 $, $Date: 2012-11-06 08:46:19 +0800 (Tue, 06 Nov 2012) $
#include <time.h>
#include <sys/time.h>
#include <sys/param.h>
#include <sys/utsname.h>
#include <syslog.h>

#include <sys/wait.h>

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

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#define	EMAIL				128
#define BUFSIZE 			1024*2

#define CMD_CNT				3

#define	LOG_DIR				"/tmp/"

#define	SERVER_ADDR			"127.0.0.1"
#define	SERVER_PORT			23

#endif /* _CONFIG_H */
