/**
 *      config - Copyright (c) 2011-2012 cn0086 <cn0086.info@gmail.com>
 *
 *      This is NOT a freeware, use is subject to license terms
 *
 *      $URL: https://icar-firmware.googlecode.com/svn/rokko/daemon_v01/config.h $
 *      $Rev: 364 $, $Date: 2013-05-09 19:20:16 +0800 (Thu, 09 May 2013) $

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
#include <fcntl.h>

#include <netdb.h>

#include <sys/param.h>
#include <sys/sysinfo.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>

#include <mysql.h>

#define	EMAIL				128
#define BUFSIZE 			2048*1024	//2MB

//#define	CLOUD_HOST				"cn0086.info"
#define	CLOUD_HOST				"yun.test.33xuexi.com"

#endif /* _CONFIG_H */
