/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __APP_GSM_H
#define __APP_GSM_H

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x.h"

/* Exported macro ------------------------------------------------------------*/
#define IP_LEN			16 //123.123.123.123
/* Exported types ------------------------------------------------------------*/
typedef enum
{
	NO_ERR = 0 ,//normal, no error
	POWER_ON_FAILURE = 1, //
	SIM_CARD_ERR = 2 ,//Pin? no SIM?
	NO_RESPOND = 3, //if ( OSTime - mg323_status.at_timer > 10*AT_TIMEOUT )
	TRY_ONLINE = 4, //if ( mg323_status.try_online > 15 )
	NO_GPRS  = 5  //mg323_status.gprs_count > 180
} SHUTDOWN_REASON;

struct GSM_STATUS {
	bool ask_power ;
	bool power_on ;

	SHUTDOWN_REASON power_off_reason;
	unsigned int power_off_timer;

	bool gprs_ready ;
	unsigned char gprs_count;
	bool ask_online;
	bool tcp_online;
	unsigned char try_online;

	//运营商信息，+COPS: 0,0,"CHINA MOBILE"
	unsigned char carrier[32];
	unsigned char imsi[16];
	unsigned char ip_local[IP_LEN];
	bool ip_updating;//updating to server 
	unsigned char *server_ip_port;
	unsigned int apn_index;
	unsigned char signal;
	unsigned char err_no;
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
