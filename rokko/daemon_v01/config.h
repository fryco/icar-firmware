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

#define	MAXCLIENT 					5000
#define PRODUCT_SN_LEN				8		//IMEI: 123456789012345 ==> 0x01 0x23 0x45 ... 0x45

#define	EMAIL						128
#define BUFSIZE 					2048	//2KB

#define	LOG_DIR						"/root/"

#define BACKLOG 5        			//Maximum queue of pending connections

#define PERIOD_CHECK_DB				3*60	//check database every 3*60 seconds
#define PERIOD_SEND_MAIL			3*60*60	//send mail every 1*60*60 seconds

//#define PERIOD_CHECK_DB			10		//check database every 3*60 seconds
//#define PERIOD_SEND_MAIL			30*60	//send mail every 1*60*60 seconds
#define PERIOD_SAVE_FILE			2*60	//Force save if timeout
#define FORCE_SAVE_FILE				1		//Force save flag

#define NOTICER_ADDR				"cn0086.info@gmail.com"
#define EMERGENCY_ADDR				"cn0086@139.com"

//发送命令后，如果在timeout内无响应，重发
//超过5*timeout后，清除此指令
//对于 UPGRADE，则放宽处理
#define CMD_TIMEOUT					5	//5秒
#define MAX_CMD_QUEUE				10

//For GSM <==> Server protocol, need to same as STM32 firmware define
#define GSM_HEAD					0xDE
#define GSM_CMD_CONSOLE				0x43 //'C', Console command
#define GSM_CMD_GPS					0x47 //'G', upload GPS information to server
#define GSM_CMD_ERROR				0x45 //'E', Upload err log
#define GSM_CMD_LOGIN				0x4C //'L', Login
#define GSM_CMD_RECORD				0x52 //'R', record gsm/adc data
#define GSM_CMD_UPGRADE				0x55 //'U', Upgrade firmware
#define GSM_CMD_WARN				0x57 //'W', warn msg, report to server

//For Console command
#define CONSOLE_CMD_NONE			0	 //no cmd
#define CONSOLE_CMD_LIST_ALL		0x4C //'L', List all
#define CONSOLE_CMD_LIST_SPE		0x6C //'l', List special

//For firmware upgrade
#define FW_CHK_PERIOD				7200//check new firmware each 2 hours
#define MAX_HW_TYPE					2	//2 type of hardware
#define FW_PATH_PREFIX				"./fw/stm32_v"
//#define MAX_FW_SIZE				61450 //60*1024+10
#define MAX_FW_SIZE					102400 //100*1024
#define MIN_FW_SIZE					40960 //40*1024

//For return error define, server ==> client
#define ERR_RETURN_NONE				0	//00: No err
#define ERR_RETURN_NO_LOGIN			1	//01: login failure
#define ERR_RETURN_SRV_BUSY			2	//02: server busy
#define ERR_RETURN_CRC_ERR			3	//03: CRC error
#define ERR_RETURN_FW_LATEST		4	//04: latest firmware, no need upgrade
#define ERR_RETURN_HW_ERR			5	//05: hardware no support
#define ERR_RETURN_CONSOLE_CMD		6	//执行console命令时出错
#define ERR_RETURN_LOGIN_CNT		7	//login count too many
#define ERR_RETURN_NO_SN			8	//No serial number
#define ERR_RETURN_NO_BLK			9	//Require block err, no this blk
#define ERR_RETURN_FILE_FAILURE		10	//Open firmware file err
#define ERR_UNKNOW					0xFF//未知错误

//record index define, need same as client main.h define
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

//For post cloud
//Need to define below Forum id in cloud server
//'36' ==> Machine
//'37' ==> Instruction 	
//'38' ==> Signal 	
//'39' ==> Sync time
//'40' ==> Login
//'41' ==> Log err
//'42' ==> Upgrade firmware / Update parameter
//'43' ==> Server

//#define	CLOUD_HOST				"cn0086.info"
#define	CLOUD_HOST					"yun.test.33xuexi.com"

//For database
#define	DB_HOST						"localhost"
#define	DB_USER						"root"
#define	DB_PWD						"cn0086"
#define	DB_NAME						"rokko_v00"

#endif /* _CONFIG_H */
