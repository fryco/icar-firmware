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
/*
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <dirent.h>

#include <time.h>
#include <sys/param.h>
#include <sys/sysinfo.h>
#include <sys/wait.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <utmp.h>
#include <time.h>
#include <termios.h>
#include <fcntl.h>

#include <netdb.h>

#include <sys/param.h>
#include <sys/sysinfo.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>

#define NONE				"\033[m"  
#define RED					"\033[0;32;31m"  
#define LIGHT_RED			"\033[1;31m"  
#define GREEN				"\033[0;32;32m"  
#define LIGHT_GREEN			"\033[1;32m"  
#define BLUE				"\033[0;32;34m"  
#define LIGHT_BLUE			"\033[1;34m"  
#define DARY_GRAY			"\033[1;30m"  
#define CYAN				"\033[0;36m"  
#define LIGHT_CYAN			"\033[1;36m"  
#define PURPLE				"\033[0;35m"  
#define LIGHT_PURPLE		"\033[1;35m"  
#define BROWN				"\033[0;33m"  
#define YELLOW				"\033[1;33m"  
#define LIGHT_GRAY			"\033[0;37m"  
#define WHITE				"\033[1;37m"  


#define PRODUCT_SN_LEN		8		//IMEI: 123456789012345 ==> 0x01 0x23 0x45 ... 0x45
#define	MAXCLIENT 			5000

#define	EMAIL				512

#define MAX_DISPLAY			100

#define GSM_HEAD				0xDE
#define GSM_CMD_CONSOLE			0x43 //'C', Console command

#define GSM_CMD_ERROR			0x45 //'E', upload error log to server
//Out: DE 01 45 00 06 00 00 00 08 30 00 81 79
//In : DE 01 C5 00 02 00 04 08 4D
#define GSM_CMD_LOGIN			0x4C //'L', Login
//Out: DE 00 4C 00 1E 00 00 08 B6 44 45 4D 4F 43 34 33 45 45 39 00 00 00 DE 31 30 2E 38 31 2E 32 33 37 2E 39 36 FF E8
//In : DE 00 CC 00 11 00 50 EB D4 FC 06 6B FF 48 56 48 67 49 87 10 12 37 B8 C4
#define GSM_CMD_RECORD			0x52 //'R', Record vehicle parameters
//Out: DE 04 52 00 08 51 1F 4C 4E 00 0C 30 1A A5 1C
//In : DE 04 D2 00 02 00 00 68 46

//For Console command
#define CONSOLE_CMD_NONE		0	 //no cmd
#define CONSOLE_CMD_LIST_ALL	0x4C //'L', List all
//Out: DE 0A 43 00 01 4C 2D 80
//In : DE 06 C3 85 52 00 AA 09 97 75 53 31 10 30 18 09 97 75 53 31 10 30 23 09 97 75 53 31 10 30 
//     中间省略n行… (0xAA = 170 ), 
//     97 75 53 31 10 39 51 09 97 75 53 31 10 39 55 09 97 75 53 31 10 39 56 09 97 75 53 31 10 39 
//     61 09 97 75 53 31 10 39 63 09 97 75 53 31 10 39 70 06 66
#define CONSOLE_CMD_LIST_SPE	0x6C //'l', List special

//record index define
#define REC_IDX_ADC1				10 //for ADC1
#define REC_IDX_ADC2				20 //for ADC2
#define REC_IDX_ADC3				30 //for ADC3
#define REC_IDX_ADC4				40 //for ADC4
#define REC_IDX_ADC5				50 //for ADC5
#define REC_IDX_MCU					60 //for MCU temperature
#define REC_IDX_GSM					90 //for GSM signal


#define NOTICER_ADDR		"13828431106@139.com"

#endif /* _CONFIG_H */
