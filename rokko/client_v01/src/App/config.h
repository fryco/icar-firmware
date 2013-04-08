/**
 *      config - Copyright (c) 2011-2012 cn0086 <cn0086.info@gmail.com>
 *
 *      This is NOT a freeware, use is subject to license terms
 *
 *      $URL: svn://svn.cn0086.info/icar/firmware/APP_HW_v01/src/App/config.h $
 *      $Rev: 102 $, $Date: 2013-02-16 17:55:12 +0800 (鍛ㄥ叚, 2013-02-16) $
 */

#ifndef _CONFIG_H 
#define _CONFIG_H

#define DEBUG_OBD
#define DEBUG_GSM
#define DEBUG_PTC //for debug protocol
#define DEBUG_FLASH //For upgrade firmware
#define DEBUG_RECORD //For record command

#define	prompt(x, args...)	printf("[%d,%02d%%]> ",OSTime/100,OSCPUUsage);printf(x,	##args);

#ifdef DEBUG_OBD
	#define debug_obd(x, args...)  prompt(x, ##args);
#else
	#define debug_obd(x, args...)  ;
#endif

#ifdef DEBUG_GSM
	#define debug_gsm(x, args...)  prompt(x, ##args);
#else
	#define debug_gsm(x, args...)  ;
#endif

#ifdef DEBUG_PTC
	#define debug_ptc(x, args...)  prompt(x, ##args);
#else
	#define debug_ptc(x, args...)  ;
#endif

#ifdef DEBUG_FLASH
	#define debug_flash(x, args...)  prompt(x, ##args);
#else
	#define debug_flash(x, args...)  ;
#endif

#ifdef DEBUG_RECORD
	#define debug_record(x, args...)  prompt(x, ##args);
#else
	#define debug_record(x, args...)  ;
#endif

#define DEST_SERVER				"cqt.8866.org:23"
//#define DEST_SERVER				"cqt.8866.org:7558"
//#define DEST_SERVER				"svn.cn0086.info:23"
//#define DEST_SERVER				"COOLMAN2013.NO-IP.ORG:7557"
//#define DEST_SERVER				"122.49.20.174:5678"

#define CALL_BACK_NUMBER		"13828431106"

//For GSM <==> Server protocol
#define GSM_HEAD				0xDE
#define GSM_CMD_ERROR			0x45 //'E', upload error log to server
//[9] =  0xF0 ;// 1:POR, 2:Software rst, 4:IWDG, 8:Low power rst
//Out: DE 01 45 00 06 00 00 00 08 30 00 81 79
//In : DE 01 C5 00 02 00 04 08 4D
#define GSM_CMD_LOGIN			0x4C //'L', Login
//Out: DE 00 4C 00 1E 00 00 08 B6 44 45 4D 4F 43 34 33 45 45 39 00 00 00 DE 31 30 2E 38 31 2E 32 33 37 2E 39 36 FF E8
//In : DE 00 CC 00 11 00 50 EB D4 FC 06 6B FF 48 56 48 67 49 87 10 12 37 B8 C4
#define GSM_CMD_RECORD			0x52 //'R', Record vehicle parameters
//Out: DE 04 52 00 08 51 1F 4C 4E 00 0C 30 1A A5 1C
//In : DE 04 D2 00 02 00 00 68 46
#define GSM_CMD_UPGRADE			0x55 //'U', Upgrade firmware
//DE 02 55 00 03 05 00 DA E1 99
#define GSM_CMD_WARN			0x57 //'W', warn msg, report to server
//Out: DE 03 57 00 05 01 01 0A 20 01 54 96
//In : DE 03 D7 00 02 00 01 93 71

//old
#define GSM_CMD_TIME			0x54 //'T', time
#define GSM_CMD_UPDATE			0x75 //'u', Update parameter

//HEAD SEQ CMD Length(2 bytes) INF(Max.1024) CRC16
#define GSM_BUF_LENGTH			1152 //for GSM command, 1024+128

#define AT_CMD_LENGTH			64 //for GSM command, must < RX_BUF_SIZE in drv_uart.h
#define MAX_ONLINE_TRY			25 //if ( my_icar.mg323.try_online_cnt_cnt > MAX_ONLINE_TRY )
#define MAX_ERR_MSG				4  //ERR message, BackupRegister1 最多存储4种错误类型
#define MAX_WARN_MSG			4  //Warn message, uint32
#define MAX_RECORD_IDX			250 //will be removed later

#define MAX_RECORD_CNT			120 //Max. record count, must < 1024/sizeof(RECORD_STRUCTURE)

#define MIN_GSM_SIGNAL			8  //Min. GSM Signal require

#define MAX_PROG_TRY			16 //Max. program flash retry

#define AT_TIMEOUT				1*OS_TICKS_PER_SEC // 1 sec

#define VOICE_RESPOND_TIMEOUT	1*1*60*OS_TICKS_PER_SEC //60 secs
#define TCP_RESPOND_TIMEOUT		1*1*30*OS_TICKS_PER_SEC //30 secs
#define TCP_SEND_PERIOD			1*1*30*OS_TICKS_PER_SEC //30 secs
#define TCP_CHECK_PERIOD		1*1*30*OS_TICKS_PER_SEC //30 secs
#define CLEAN_QUEUE_PERIOD		1*2*60*OS_TICKS_PER_SEC //2  mins

#define RE_DIAL_PERIOD			1*2*60*OS_TICKS_PER_SEC //2  mins

//#define RTC_UPDATE_PERIOD		1*60*60 //1 Hour
#define RTC_UPDATE_PERIOD		1*2*60 //2 mins, use RTC time, +1 per second in RTC

//File name define, for warn msg report
//warn report format: unsigned int msg;//file name(1 Byte), msg(1 Byte), line(2 B)

#define F_MAIN				01	//main.c
#define F_APP_TASKMANAGER	02	//app_taskmanager.c
#define F_APP_GSM			03	//app_gsm.c
#define F_APP_OBD			04	//app_obd.c
#define F_DRV_can			05	//drv_can.c


#define SetLevel(io,pin,level)	 ((level)?(io->BSRR=(pin)):(io->BRR=(pin)))
#define GetLevel(io,pin)  		 (((io->IDR)&(pin))?1:0)
#define MCU_K_RECEIVE_SIGNAL    GetLevel(GPIOA,GPIO_Pin_3)

#define StartTimer()			TIM2->CR1|=0x0001
#define StopTimer()				TIM2->CR1&=0x03FE

#define DelayUs(us)				TIM2->CNT=0;TIM2->SR&=~TIM_FLAG_Update;while(TIM2->CNT<(us))
#define ResetUs()				TIM2->CNT=0;TIM2->SR&=~TIM_FLAG_Update

#endif /* _CONFIG_H */
