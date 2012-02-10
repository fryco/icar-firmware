/**
  ******************************************************************************
  * SVN revision information:
  * @file    $URL$ 
  * @version $Rev$
  * @author  $Author$
  * @date    $Date$
  * @brief   This is GSM applicaion
  ******************************************************************************
  */ 

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __APP_GSM_H
#define __APP_GSM_H

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x.h"

/* Exported macro ------------------------------------------------------------*/
#define IP_LEN			16 //123.123.123.123
/* Exported types ------------------------------------------------------------*/
typedef enum
{//same as define in drv_mg323.c
	DISCONNECT_NO_ERR = 0,	//normal, no error

	CONNECTION_DOWN	=	1,	//^SICI: 0,2,0
	PEER_CLOSED 	= 	2,	//^SIS: 0, 0, 48, Remote Peer has closed the connection
	PROFILE_NO_UP 	=	3,	//AT^SISI return is not 4: up

	NO_GPRS_IN_INIT	=	6,	//no gprs network
	GPRS_ATT_ERR	=	7,	//gprs attach failure
	CONN_TYPE_ERR	=	8,	//set connect type error
	GET_APN_ERR		=	9,	//get APN error
	SET_APN_ERR		=	10,	//set APN error
	SET_CONN_ERR	=	11,	//set conID error
	SVR_TYPE_ERR	=	12,	//set svr type error
	DEST_IP_ERR		=	13	//set dest IP and port error
} DISCONNECT_REASON;//enum must < 15

typedef enum
{//same as define in drv_mg323.c
	SHUTDOWN_NO_ERR	=	0,	//normal, no error
	NO_RESPOND		=	1, 	//if ( OSTime - mg323_status.at_timer > 10*AT_TIMEOUT )
							//or send AT no return
	SIM_CARD_ERR	=	2,	//Pin? no SIM?
	NO_GSM_NET		=	3,	//no GSM net or can't register
	NO_CARRIER_INFO	=	4,	//Get GSM carrier info failure
	SIGNAL_WEAK		=	5,	//gsm signal < MIN_GSM_SIGNAL
	NO_GPRS			=	6,	//mg323_status.gprs_count > 60

	TRY_ONLINE		=	13,	//if ( my_icar.mg323.try_online > MAX_ONLINE_TRY )
	RETURN_TOO_ERR	= 	14,	//if ( module_err_count > MAX_MODULE_ERR ) {//reboot
	MODULE_REBOOT	=	15	//if receive: SYSSTART
} POWEROFF_REASON; //enum must < 15

struct GSM_STATUS {
	bool ask_power ;
	bool power_on ;

	//POWEROFF_REASON power_off_reason;
	//unsigned int power_off_timer;

	bool gprs_ready ;
	unsigned char gprs_count;
	bool ask_online;
	bool tcp_online;
	unsigned char try_online;

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
void  App_TaskGsm        (void        *p_arg);

#endif /* __APP_GSM_H */
