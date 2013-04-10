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
#include <termios.h>
#include <fcntl.h>

#include <time.h>
#include <sys/param.h>
#include <sys/sysinfo.h>
#include <sys/wait.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#define	EMAIL				128
#define BUFSIZE 			1024*2		//2KB

#define CMD_CNT				0

#define GSM_HEAD				0xDE
#define GSM_CMD_CONSOLE			0x43 //'C', Console command
//--> 

#define GSM_CMD_ERROR			0x45 //'E', upload error log to server
//Out: DE 01 45 00 06 00 00 00 08 30 00 81 79
//In : DE 01 C5 00 02 00 04 08 4D
#define GSM_CMD_LOGIN			0x4C //'L', Login
//Out: DE 00 4C 00 1E 00 00 08 B6 44 45 4D 4F 43 34 33 45 45 39 00 00 00 DE 31 30 2E 38 31 2E 32 33 37 2E 39 36 FF E8
//In : DE 00 CC 00 11 00 50 EB D4 FC 06 6B FF 48 56 48 67 49 87 10 12 37 B8 C4
#define GSM_CMD_RECORD			0x52 //'R', Record vehicle parameters
//Out: DE 04 52 00 08 51 1F 4C 4E 00 0C 30 1A A5 1C
//In : DE 04 D2 00 02 00 00 68 46

//record index define
#define REC_IDX_ADC1				10 //for ADC1
#define REC_IDX_ADC2				20 //for ADC2
#define REC_IDX_ADC3				30 //for ADC3
#define REC_IDX_ADC4				40 //for ADC4
#define REC_IDX_ADC5				50 //for ADC5
#define REC_IDX_MCU					60 //for MCU temperature
#define REC_IDX_GSM					90 //for GSM signal


#define	LOG_DIR				"/tmp/"

#endif /* _CONFIG_H */
