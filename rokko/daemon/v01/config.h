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
#include <fcntl.h>

#include <netdb.h>

#include <sys/param.h>
#include <sys/sysinfo.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>

#include <mysql.h>

#define	EMAIL				128
#define BUFSIZE 			2048

#define	LOG_DIR				"/tmp/"

#define BACKLOG 5        //Maximum queue of pending connections

//#define PERIOD_CHECK_DB		3*60	//check database every 3*60 seconds
//#define PERIOD_SEND_MAIL	1*60*60	//send mail every 1*60*60 seconds

#define PERIOD_CHECK_DB		10	//check database every 3*60 seconds
#define PERIOD_SEND_MAIL	30*60	//send mail every 1*60*60 seconds

#define NOTICER_ADDR		"cn0086@139.com"
#define EMERGENCY_ADDR		"cn0086@139.com"

//��������������timeout������Ӧ���ط�
//����5*timeout�������ָ��
//���� UPGRADE����ſ�����
#define CMD_TIMEOUT				5	//5��
#define MAX_CMD_QUEUE			30

//For GSM <==> Server protocol, need to same as STM32 firmware define
#define GSM_HEAD				0xDE
#define GSM_CMD_LOGIN			0x4C //'L', Login
#define GSM_CMD_UPGRADE			0x55 //'U', Upgrade firmware

//For firmware upgrade
#define MAX_FW_SIZE				61450 //60*1024+10
#define MIN_FW_SIZE				40960 //40*1024

#endif /* _CONFIG_H */