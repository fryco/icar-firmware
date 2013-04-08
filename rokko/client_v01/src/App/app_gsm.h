/**
  ******************************************************************************
  * SVN revision information:
  * @file    $URL: svn://svn.cn0086.info/icar/firmware/APP_HW_v01/src/App/app_gsm.h $ 
  * @version $Rev: 73 $
  * @author  $Author: cn0086.info $
  * @date    $Date: 2013-01-14 22:25:20 +0800 (鍛ㄤ竴, 2013-01-14) $
  * @brief   This is GSM applicaion
  ******************************************************************************
  */ 

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __APP_GSM_H
#define __APP_GSM_H

/* Includes ------------------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/
#define IP_LEN			16 //123.123.123.123

/* Exported types ------------------------------------------------------------*/
typedef enum
{//same as define in drv_mg323.c
	GPRS_NO_ERR = 0,	//normal, no error

	//Below is GSM power off reason
	NO_RESPOND		=	1, 	//if ( OSTime - mg323_status.at_timer > 10*AT_TIMEOUT )
							//or send AT no return
	GSM_HW			= 	2,	//GSM HW err, gsm_power_on() report err.
	NO_GPRS_NET		=	3,	//mg323_status.gprs_count > 60, Find GPRS network timeout
	MODULE_REBOOT	=	4,	//if receive: SYSSTART
	RSV5			= 	5,	//Reserve
	
	//Below is GPRS disconnect reason
	PEER_CLOSED 	= 	6,	//^SIS: 0, 0, 48, Remote Peer has closed the connection
	PROFILE_NO_UP 	=	7,	//AT^SISI return is not 4: up
	RX_TIMEOUT	 	=	8,	//RX data timeout after tx
	GPRS_SETTING	=	9,  //GPRS setting error
	CONNECTION_DOWN	=	10,	//^SICI: 0,2,0, Down 状态，Internet 连接已经定义但还没连接
	RSV11			= 	11,	//Reserve
	RSV12			= 	12,	//Reserve
	RSV13			= 	13,	//Reserve
	RSV14			= 	14,	//Reserve
	RSV15			= 	15	//Reserve
} GPRS_REASON;//enum must < 15


struct GSM_STATUS {
	bool ask_power ;
	bool power_on ;

	//POWEROFF_REASON power_off_reason;
	//unsigned int power_off_timer;

	bool gprs_ready ;
	unsigned char gprs_count;
	bool ask_online;
	bool tcp_online;
	unsigned char try_online_cnt;
	unsigned int try_online_time;

	//运营商信息，+COPS: 0,0,"CHINA MOBILE"
	unsigned char carrier[32];
	unsigned char imsi[16];
	unsigned char ip_local[IP_LEN];

	unsigned char *server_ip_port;
	unsigned int apn_index;
	unsigned char signal;
	bool roam;
	bool cgatt;
	bool rx_empty;//GSM Module rx buffer
	unsigned int at_timer;

	unsigned char ring_count;
	unsigned int dial_timer;
	bool need_dial ;
	bool voice_confirm ;
};

/* Exported constants --------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */
void  app_task_gsm        (void        *p_arg);

#endif /* __APP_GSM_H */
