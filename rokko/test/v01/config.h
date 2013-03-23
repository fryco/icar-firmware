/**
 *      config - Copyright (c) 2011-2012 cn0086 <cn0086.info@gmail.com>
 *
 *      This is NOT a freeware, use is subject to license terms
 *
 *      $URL: https://icar-firmware.googlecode.com/svn/rokko/daemon/v00/config.h $
 *      $Rev: 285 $, $Date: 2012-11-06 08:46:19 +0800 (Tue, 06 Nov 2012) $
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
#include <sys/wait.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#define	EMAIL				128
#define BUFSIZE 			256

#define CMD_CNT				0

#define GSM_HEAD				0xDE
#define GSM_CMD_ERROR			0x45 //'E', upload error log to server
//Out: DE 01 45 00 06 00 00 00 08 30 00 81 79
//In : DE 01 C5 00 02 00 04 08 4D
#define GSM_CMD_LOGIN			0x4C //'L', Login
//Out: DE 00 4C 00 1E 00 00 08 B6 44 45 4D 4F 43 34 33 45 45 39 00 00 00 DE 31 30 2E 38 31 2E 32 33 37 2E 39 36 FF E8
//In : DE 00 CC 00 11 00 50 EB D4 FC 06 6B FF 48 56 48 67 49 87 10 12 37 B8 C4

#define	LOG_DIR				"/tmp/"

#endif /* _CONFIG_H */
