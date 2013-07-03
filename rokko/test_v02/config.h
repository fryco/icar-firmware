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
#include <sys/param.h>
#include <sys/sysinfo.h>
#include <sys/wait.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PRODUCT_SN_LEN				8	//IMEI: 123456789012345 ==> 0x01 0x23 0x45 ... 0x45

#define	EMAIL						128
#define BUFSIZE 					2048	//2KB

#define GSM_HEAD					0xDE
#define GSM_CMD_ERROR				0x45 //'E', upload error log to server
//Out: DE 01 45 00 06 00 00 00 08 30 00 81 79
//In : DE 01 C5 00 02 00 04 08 4D
#define GSM_CMD_GPS					0x47 //'G', upload GPS information to server
//Out: DE 3C 47 00 10 51 8B 49 CF 05 E4 32 2E 01 A4 BB 12 01 8F C5 00 AD 94
//In : DE 4B C7 00 01 00 08 38

#define GSM_CMD_LOGIN				0x4C //'L', Login
//Out: DE 00 4C 00 1B 00 00 00 00 09 97 75 53 31 10 18 75 00 00 12 34 56 78 31 32 37 2E 30 2E 30 2E 31 9B 65
//In : DE 00 CC 00 11 00 51 D3 CC E4 06 6B FF 48 56 48 67 49 87 10 12 37 D4 49 to 997755331101875
#define GSM_CMD_RECORD				0x52 //'R', Record vehicle parameters
//Out: DE 04 52 00 08 51 1F 4C 4E 00 0C 30 1A A5 1C
//In : DE 04 D2 00 02 00 00 68 46
#define GSM_CMD_UPGRADE				0x55 //'U', Upgrade firmware
//Out: DE 02 55 00 03 05 00 DA E1 99 ; buf[5]=0 mean buf[6] is hw rev, others: block seq
//In : DE 02 D5 00 0A 00 00 00 6E 00 01 1F 40 33 CF 0D 08
//		buf[6]=0 表示后面跟的数据是FW版本号(2B)、FW长度(4B)、FW CRC16结果

#define GSM_CMD_ASK_BLK				0xFE //this CMD just for simulator

//record index define
#define REC_IDX_ADC1				10 //for ADC1
#define REC_IDX_ADC2				20 //for ADC2
#define REC_IDX_ADC3				30 //for ADC3
#define REC_IDX_ADC4				40 //for ADC4
#define REC_IDX_ADC5				50 //for ADC5
#define REC_IDX_MCU					60 //for MCU temperature
#define REC_IDX_V_TP1				70 //for Vol TP1
#define REC_IDX_V_TP2				72 //for Vol TP2
#define REC_IDX_V_TP3				74 //for Vol TP3
#define REC_IDX_V_TP4				76 //for Vol TP4
#define REC_IDX_V_TP5				78 //for Vol TP5
#define REC_IDX_GSM					90 //for GSM signal

struct gps_struct {
	unsigned int lat;
	unsigned int lon;
};


#endif /* _CONFIG_H */
